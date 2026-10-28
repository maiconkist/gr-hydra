from contextlib import ExitStack
import json
import multiprocessing
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import threading
import unittest
from unittest.mock import Mock, patch
from xmlrpc.server import SimpleXMLRPCServer

from gnuradio import blocks, gr, network, uhd
import hydra
import pmt
import yaml
import zmq

from qa_python import ROOT, load_script


def serve_control(connection, stopped):
    try:
        with zmq.Context() as context:
            with context.socket(zmq.REP) as socket:
                socket.linger = 0
                connection.send(socket.bind_to_random_port("tcp://127.0.0.1"))
                while not stopped.is_set():
                    if not socket.poll(100):
                        continue
                    request = socket.recv_json()
                    connection.send(request)
                    socket.send_json({"xvl_rep": {"status": True, "udp_port": 33042}})
    finally:
        connection.close()


class ControlBindingsTests(unittest.TestCase):
    def setUp(self):
        context = multiprocessing.get_context("spawn")
        self.connection, child_connection = context.Pipe(duplex=False)
        self.stopped = context.Event()
        self.server = context.Process(
            target=serve_control, args=(child_connection, self.stopped)
        )
        self.server.start()
        child_connection.close()
        self.addCleanup(self.stop_server)
        self.assertTrue(self.connection.poll(10), "Control server did not start")
        port = self.connection.recv()
        self.client = hydra.hydra_client("127.0.0.1", port, 41, "qa")
        self.addCleanup(setattr, self, "client", None)
        self.client.override_server_host("127.0.0.1")

    def stop_server(self):
        self.stopped.set()
        self.server.join(5)
        if self.server.is_alive():
            self.server.kill()
            self.server.join(5)
        self.connection.close()
        exitcode = self.server.exitcode
        self.server.close()
        self.assertEqual(exitcode, 0, "Control server failed or did not stop")

    def receive_request(self):
        self.assertTrue(self.connection.poll(5), "Control request was not received")
        return self.connection.recv()

    def test_resource_requests_update_configuration(self):
        for direction in ("rx", "tx"):
            with self.subTest(direction=direction):
                configuration = hydra.rx_configuration(1.42e9, 1e6)
                result = getattr(self.client, f"request_{direction}_resources")(
                    configuration
                )
                self.assertEqual(result, 0)
                self.assertEqual(configuration.server_port, 33042)
                self.assertEqual(configuration.server_ip, "127.0.0.1")
                self.assertEqual(
                    self.receive_request(),
                    {
                        f"xvl_r{direction}": {
                            "id": 41,
                            "centre_freq": 1.42e9,
                            "bandwidth": 1e6,
                            "ip": "127.0.0.1",
                        }
                    },
                )

    def test_query_and_release_round_trip(self):
        expected = {"xvl_rep": {"status": True, "udp_port": 33042}}
        self.assertEqual(json.loads(self.client.query_resources()), expected)
        self.assertEqual(self.receive_request(), {"xvl_que": ""})
        self.assertEqual(json.loads(self.client.free_resources()), expected)
        self.assertEqual(self.receive_request(), {"xvl_fre": {"id": 41}})

    def test_slice_manager_configures_ue_over_xmlrpc(self):
        script = load_script(ROOT / "app" / "slice_manager.py")
        state = {}
        server = SimpleXMLRPCServer(
            ("127.0.0.1", 0), allow_none=True, logRequests=False
        )
        self.addCleanup(server.server_close)
        for name in ("freqrx", "freqtx", "samp_rate", "vr1offset", "vr2offset"):
            server.register_function(
                lambda value, name=name: state.__setitem__(name, value),
                f"set_{name}",
            )
            server.register_function(lambda name=name: state[name], f"get_{name}")
        worker = threading.Thread(target=server.serve_forever, daemon=True)
        worker.start()
        self.addCleanup(worker.join, 5)
        self.addCleanup(server.shutdown)

        with script.ServerProxy(f"http://127.0.0.1:{server.server_address[1]}") as ue:
            radio = script.Slice(self.client, ue)
            for direction, remote_frequency in (("tx", "freqrx"), ("rx", "freqtx")):
                with self.subTest(direction=direction):
                    self.assertEqual(
                        getattr(radio, f"allocate_{direction}")(1.42e9, 400e3), 0
                    )
                    request = self.receive_request()[f"xvl_r{direction}"]
                    self.assertEqual(request["centre_freq"], 1.42e9)
                    self.assertEqual(request["bandwidth"], 400e3)
                    self.assertEqual(state[remote_frequency], 1.42e9)
                    self.assertEqual(state["samp_rate"], 400e3)
                    self.assertEqual(
                        (state["vr1offset"], state["vr2offset"]), (0, 0)
                    )


class StreamEndpoint(gr.hier_block2):
    def __init__(self, source):
        signature = gr.io_signature(1, 1, gr.sizeof_gr_complex)
        empty = gr.io_signature(0, 0, 0)
        gr.hier_block2.__init__(
            self,
            "qa_endpoint",
            empty if source else signature,
            signature if source else empty,
        )
        if source:
            self.connect(blocks.null_source(gr.sizeof_gr_complex), self)
        else:
            self.connect(self, blocks.null_sink(gr.sizeof_gr_complex))
        for name in (
            "start_client",
            "set_samp_rate",
            "set_center_freq",
            "set_gain",
            "set_normalized_gain",
            "set_antenna",
        ):
            setattr(self, name, Mock())


