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
  // Distância entre latitudes e longitudes
  double dLat = (lat2 - lat1) * M_PI / 180.0;
  double dLon = (lon2 - lon1) * M_PI / 180.0;

  // Converter para radianos
  lat1 = (lat1) * M_PI / 180.0;
  lat2 = (lat2) * M_PI / 180.0;

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
  Coordinate clientCoordinates; // Coordenadas o cliente

  // Receber mensagem do cliente
  char message[MESSAGE_SIZE];
  // Com a conexão estabelecida, é possível receber mensagem do cliente enviada pelo método send
  ssize_t numBytesReceived = recv(clientSocket, message, sizeof(message), 0);
  sscanf(message, "%lf, %lf", &clientCoordinates.latitude, &clientCoordinates.longitude);
  if (numBytesReceived < 0) {
    exitWithMessage("recv() falhou", NULL);
  }

  printServerClient();
  int acceptRide = 0;
  scanf("%d", &acceptRide);

  if (acceptRide == 0) {
    // Enviar mensagem de volta para o socket do cliente
    ssize_t numBytesSent = send(clientSocket, "NO_DRIVER_FOUND", sizeof("NO_DRIVER_FOUND"), 0);
    
    if (numBytesSent < 0)
      exitWithMessage("send() falhou", NULL);
  } else {
    int distance = round(haversine(clientCoordinates.latitude, clientCoordinates.longitude, serverCoordinates.latitude, serverCoordinates.longitude) * 1000);
    
    // Enviar string recebida e receber novamente até o final do fluxo
    while (1) { 
      char distanceMessage[countDigits(distance)+2];
      snprintf(distanceMessage, countDigits(distance)+2, "%dm", distance);

      // Ecoar mensagem de volta para o cliente
      ssize_t numBytesSent = -1;
      if (distance > 0) {
        numBytesSent = send(clientSocket, &distanceMessage, sizeof(distanceMessage), 0);
      } else {
        numBytesSent = send(clientSocket, "DRIVER_ARRIVED", sizeof("DRIVER_ARRIVED"), 0);
        printf("O motorista chegou!\n");
        break;
      }

      if (numBytesSent < 0)
        exitWithMessage("send() falhou", NULL);
  
      distance -= 400;
      sleep(2);
    }
  }

  close(clientSocket); // Fechar o socket do cliente
}

int main (int argc, char *argv[]) {
  if (argc != 3) {
    exitWithMessage("Parâmetro(s)", "<Porta do Servidor>");
  }

  // Criar estrutura de endereço do servidor
  int ipType = (strcmp(argv[1], "ipv6") == 0) ? IPV6 : IPV4; // Primeiro argumento: tipo de IP
  in_port_t servPort = atoi(argv[2]); // Segundo argumento: porta local
  union ServerAddress serverAddress = getServerAddressStructure(ipType, servPort);

  // O primeiro passo, deve ser a criação do socket para iniciar o fluxo de comunicação com conexão
  // Criação de um ponto final e comunicação
  int serverSock; // Descritor de socket para o servidor
  if ((serverSock = socket(ipType, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    exitWithMessage("socket() falhou", NULL);
  }

  // Abertura passiva, o servidor deve ser capaz de aceitar conexões
  // Vincular ao endereço local: Associa um endereço IP + Porta ao socket
  if (bind(serverSock, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
    exitWithMessage("bind() falhou", NULL);
  }

  // Marcar o socket para que ele escute as conexões de entrada
  // Define um socket como passivo, ou seja, para aceitar conexões
  if (listen(serverSock, MAX_DISPOSITIVES) < 0) {
    exitWithMessage("listen() falhou", NULL);
  }

  while (1) {
    printf("Aguardando solicitação.\n");
    union ClientAddress clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress); // Definir o tamanho da estrutura de endereço do cliente

    // Aguardar um cliente se conectar - Aceita uma conexão em um socket passivo
    int clientSock = accept(serverSock, (struct sockaddr *) &clientAddress, &clientAddressLength);
    if (clientSock < 0) {
      exitWithMessage("accept() falhou", NULL);
    }

    // Chamar a função para lidar com o cliente
    handleTCPClient(clientSock);
  }
}
