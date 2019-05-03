#ifndef HYDRA_LOGGER_INCLUDE_H
#define HYDRA_LOGGER_INCLUDE_H

#include <boost/algorithm/string.hpp>

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>

#include "hydra/types.h"

namespace hydra {

// Typdef the type of logger we want -- this one has levels and channels
typedef boost::log::sources::severity_channel_logger_mt<
  boost::log::trivial::severity_level, std::string> source_t;

class hydra_log
{
  public:
    // Default constructor
    hydra_log(){};

    // Real constructor
    hydra_log(
        const std::string &name,
        const boost::log::trivial::severity_level &level = boost::log::trivial::debug);

    // Overloaded << operators
    void operator<<(const std::string &log);

    // Direct access log levels
    void info(const std::string &log);
    void debug(const std::string &log);
    void warning(const std::string &log);
    void error(const std::string &log);

  private:

    // Initialise the logging backends
    void log_init(
        const std::string &name,
        const boost::log::trivial::severity_level &level);

    source_t p_logger;
};



}; /* namespace hydra */
#endif
