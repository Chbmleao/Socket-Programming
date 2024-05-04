#include "utils.h"

#define MAX_DISPOSITIVES 10
#define M_PI 3.14159265358979323846

Coordinate serverCoordinates = {-19.9227,-43.9451};

void printServerClient() {
  printf("Corrida disponível\n");
  printf("0 - Recusar\n");
  printf("1 - Aceitar\n");
}

double haversine(double lat1, double lon1, double lat2, double lon2) {
  // distance between latitudes and longitudes
  double dLat = (lat2 - lat1) * M_PI / 180.0;
  double dLon = (lon2 - lon1) * M_PI / 180.0;

  // convert to radians
  lat1 = (lat1) * M_PI / 180.0;
  lat2 = (lat2) * M_PI / 180.0;

  // apply formula
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
    exitWithMessage("recv() failed", NULL);
  }

  printServerClient();
  int acceptRide = 0;
  scanf("%d", &acceptRide);

  if (acceptRide == 0) {
    ssize_t numBytesSent = send(clientSocket, "NO_DRIVER_FOUND", sizeof("NO_DRIVER_FOUND"), 0);
    
    if (numBytesSent < 0)
      exitWithMessage("send() failed", NULL);
  } else {
    int distance = round(haversine(clientCoordinates.latitude, clientCoordinates.longitude, serverCoordinates.latitude, serverCoordinates.longitude) * 1000);
    
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
        printf("O motorista chegou!\n");
        break;
      }

      if (numBytesSent < 0)
        exitWithMessage("send() failed", NULL);
  
      distance -= 400;
      sleep(2);
    }
  }

  close(clientSocket); // Close client socket
}

int main (int argc, char *argv[]) {
  if (argc != 3) {
    exitWithMessage("Parameter(s)", "<Server Port>");
  }

  // Create server address structure
  int ipType = (strcmp(argv[1], "ipv6") == 0) ? IPV6 : IPV4; // First arg: ip type
  in_port_t servPort = atoi(argv[2]); // Second arg: local port
  union ServerAddress serverAddress = getServerAddressStructure(ipType, servPort);

  // Create socket for incoming connections
  int serverSock; // Socket descriptor for server
  if ((serverSock = socket(ipType, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    exitWithMessage("socket() failed", NULL);
  }

  // Bind to the local address
  if (bind(serverSock, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
    exitWithMessage("bind() failed", NULL);
  }

  // Mark the socket so it will listen for incoming connections
  if (listen(serverSock, MAX_DISPOSITIVES) < 0) {
    exitWithMessage("listen() failed", NULL);
  }

  while (1) {
    printf("Aguardando solicitação.\n");
    union ClientAddress clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress); // Set length of client address structure

    // Wait for a client to connect
    int clientSock = accept(serverSock, (struct sockaddr *) &clientAddress, &clientAddressLength);
    if (clientSock < 0) {
      exitWithMessage("accept() failed", NULL);
    }

    // Call function to handle client
    handleTCPClient(clientSock);
  }
}