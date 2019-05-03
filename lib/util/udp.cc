#include "hydra/util/udp.h"


namespace hydra {

int send_udp(std::string server_ip,
    std::string msg,
    int broadcast,
    int port,
    hydra_log* logger)
{
  int sd, rc;
  struct sockaddr_in cliAddr, remoteServAddr;
  struct hostent *h;

  // Change the server_ip address to end with '255' if broadcast == true
  if (broadcast)
  {
    // Temporary vector
    std::vector<std::string> sp;
    // Split the server string
    boost::split(sp, server_ip, boost::is_any_of("."));

    // Check size
    if (sp.size()  != 4)
    {
      if (logger != nullptr)
      {
        logger->error("Invalid IP format.");
      }
      else
      {
        std::cout << "<server/autodiscovery> Invalid IP format." << std::endl;
      }

      return -1;
    }

    // Replace last octet
    sp[3] = "255";
    // Join new string
    server_ip =  boost::algorithm::join(sp, ".");
  }

   /* get server IP address (no check if input is IP address or DNS name */
   h = gethostbyname(server_ip.c_str());
   // Exit if get hostname failed
   if (h == NULL){ exit(1);}

   remoteServAddr.sin_family = h->h_addrtype;
   memcpy((char *) &remoteServAddr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
   remoteServAddr.sin_port = htons(port);

   /* socket creation */
   sd = socket(AF_INET,SOCK_DGRAM,0);

   if(sd<0) return -1;
   if (setsockopt(sd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast) == -1) return -1;

   /* bind any port */
   cliAddr.sin_family = AF_INET;
   cliAddr.sin_addr.s_addr = htonl(INADDR_ANY);
   cliAddr.sin_port = htons(0);

   rc = bind(sd, (struct sockaddr *) &cliAddr, sizeof(cliAddr));

   if (rc<0)
   {
       if (logger != nullptr)
      {
        logger->error("Could not bind.");
      }
      else
      {
        std::cout << "<server/autodiscovery> Could not bind." << std::endl;
      }

      close(sd);
      return -1;
   }

   if (logger != nullptr)
   {
     logger->info(str(boost::format("Sending: '%s' to %s:%d") % msg % server_ip % port ));
   }
   else
   {
     std::cout << boost::format("<server/autodiscovery> Sending: '%s' to %s:%d") % msg % server_ip % port  << std::endl;
   }

   /* send data */
   rc = sendto(sd,
               msg.c_str(),
               msg.length()+1,
               0,
               (struct sockaddr *) &remoteServAddr,
               sizeof(remoteServAddr));

   if (rc<0)
   {
       if (logger != nullptr)
      {
        logger->error("Could not send.");
      }
      else
      {
        std::cout << "<server/autodiscovery> Could not send." << std::endl;
      }

      close(sd);
      return -1;
   }

   close(sd);
   return 1;
}

int
recv_udp(char *msg,
    size_t msg_len,
    int broadcast,
    int port,
    struct timeval timeout,
    hydra_log* logger)
{
   int sd, rc, n, cliLen;
   struct sockaddr_in cliAddr, servAddr;

   /* socket creation */
   sd=socket(AF_INET, SOCK_DGRAM, 0);
   if (sd<0)
   {
     if (logger != nullptr)
     {
       logger->error("Could not open socket.");
     }
     else
     {
       std::cout << "<server/autodiscovery> Could not open socket." << std::endl;
     }
     return -1;
   }

   if (setsockopt(sd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1)
   {

  if (logger != nullptr)
     {
       logger->error("Could not setsockopt BROADCAST.");
     }
     else
     {
       std::cout << "<server/autodiscovery> Could not setsockopt BROADCAST." << std::endl;
     }
     return -1;
   }

   if ((timeout.tv_sec > 0 || timeout.tv_usec > 0) &&
       (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval)) == -1))
   {
     if (logger != nullptr)
     {
       logger->error("Could not setsockopt SO_RCVTIMEO.");
     }
     else
     {
       std::cout << "<server/autodiscovery> Could not setsockopt SO_RCVTIMEO." << std::endl;
     }
     return -1;
   }

   /* bind local server port */
   servAddr.sin_family = AF_INET;
   servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
   servAddr.sin_port = htons(port);
   rc = bind(sd, (struct sockaddr *) &servAddr, sizeof(servAddr));

   // std::cout << boost::format("waiting for data on port UDP %u") %  port << std::endl;

   //while(1) {
   /* init buffer */
   memset(msg, 0x0, msg_len);

   /* receive message */
   cliLen = sizeof(cliAddr);

   n = recvfrom(sd, msg, msg_len, 0, (struct sockaddr *) &cliAddr, (socklen_t *) &cliLen);

   if (n == EAGAIN || n == EWOULDBLOCK || n < 0)
   {
      close(sd);
      return -1;
   }

   /* print received message */
   if (logger != nullptr)
   {
     logger->info(str(boost::format("Received from %s: UDP %u: %s") % inet_ntoa(cliAddr.sin_addr) % ntohs(cliAddr.sin_port) % msg));
   }
   else
   {
     std::cout << boost::format("<server/autodiscovery> Received from %s: UDP %u: %s") % inet_ntoa(cliAddr.sin_addr) % ntohs(cliAddr.sin_port) % msg << std::endl;
   }

   //}
   close(sd);
   return 0;
}

} /* namespace hydra */
