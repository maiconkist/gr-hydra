#ifndef UTIL_UDP_INCLUDE_H
#define UTIL_UDP_INCLUDE_H

// STD dependencies
#include <stdlib.h>
#include <string>
#include <iostream>
#include <numeric>
#include <vector>
// Boost dependencies
//
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
// Other dependencies
#include <netdb.h>
#include <arpa/inet.h>

#include "hydra/hydra_log.h"

namespace hydra {

int send_udp(std::string server_ip, std::string msg, int broadcast, int port, hydra_log* logger = NULL);
int recv_udp(char *msg, size_t msg_len, int broadcast, int port, struct timeval timeout = {1, 0}, hydra_log* logger = NULL);


} /* namespace hydra */

#endif
