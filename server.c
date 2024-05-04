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
#define MAXPENDING 10
#define M_PI 3.14159265358979323846
#define MESSAGE_SIZE 40

typedef struct {
  double latitude;
  double longitude;
} Coordinate;

Coordinate coordServ = {-19.9227,-43.9451};

void exitWithUserMessage(const char *msg, const char *detail) {
  fputs(msg, stderr);
  fputs(": ", stderr);
  fputs(detail, stderr);
  fputc('\n', stderr);
  exit(1);
}

void exitWithSystemMessage(const char *msg) {
  perror(msg);
  exit(1);
}

void printServerClient() {
  printf("-----------------------------------\n");
  printf("| $ Corrida disponível            |\n");
  printf("| $ 0 - Recusar                   |\n");
  printf("| $ 1 - Aceitar                   |\n");
  printf("| $                               |\n");
  printf("-----------------------------------\n");
}

void printServerWaiting() {
  printf("-----------------------------------\n");
  printf("| $ Aguardando solicitação.       |\n");
  printf("| $                               |\n");
  printf("-----------------------------------\n");
}

double haversine(double lat1, double lon1, double lat2, double lon2) {
  // distance between latitudes
  // and longitudes
  double dLat = (lat2 - lat1) * M_PI / 180.0;
  double dLon = (lon2 - lon1) * M_PI / 180.0;

  // convert to radians
  lat1 = (lat1) * M_PI / 180.0;
  lat2 = (lat2) * M_PI / 180.0;

  // apply formulae
  double a = pow(sin(dLat / 2), 2) + pow(sin(dLon / 2), 2) * cos(lat1) * cos(lat2);
  double rad = 6371;
  double c = 2 * asin(sqrt(a));
  return rad * c;
}

int countDigits(int numero) {
    int digits = 0;
    
    if (numero == 0) return 1;
    
    while (numero != 0) {
      numero /= 10;
      digits++;
    }
    
    return digits;
}

void handleTCPClient(int clientSocket) {
  Coordinate clientCoordinates; // Buffer for echo string

  // Receive message from client
  char message[MESSAGE_SIZE];
  ssize_t numBytesReceived = recv(clientSocket, message, sizeof(message), 0);
  sscanf(message, "%lf, %lf", &clientCoordinates.latitude, &clientCoordinates.longitude);
  if (numBytesReceived < 0) {
    exitWithSystemMessage("recv() failed");
  }

  printf("Received from client: %f %f\n", clientCoordinates.latitude, clientCoordinates.longitude);

  printServerClient();
  int acceptRide = 0;
  scanf("%d", &acceptRide);

  if (acceptRide == 0) {
    ssize_t numBytesSent = send(clientSocket, "NO_DRIVER_FOUND", sizeof("NO_DRIVER_FOUND"), 0);
    
    if (numBytesSent < 0)
        exitWithSystemMessage("send() failed");
  } else {
    int distance = round(haversine(clientCoordinates.latitude, clientCoordinates.longitude, coordServ.latitude, coordServ.longitude) * 1000);
    
    // Send received string and receive again until end of stream
    while (1) { // 0 indicates end of stream
      char distanceMessage[countDigits(distance)+2];
      snprintf(distanceMessage, countDigits(distance)+2, "%dm", distance);

      // Echo message back to client
      ssize_t numBytesSent = -1;
      if (distance > 0) {
        numBytesSent = send(clientSocket, &distanceMessage, sizeof(distanceMessage), 0);
      } else {
        numBytesSent = send(clientSocket, "DRIVER_ARRIVED", sizeof("DRIVER_ARRIVED"), 0);
        break;
      }

      if (numBytesSent < 0)
        exitWithSystemMessage("send() failed");
  
      distance -= 400;
      sleep(2);
    }
  }

  close(clientSocket); // Close client socket
}

union ServerAddress {
  struct sockaddr_in serverAddressIPV4; 
  struct sockaddr_in6 serverAddressIPV6;
};

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
    exitWithUserMessage("Invalid IP type", "IP type must be either IPV4 or IPV6");
  }

  return serverAddress;
}

union ClientAddress {
  struct sockaddr_in clientAddressIPV4;
  struct sockaddr_in6 clientAddressIPV6;
};

int main (int argc, char *argv[]) {
  printServerWaiting();
  printf("%s",argv[0]);

  if (argc != 3) {
    exitWithUserMessage("Parameter(s)", "<Server Port>");
  }

  // Create server address structure
  int ipType = (strcmp(argv[1], "ipv6") == 0) ? IPV6 : IPV4; // First arg: ip type
  in_port_t servPort = atoi(argv[2]); // Second arg: local port
  union ServerAddress serverAddress = getServerAddressStructure(ipType, servPort);

  // Create socket for incoming connections
  int serverSock; // Socket descriptor for server
  if ((serverSock = socket(ipType, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    exitWithSystemMessage("socket() failed");
  }

  // Bind to the local address
  if (bind(serverSock, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
    exitWithSystemMessage("bind() failed");
  }

  // Mark the socket so it will listen for incoming connections
  if (listen(serverSock, MAXPENDING) < 0) {
    exitWithSystemMessage("listen() failed");
  }

  while (1) {
    union ClientAddress clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress); // Set length of client address structure

    // Wait for a client to connect
    int clientSock = accept(serverSock, (struct sockaddr *) &clientAddress, &clientAddressLength);
    if (clientSock < 0) {
      exitWithSystemMessage("accept() failed");
    }

    // Call function to handle client
    handleTCPClient(clientSock);
  }
}