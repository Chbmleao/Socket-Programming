#include "utils.h"

// Função para imprimir o menu do cliente
void printClientMenu() {
  printf("0 - Sair\n");
  printf("1 - Solicitar Corrida\n");
}

int main(int argc, char *argv[]) {
  // Coordenadas do cliente (fixas para este exemplo)
  Coordinate clientCoordinates = {-19.892077728491678, -43.96541482752344};

  // Verifica se o número correto de argumentos foi fornecido na linha de comando
  if(argc != 4)
    exitWithMessage("Parâmetros: <Tipo_IP> <Endereço_IP> <porta>\n", NULL);

  // Determina o tipo de IP a ser usado e extrai o endereço IP e a porta
  int ipType = (strcmp(argv[1], "ipv6") == 0) ? IPV6 : IPV4;
  char *ipAddress = argv[2];
  int port = atoi(argv[3]);
  
  int endsProgram = -1; // Variável de controle do loop principal
  int noDriverFound = 0; // Flag para indicar se nenhum motorista foi encontrado

  // Loop principal do programa
  while(endsProgram != 0) {
    if (noDriverFound)
      printf("Não foi encontrado um motorista\n");

    // Exibe o menu do cliente e aguarda a entrada do usuário
    printClientMenu();
    scanf("%d", &endsProgram);

    if(endsProgram == 0) break; // Se o usuário optar por sair, encerra o loop principal

    // Cria um socket confiável e de fluxo usando TCP
    int sock = socket(ipType, SOCK_STREAM, IPPROTO_TCP);
    if(sock == -1) 
      exitWithMessage("socket() falhou", NULL);

    // Constrói a estrutura de endereço do servidor
    union ServerAddress serverAddress = getServerAddressStructure(ipType, port);

    // Converte a representação de string do endereço do servidor em uma representação binária de 32 bits
    int returnValue = -1;
    if (ipType == IPV4)
      returnValue = inet_pton(AF_INET, ipAddress, &serverAddress.serverAddressIPV4.sin_addr.s_addr); 
    else 
      returnValue = inet_pton(AF_INET6, ipAddress, &serverAddress.serverAddressIPV6.sin6_addr);
    if(returnValue == 0)
      exitWithMessage("inet_pton() falhou", "endereço inválido");
    else if(returnValue < 0)
      exitWithMessage("inet_pton() falhou", NULL);

    // Estabelece a conexão com o servidor
    if(connect(sock, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) == -1)
      exitWithMessage("connect() falhou", NULL);

    char message[MESSAGE_SIZE]; 
    sprintf(message, "%lf, %lf", clientCoordinates.latitude, clientCoordinates.longitude);

    // Envia as coordenadas do cliente para o servidor
    ssize_t numBytes = send(sock, message, sizeof(message), 0);
    if (numBytes < 0)
      exitWithMessage("send() falhou", NULL);
    else if (numBytes != sizeof(message))
      exitWithMessage("send()", "número inesperado de bytes enviados");

    char buffer[MESSAGE_SIZE];
    // Loop para receber mensagens do servidor
    while(1) {
      memset(buffer, 0, sizeof(buffer));
      numBytes = recv(sock, buffer, sizeof(buffer) - 1, 0); // Ajusta o tamanho do buffer para deixar espaço para o terminador nulo
      buffer[numBytes] = '\0'; // Garante que o buffer esteja terminado em nulo
      if(numBytes < 0)
        exitWithMessage("recv() falhou", NULL);
      else if(numBytes == 0) 
        exitWithMessage("recv()", "conexão fechada prematuramente");

      // Verifica as mensagens recebidas do servidor
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

    // Fecha o descritor de arquivo do socket
    close(sock);
  }

  return 0;
}
