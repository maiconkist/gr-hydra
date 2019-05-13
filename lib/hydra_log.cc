#include "hydra/hydra_log.h"

namespace hydra {

void
hydra_log::log_init(
    const std::string &name,
  const boost::log::trivial::severity_level &level)
{
  boost::log::register_simple_formatter_factory<
    boost::log::trivial::severity_level, char>("Severity");

  // Specify file backend characteristic
  boost::log::add_file_log(
    boost::log::keywords::target = "logs",
    boost::log::keywords::auto_flush= true,
    boost::log::keywords::filter = (boost::log::expressions::attr<std::string>("Channel")== name),
    boost::log::keywords::file_name = "logs/" + boost::algorithm::to_lower_copy(name) + ".log",
    boost::log::keywords::rotation_size = 10 * 1024 * 1024,
    boost::log::keywords::scan_method = boost::log::sinks::file::scan_matching,
    boost::log::keywords::format = "[%TimeStamp%][%Channel%][%Severity%]: %Message%");

  // Specify console backend characteristics
  boost::log::add_console_log(
    std::cout,
    boost::log::keywords::filter = (boost::log::expressions::attr<std::string>("Channel")== name),
    boost::log::keywords::format = "[%TimeStamp%][%Channel%][%Severity%]: %Message%");

  // Filter the log output
  boost::log::core::get()->set_filter(boost::log::trivial::severity >= level);

  // Include common attributes, e.g., timestamp
  boost::log::add_common_attributes();
};

hydra_log::hydra_log(
    const std::string &name,
    const boost::log::trivial::severity_level &level)
{
  // Create the logger backend
  log_init(name, level);

  // Create logger object
  p_logger = source_t(
      boost::log::keywords::severity = boost::log::trivial::info,
      boost::log::keywords::channel = name);
};

void
hydra_log::operator<<(const std::string &log)
{
 // Log the input stream with the default log level
 BOOST_LOG(p_logger) << log;
};

void
hydra_log::info(const std::string &log)
{
 // Log the input stream with the info log level
 BOOST_LOG_SEV(p_logger, boost::log::trivial::info) << log;
};

void
hydra_log::debug(const std::string &log)
{
 // Log the input stream with the debug log level
 BOOST_LOG_SEV(p_logger, boost::log::trivial::debug) << log;
};

void
hydra_log::warning(const std::string &log)
{
 // Log the input stream with the warning log level
 BOOST_LOG_SEV(p_logger, boost::log::trivial::warning) << log;
};

void
hydra_log::error(const std::string &log)
{
 // Log the input stream with the error log level
 BOOST_LOG_SEV(p_logger, boost::log::trivial::error) << log;
};

} // namespace hydra
