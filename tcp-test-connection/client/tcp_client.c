#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#define MAX_BUFF 1000
#define PORT 8080
#define IP "127.0.0.1"
#define SA struct sockaddr


int main(void){
  // Defining sockets for client and server
  int sock_client, client_size;
  struct sockaddr_in server_addr;
  
  // Creating buffer for messages and responses.
  char server_message[MAX_BUFF];
  memset(server_message, '\0', sizeof(server_message));

  char client_message[MAX_BUFF];
  memset(client_message, '\0', sizeof(client_message));

  // Create client-side socket
  sock_client = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_client < 0){
    printf("Error while creating client socket!\n");
    return -1;
  }

  // Setting IP and Port for the server socket
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  server_addr.sin_addr.s_addr(IP);

  // Connecting to the server with defined address
  if(connect(sock_client, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
    printf("Error while trying to connect to the server!\n");
    return -1;
  }
  printf("Connection to the server has been established.");

  //Sending a message to the server
  strcpy(client_message, "Hello, server!\n");
  if(send(sock_client, client_message, strlen(client_message), 0) < 0){
    printf("Error while sending a message to the server!\n");
    return -1;
  }

  // Receiving a message from the server
  if(recv(sock_client, server_message, sizeof(server_message), 0) < 0 ){
    printf("Error while trying to receive a message from the server!\n");
    return -1;
  }
  printf("The message from the server: %s\n", server_message);

  close(sock_client);
  return 0;
}