class TapEndpoint(gr.sync_block):
    def __init__(self, *args):
        gr.sync_block.__init__(self, "qa_tap", in_sig=None, out_sig=None)
        port = pmt.intern("pdus")
        self.message_port_register_in(port)
        self.message_port_register_out(port)
        self.set_msg_handler(port, lambda message: None)


class ExampleScriptsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.temporary.cleanup)
        environment = patch.dict(
            os.environ,
            {
                "QT_QPA_PLATFORM": "offscreen",
                "XDG_CONFIG_HOME": cls.temporary.name,
                "XDG_CACHE_HOME": cls.temporary.name,
            },
        )
        environment.start()
        cls.addClassCleanup(environment.stop)
        from PyQt5 import Qt

        cls.application = Qt.QApplication.instance() or Qt.QApplication([])
        cls.addClassCleanup(cls.application.quit)

    def test_cli_entrypoints_preserve_address_options(self):
        for path in sorted((ROOT / "grc_blocks" / "app").glob("ansible_*.py")):
            with self.subTest(script=path.name):
                result = subprocess.run(
                    [sys.executable, "-B", str(path), "--help"],
                    capture_output=True, text=True, timeout=30,
                )
                self.assertEqual(result.returncode, 0, result.stderr)
                is_server = path.stem.endswith("_server")
                option = "--ansibleIPPort" if is_server else "--ansibleIP"
                self.assertIn(option, result.stdout)
                script = load_script(path)
                address = "127.0.0.1:5000" if is_server else "127.0.0.1"
                options = script.argument_parser().parse_args([option, address])
                self.assertEqual(getattr(options, option[2:]), address)

    def test_flowgraphs_construct_with_real_dsp_blocks(self):
        for path in sorted((ROOT / "grc_blocks" / "app").glob("ansible_*.py")):
            with self.subTest(script=path.name), ExitStack() as stack:
                script = load_script(path)
                for module, name, source in (
                    (hydra, "hydra_gr_client_source", True),
                    (hydra, "hydra_gr_client_sink", False),
                    (uhd, "usrp_source", True),
                    (uhd, "usrp_sink", False),
                ):
                    stack.enter_context(
                        patch.object(
                            module,
                            name,
                            side_effect=lambda *args, source=source, **kwargs: (
                                StreamEndpoint(source)
                            ),
                        )
                    )
                stack.enter_context(patch.object(network, "tuntap_pdu", TapEndpoint))
                server_factory = stack.enter_context(
                    patch.object(hydra, "hydra_gr_server")
                )
                if hasattr(script, "SimpleXMLRPCServer"):
                    def local_server(address, **kwargs):
                        server = SimpleXMLRPCServer(
                            ("127.0.0.1", 0), logRequests=False, **kwargs
                        )
                        stack.callback(server.server_close)
                        stack.callback(server.shutdown)
                        return server

                    stack.enter_context(
                        patch.object(script, "SimpleXMLRPCServer", local_server)
                    )
                graph = getattr(script, path.stem)()
                stack.callback(graph.wait)
                stack.callback(graph.stop)
                if hasattr(graph, "close"):
                    stack.callback(graph.close)
                if hasattr(graph, "xmlrpc_server_0_thread"):
                    worker = graph.xmlrpc_server_0_thread
                    graph.xmlrpc_server_0.shutdown()
                    worker.join(5)
                    self.assertFalse(worker.is_alive())
                    graph.uhd_usrp_sink_0.set_normalized_gain.assert_called_once_with(
                        0.85, 0
                    )
                    graph.set_gain(0.5)
                    graph.uhd_usrp_sink_0.set_normalized_gain.assert_called_with(
                        0.5, 0
                    )
                elif path.stem.endswith("_server"):
                    graph.ahydra_gr_server_0_thread.join(5)
                    server = server_factory.return_value
                    server.start_server.assert_called_once_with()
                else:
                    self.assertIsInstance(graph.digital_ofdm_tx_0, gr.hier_block2)
                    self.assertIsInstance(graph.digital_ofdm_rx_0, gr.hier_block2)


class CompanionDefinitionsTests(unittest.TestCase):
    def test_block_definitions_load(self):
        from gnuradio.grc.core.platform import Platform
        from gnuradio.grc.core import schema_checker

        platform = Platform(version=gr.version())
        folder = Path(__file__).resolve().parents[1] / "grc_blocks" / "grc"
        for name in (
            "hydra_gr_client_sink",
            "hydra_gr_client_source",
            "hydra_gr_server",
        ):
            with self.subTest(block=name):
                filename = folder / f"{name}.block.yml"
                with filename.open(encoding="utf-8") as source:
                    definition = yaml.safe_load(source)
                checker = schema_checker.Validator(schema_checker.BLOCK_SCHEME)
                self.assertTrue(checker.run(definition), str(checker.messages))
                platform.load_block_description(definition, str(filename))
                self.assertIn(definition["id"], platform.blocks)

    def test_example_sources_regenerate(self):
        from gnuradio.grc.core.platform import Platform

        platform = Platform(version=gr.version())
        platform.build_library(
            [*platform.config.block_paths, str(ROOT / "grc_blocks" / "grc")]
        )
        self.addCleanup(platform._docstring_extractor.wait)
        with tempfile.TemporaryDirectory() as output:
            sources = (ROOT / "grc_blocks" / "app").glob("ansible_*.grc")
            for source in sorted(sources):
                with self.subTest(flowgraph=source.name):
                    graph, generated = platform.load_and_generate_flow_graph(
                        str(source), output
                    )
                    self.assertIsNotNone(graph)
                    self.assertIsNotNone(generated)
                    compile(Path(generated).read_bytes(), generated, "exec")


if __name__ == "__main__":
    unittest.main(verbosity=2)
