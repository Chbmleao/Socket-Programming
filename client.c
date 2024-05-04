#include "utils.h"

void printClientMenu() {
  printf("0 - Sair\n");
  printf("1 - Solicitar Corrida\n");
}

int main(int argc, char *argv[]) {
  Coordinate clientCoordinates = {-19.892077728491678, -43.96541482752344};

  if(argc != 4)
    exitWithMessage("Parameters: <IP_type> <IP_address)> <port>\n", NULL);

  int ipType = (strcmp(argv[1], "ipv6") == 0) ? IPV6 : IPV4;
  char *ipAddress = argv[2];
  int port = atoi(argv[3]);
  
  int endsProgram = -1;
  int noDriverFound = 0;
  while(endsProgram != 0) {
    if (noDriverFound)
      printf("Não foi encontrado um motorista\n");
    printClientMenu();
    scanf("%d", &endsProgram);

    if(endsProgram == 0) break;

    // Create a reliable, stream socket using TCP
    int sock = socket(ipType, SOCK_STREAM, IPPROTO_TCP);
    if(sock == -1) 
      exitWithMessage("socket() failed", NULL);

    // Construct the server address structure
    union ServerAddress serverAddress = getServerAddressStructure(ipType, port);

    // Converts the string representation of the server’s address into a 32-bit binary representation
    int returnValue = -1;
    if (ipType == IPV4)
      returnValue = inet_pton(AF_INET, ipAddress, &serverAddress.serverAddressIPV4.sin_addr.s_addr); 
    else 
      returnValue = inet_pton(AF_INET6, ipAddress, &serverAddress.serverAddressIPV6.sin6_addr);

    if(returnValue == 0)
      exitWithMessage("inet_pton() failed", "invalid address string");
    else if(returnValue < 0)
      exitWithMessage("inet_pton() failed", NULL);

    // Establish the connection to the echo server
    if(connect(sock, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) == -1)
      exitWithMessage("connect() failed", NULL);

    char message[MESSAGE_SIZE]; 
    sprintf(message, "%lf, %lf", clientCoordinates.latitude, clientCoordinates.longitude);

    ssize_t numBytes = send(sock, message, sizeof(message), 0);
    if (numBytes < 0)
      exitWithMessage("send() failed", NULL);
    else if (numBytes != sizeof(message))
      exitWithMessage("send()", "sent unexpected number of bytes");

    char buffer[MESSAGE_SIZE];
    while(1) {
      memset(buffer, 0, sizeof(buffer));
      numBytes = recv(sock, buffer, sizeof(buffer) - 1, 0); // Adjust buffer size to leave space for null terminator

      if(numBytes < 0)
        exitWithMessage("recv() failed", NULL);
      else if(numBytes == 0) 
        exitWithMessage("recv()", "connection closed prematurely");
      
        
      buffer[numBytes] = '\0'; // Ensure buffer is null-terminated

      if(strcmp(buffer, "NO_DRIVER_FOUND") == 0) {
        noDriverFound = 1;
        break;
      } else if (strcmp(buffer, "DRIVER_ARRIVED") == 0) {
        printf("O motorista chegou.\n");
        endsProgram = 0;
        break;
      } else {
        printf("Motorista a %s\n", buffer);
      }      
    }

    close(sock);
  }

  exit(0);
  return 0;
}