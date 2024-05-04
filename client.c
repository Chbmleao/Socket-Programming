#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define IPV4 4
#define IPV6 6
#define BUFFER_SIZE 32
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

int main(int argc, char *argv[]) {
  Coordinate clientCoordinates = {-19.892077728491678, -43.96541482752344};

  if(argc != 4)
    exitWithSystemMessage("Parameters: <IP_type> <IP_address)> <port>\n");

  int ipType = (strcmp(argv[1], "ipv4") == 0) ? IPV4 : IPV6;
  char *ipAddress = argv[2];
  int port = atoi(argv[3]);
  
  int endsProgram = -1;
  int additionalInfo = 0;
  while(endsProgram != 0) {
    printClientMenu(additionalInfo, "Não foi encontrado um motorista.");
    scanf("%d", &endsProgram);

    if(endsProgram == 0) break;

    // Create a reliable, stream socket using TCP
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock == -1) 
      exitWithSystemMessage("socket() failed");

    // Construct the server address structure
    struct sockaddr_in serverAddress; // Server address
    memset(&serverAddress, 0, sizeof(serverAddress)); // Zero out structure
    serverAddress.sin_family = AF_INET; // IPv4 address family

    // Converts the string representation of the server’s address into a 32-bit binary representation
    int returnValue = inet_pton(AF_INET, ipAddress, &serverAddress.sin_addr.s_addr); 
    if(returnValue == 0)
      exitWithUserMessage("inet_pton() failed", "invalid address string");
    else if(returnValue < 0)
      exitWithSystemMessage("inet_pton() failed");
    serverAddress.sin_port = htons(port); 

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