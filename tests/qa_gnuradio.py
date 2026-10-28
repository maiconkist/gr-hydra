import json
import multiprocessing
from pathlib import Path
import unittest

from gnuradio import gr
import hydra
import yaml
import zmq


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


if __name__ == "__main__":
    unittest.main(verbosity=2)
