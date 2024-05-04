#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define IPV4 AF_INET
#define IPV6 AF_INET6
#define MESSAGE_SIZE 40

typedef struct {
  double latitude;
  double longitude;
} Coordinate;

void printClientMenu(int hasAdditionalInfo, char *additionalInfo) {
  printf("-----------------------------------\n");
  hasAdditionalInfo ? 
    (printf("| $ %s                            |\n", additionalInfo)) 
    : 0; 
  printf("| $ 0 - Sair                      |\n");
  printf("| $ 1 - Solicitar Corrida         |\n");
  printf("|                                 |\n");
  printf("-----------------------------------\n");
}

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

int main(int argc, char *argv[]) {
  Coordinate clientCoordinates = {-19.892077728491678, -43.96541482752344};

  if(argc != 4)
    exitWithSystemMessage("Parameters: <IP_type> <IP_address)> <port>\n");

  int ipType = (strcmp(argv[1], "ipv6") == 0) ? IPV6 : IPV4;
  char *ipAddress = argv[2];
  int port = atoi(argv[3]);
  
  int endsProgram = -1;
  int additionalInfo = 0;
  while(endsProgram != 0) {
    printClientMenu(additionalInfo, "Não foi encontrado um motorista.");
    scanf("%d", &endsProgram);

    if(endsProgram == 0) break;

    // Create a reliable, stream socket using TCP
    int sock = socket(ipType, SOCK_STREAM, IPPROTO_TCP);
    if(sock == -1) 
      exitWithSystemMessage("socket() failed");

    // Construct the server address structure
    union ServerAddress serverAddress = getServerAddressStructure(ipType, port);

    // Converts the string representation of the server’s address into a 32-bit binary representation
    int returnValue = -1;
    if (ipType == IPV4)
      returnValue = inet_pton(AF_INET, ipAddress, &serverAddress.serverAddressIPV4.sin_addr.s_addr); 
    else 
      returnValue = inet_pton(AF_INET6, ipAddress, &serverAddress.serverAddressIPV6.sin6_addr);

    if(returnValue == 0)
      exitWithUserMessage("inet_pton() failed", "invalid address string");
    else if(returnValue < 0)
      exitWithSystemMessage("inet_pton() failed");

    // Establish the connection to the echo server
    if(connect(sock, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) == -1)
      exitWithSystemMessage("connect() failed");

    char message[MESSAGE_SIZE]; 
    sprintf(message, "%lf, %lf", clientCoordinates.latitude, clientCoordinates.longitude);

    ssize_t numBytes = send(sock, message, sizeof(message), 0);
    if (numBytes < 0)
      exitWithSystemMessage("send() failed");
    else if (numBytes != sizeof(message))
      exitWithUserMessage("send()", "sent unexpected number of bytes");

    int printLine = 1;

    char buffer[MESSAGE_SIZE];
    while(1) {
      memset(buffer, 0, sizeof(buffer));
      numBytes = recv(sock, buffer, sizeof(buffer) - 1, 0); // Adjust buffer size to leave space for null terminator

      if(numBytes < 0)
        exitWithSystemMessage("recv() failed");
      else if(numBytes == 0) 
        exitWithUserMessage("recv()", "connection closed prematurely");
      
        
      buffer[numBytes] = '\0'; // Ensure buffer is null-terminated

      if(strcmp(buffer, "NO_DRIVER_FOUND") == 0) {
        additionalInfo = 1;
        break;
      } else if (strcmp(buffer, "DRIVER_ARRIVED") == 0) {
        printf("| $ O motorista chegou.           |\n");
        printf("| $ <Encerrar programa >          |\n");
        printf("-----------------------------------\n");

        endsProgram = 0;
        break;
      } else {
        if(printLine)  {
          printf("-----------------------------------\n");
          printLine = 0;
        }
        printf("| $ Motorista a %s                 |\n", buffer);
      }      
    }

    close(sock);
  }

  exit(0);
  return 0;
}