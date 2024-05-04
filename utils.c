#include "utils.h"

void exitWithMessage(const char *msg, const char *detail) {
  if (detail == NULL) {
    printf("%s\n", msg);
  } else {
    printf("%s: %s\n", msg, detail);
  }
  exit(1);
}

union ServerAddress getServerAddressStructure (int ipType, in_port_t servPort) {
  union ServerAddress serverAddress;
  memset(&serverAddress, 0, sizeof(serverAddress)); // Zero out structure

  if (ipType == IPV4) {
    serverAddress.serverAddressIPV4.sin_family = AF_INET; // IPv4 address family
    serverAddress.serverAddressIPV4.sin_addr.s_addr = htonl(INADDR_ANY); // Any incoming interface
    serverAddress.serverAddressIPV4.sin_port = htons(servPort); // Local port
  } else if (ipType == IPV6) {
    serverAddress.serverAddressIPV6.sin6_family = AF_INET6; // IPv6 address family
    serverAddress.serverAddressIPV6.sin6_addr = in6addr_any; // Any incoming interface
    serverAddress.serverAddressIPV6.sin6_port = htons(servPort); // Local port
  } else {
    exitWithMessage("Invalid IP type", "IP type must be either IPV4 or IPV6");
  }

  return serverAddress;
}