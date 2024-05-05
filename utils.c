#include "utils.h"


/* 
   A função exitWithMessage imprime uma mensagem de erro junto com detalhes, 
   se fornecidos, e finaliza o programa.
   
   Parâmetros:
    - msg: Mensagem de erro principal.
    - detail: Detalhes adicionais sobre o erro (opcional).
*/
void exitWithMessage(const char *msg, const char *detail) {
  if (detail == NULL) {
    printf("%s\n", msg);
  } else {
    printf("%s: %s\n", msg, detail);
  }
  exit(1);
}

/* 
   A função getServerAddressStructure cria e retorna uma estrutura de endereço 
   para o servidor, dependendo do tipo de IP especificado (IPv4 ou IPv6).

   Parâmetros:
    - ipType: Tipo de endereço IP (IPv4 ou IPv6).
    - serverPort: Porta do servidor.
   
   Retorna:
    Uma estrutura de endereço para o servidor.
*/
union ServerAddress getServerAddressStructure (int ipType, in_port_t serverPort) {
  union ServerAddress serverAddress;
  memset(&serverAddress, 0, sizeof(serverAddress)); // Zera a estrutura

  if (ipType == IPV4) {
    serverAddress.serverAddressIPV4.sin_family = AF_INET; // Família de endereços IPv4
    serverAddress.serverAddressIPV4.sin_addr.s_addr = htonl(INADDR_ANY); // Qualquer interface de entrada
    serverAddress.serverAddressIPV4.sin_port = htons(serverPort); // Porta local
  } else if (ipType == IPV6) {
    serverAddress.serverAddressIPV6.sin6_family = AF_INET6; // Família de endereços IPv6
    serverAddress.serverAddressIPV6.sin6_addr = in6addr_any; // Qualquer interface de entrada
    serverAddress.serverAddressIPV6.sin6_port = htons(serverPort); // Porta local
  } else {
    exitWithMessage("Tipo de IP inválido", "O tipo de IP deve ser IPV4 ou IPV6");
  }

  return serverAddress;
}