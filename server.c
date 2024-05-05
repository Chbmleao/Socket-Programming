/* server.c */

#include "utils.h" // Inclui as definições e declarações de funções do arquivo utils.h

#define MAX_DISPOSITIVES 10 // Número máximo de dispositivos (clientes) suportados pelo servidor
#define PI 3.14159265358979323846 // Valor de pi

Coordinate serverCoordinates = {-19.9227,-43.9451}; // Coordenadas do servidor (fixas para este exemplo)

/* 
   A função printServerClient imprime as opções disponíveis para o cliente (usuário).
   Neste caso, as opções são recusar ou aceitar uma corrida.
*/
void printServerClient(Coordinate clientCoordinates) {
  printf("Corrida disponível:\n");
  printf("Coordenadas: %lf, %lf\n", clientCoordinates.latitude, clientCoordinates.longitude);
  printf("0 - Recusar\n");
  printf("1 - Aceitar\n");
}

/* 
   A função haversine calcula a distância entre dois pontos na superfície da Terra 
   usando a fórmula de Haversine.

   Parâmetros:
    - lat1, lon1: Latitude e longitude do primeiro ponto.
    - lat2, lon2: Latitude e longitude do segundo ponto.
   
   Retorna:
    A distância entre os dois pontos, em quilômetros.
*/
double haversine(double lat1, double lon1, double lat2, double lon2) {
  // Distância entre latitudes e longitudes
  double dLat = (lat2 - lat1) * PI / 180.0;
  double dLon = (lon2 - lon1) * PI / 180.0;

  // Converter para radianos
  lat1 = (lat1) * PI / 180.0;
  lat2 = (lat2) * PI / 180.0;

  double a = pow(sin(dLat / 2), 2) + pow(sin(dLon / 2), 2) * cos(lat1) * cos(lat2);
  double rad = 6371; // Raio médio da Terra em quilômetros
  double c = 2 * asin(sqrt(a));
  return rad * c;
}

/* 
   A função countDigits conta o número de dígitos em um número inteiro.

   Parâmetros:
    - numero: O número inteiro.
   
   Retorna:
    O número de dígitos no número.
*/
int countDigits(int numero) {
  int digits = 0;
  if (numero == 0) return 1;
  while (numero != 0) {
    numero /= 10;
    digits++;
  }
  return digits;
}

/* 
   A função handleServerClientComunication é responsável por lidar com as interações entre o servidor 
   e um cliente específico.

   Parâmetros:
    - clientSocket: O descritor de socket para o cliente.
*/
void handleServerClientComunication(int clientSocket) {
  Coordinate clientCoordinates; // Coordenadas do cliente

  // Receber mensagem do cliente contendo suas coordenadas
  char message[MESSAGE_SIZE];
  ssize_t numBytesReceived = recv(clientSocket, message, sizeof(message), 0);
  sscanf(message, "%lf, %lf", &clientCoordinates.latitude, &clientCoordinates.longitude);
  if (numBytesReceived < 0) {
    exitWithMessage("recv() falhou", NULL);
  }

  // Imprimir opções disponíveis para o cliente
  printServerClient(clientCoordinates);

  int acceptRide = 0;
  scanf("%d", &acceptRide); // Aguardar a escolha do cliente

  if (acceptRide == 0) {
    // Se o cliente recusar a corrida, enviar mensagem correspondente de volta para o cliente
    ssize_t numBytesSent = send(clientSocket, "NO_DRIVER_FOUND", sizeof("NO_DRIVER_FOUND"), 0);
    if (numBytesSent < 0)
      exitWithMessage("send() falhou", NULL);
  } else {
    // Se o cliente aceitar a corrida, calcular a distância até o cliente
    int distance = round(haversine(clientCoordinates.latitude, clientCoordinates.longitude, serverCoordinates.latitude, serverCoordinates.longitude) * 1000);
    
    // Enviar distância ao cliente em intervalos regulares até que o motorista chegue ao destino
    while (1) { 
      char distanceMessage[countDigits(distance)+2];
      snprintf(distanceMessage, countDigits(distance)+2, "%dm", distance);

      ssize_t numBytesSent = -1;
      if (distance > 0) {
        // Enviar distância ao cliente
        numBytesSent = send(clientSocket, &distanceMessage, sizeof(distanceMessage), 0);
      } else {
        // Se a distância for zero, significa que o motorista chegou ao destino
        numBytesSent = send(clientSocket, "DRIVER_ARRIVED", sizeof("DRIVER_ARRIVED"), 0);
        printf("O motorista chegou!\n");
        break;
      }

      if (numBytesSent < 0)
        exitWithMessage("send() falhou", NULL);
  
      // Simular movimento do motorista e atualizar a distância
      distance -= 400; 
      sleep(2); // Aguardar 2 segundos antes de enviar a próxima atualização de distância
    }
  }

  close(clientSocket); // Fechar o socket do cliente após a interação
}

/* 
   A função principal (main) é responsável por inicializar e configurar o servidor, 
   aguardar conexões de clientes e chamar a função para lidar com cada cliente.

   Parâmetros:
    - argc: O número de argumentos de linha de comando.
    - argv: Um array de strings contendo os argumentos de linha de comando.
*/
int main (int argc, char *argv[]) {
  if (argc != 3) {
    // Verificar se o número correto de argumentos foi fornecido na linha de comando
    exitWithMessage("A entrada deve ter os seguintes parâmetros", "<Tipo de IP (ipv4 ou ipv6)> <Porta do Servidor>");
  }

  // Criar estrutura de endereço do servidor
  int ipType = (strcmp(argv[1], "ipv6") == 0) ? IPV6 : IPV4; // Tipo de IP
  in_port_t servPort = atoi(argv[2]); // Porta do servidor
  union ServerAddress serverAddress = getServerAddressStructure(ipType, servPort);

  // Criar socket do servidor
  int serverSock; // Descritor de socket para o servidor
  if ((serverSock = socket(ipType, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    exitWithMessage("socket() falhou", NULL);
  }

  // Associar o socket do servidor ao endereço IP e porta especificados
  if (bind(serverSock, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
    exitWithMessage("bind() falhou", NULL);
  }

  // Configurar o socket do servidor para aguardar conexões de entrada
  if (listen(serverSock, MAX_DISPOSITIVES) < 0) {
    exitWithMessage("listen() falhou", NULL);
  }

  while (1) {
    printf("Aguardando solicitação.\n");
    union ClientAddress clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);

    // Aguardar e aceitar conexões de clientes
    int clientSock = accept(serverSock, (struct sockaddr *) &clientAddress, &clientAddressLength);
    if (clientSock < 0) {
      exitWithMessage("accept() falhou", NULL);
    }

    // Lidar com a interação entre o servidor e o cliente
    handleServerClientComunication(clientSock);
  }
}
