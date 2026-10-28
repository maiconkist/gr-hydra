#include <pybind11/pybind11.h>

#include <hydra/hydra_client.h>
#include <hydra/hydra_gr_client_sink.h>
#include <hydra/hydra_gr_client_source.h>
#include <hydra/hydra_gr_server.h>

#include <memory>
#include <string>

namespace py = pybind11;

PYBIND11_MODULE(hydra_python, module)
{
  py::module_::import("gnuradio.gr");

  using Configuration = ::hydra::rx_configuration;
  py::class_<Configuration>(module, "rx_configuration")
      .def(py::init<double, double>(), py::arg("cf"), py::arg("bw"))
      .def_readwrite("center_freq", &Configuration::center_freq)
      .def_readwrite("bandwidth", &Configuration::bandwidth)
      .def_readwrite("server_port", &Configuration::server_port)
      .def_readwrite("server_ip", &Configuration::server_ip);

  using Client = ::hydra::hydra_client;
  py::class_<Client>(module, "hydra_client")
      .def(py::init<std::string, unsigned int, unsigned int, std::string, bool>(),
           py::arg("client_ip") = "localhost", py::arg("u_port") = 5000,
           py::arg("u_client_id") = 10, py::arg("s_group_name") = "default",
           py::arg("b_debug") = false)
      .def("request_rx_resources", &Client::request_rx_resources, py::arg("rx_conf"),
           py::call_guard<py::gil_scoped_release>())
      .def("request_tx_resources", &Client::request_tx_resources, py::arg("tx_conf"),
           py::call_guard<py::gil_scoped_release>())
      .def("check_connection", &Client::check_connection, py::arg("max_tries") = 10,
           py::call_guard<py::gil_scoped_release>())
      .def("query_resources", &Client::query_resources,
           py::call_guard<py::gil_scoped_release>())
      .def("free_resources", &Client::free_resources,
           py::call_guard<py::gil_scoped_release>())
      .def("override_server_host", &Client::override_server_host, py::arg("s"));

  using Sink = gr::hydra::hydra_gr_client_sink;
  py::class_<Sink, gr::hier_block2, gr::basic_block, std::shared_ptr<Sink>>(
      module, "hydra_gr_client_sink")
      .def(py::init([](unsigned int id, const std::string &host,
                       unsigned int port, const std::string &group) {
             py::gil_scoped_release release;
             return Sink::make(id, host, port, group);
           }), py::arg("u_id"), py::arg("s_host"), py::arg("u_port"), py::arg("s_group"))
      .def("start_client", &Sink::start_client, py::arg("d_center_frequency"),
           py::arg("d_samp_rate"), py::arg("u_payload"),
           py::call_guard<py::gil_scoped_release>())
      .def("request_tx_resources", &Sink::request_tx_resources,
           py::arg("d_center_frequency"), py::arg("d_samp_rate"),
           py::arg("u_payload"), py::call_guard<py::gil_scoped_release>());

  using Source = gr::hydra::hydra_gr_client_source;
  py::class_<Source, gr::hier_block2, gr::basic_block, std::shared_ptr<Source>>(
      module, "hydra_gr_client_source")
      .def(py::init([](unsigned int id, const std::string &host,
                       unsigned int port, const std::string &group) {
             py::gil_scoped_release release;
             return Source::make(id, host, port, group);
           }), py::arg("u_id"), py::arg("s_host"), py::arg("u_port"), py::arg("s_group"))
      .def("start_client", &Source::start_client, py::arg("d_center_frequency"),
           py::arg("d_samp_rate"), py::arg("u_payload"),
           py::call_guard<py::gil_scoped_release>());

  using Server = gr::hydra::hydra_gr_server;
  py::class_<Server, gr::hier_block2, gr::basic_block, std::shared_ptr<Server>>(
      module, "hydra_gr_server")
      .def(py::init([](const std::string &address, const std::string &group) {
             py::gil_scoped_release release;
             return Server::make(address, group);
           }), py::arg("server_addr"), py::arg("group_name"))
      .def("set_tx_config", &Server::set_tx_config, py::arg("d_center_frequency"),
           py::arg("d_samp_rate"), py::arg("d_norm_gain"),
           py::arg("d_tx_fft_size"), py::arg("mode"),
           py::call_guard<py::gil_scoped_release>())
      .def("set_rx_config", &Server::set_rx_config, py::arg("d_center_frequency"),
           py::arg("d_samp_rate"), py::arg("d_norm_gain"),
           py::arg("d_tx_fft_size"), py::arg("mode"),
           py::call_guard<py::gil_scoped_release>())
      .def("start_server", &Server::start_server,
           py::call_guard<py::gil_scoped_release>());
}
