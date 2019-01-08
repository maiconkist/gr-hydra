#ifndef UTIL_UDP_INCLUDE_H
#define UTIL_UDP_INCLUDE_H

#include <stdlib.h>
#include <string>

namespace hydra {

int send_udp(std::string server_ip, std::string msg, int broadcast, int port);
int recv_udp(char *msg, size_t msg_len, int broadcast, int port, struct timeval timeout = {0, 0});
   

} /* namespace hydra */

#endif
