#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define IPV4 AF_INET
#define IPV6 AF_INET6
#define MESSAGE_SIZE 40

typedef struct {
  double latitude;
  double longitude;
} Coordinate;

union ServerAddress {
  struct sockaddr_in serverAddressIPV4; 
  struct sockaddr_in6 serverAddressIPV6;
};

union ClientAddress {
  struct sockaddr_in clientAddressIPV4;
  struct sockaddr_in6 clientAddressIPV6;
};

void exitWithMessage(const char *msg, const char *detail);

union ServerAddress getServerAddressStructure (int ipType, in_port_t serverPort);

#endif // UTILS_H