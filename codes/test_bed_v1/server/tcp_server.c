#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define MAX_BUFF 1000
#define PORT 8080
#define IP "10.34.15.5"
#define SA struct sockaddr


int main(void){
  // Defining sockets for client and server
  int sock_server, sock_client, client_size;
  struct sockaddr_in server_addr, client_addr;
  
  // Creating buffer for messages and responses.
  char server_message[MAX_BUFF];
  memset(server_message, '\0', sizeof(server_message));

  char client_message[MAX_BUFF];
  memset(client_message, '\0', sizeof(client_message));

  // Create Server-side socket
  sock_server = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_server < 0){
    printf("Error while creating server socket!\n");
    return -1;
  }

  // Setting IP and Port for the server socket
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  server_addr.sin_addr.s_addr = inet_addr(IP);

  // Binding the socket to the pair of IP and Port
  if(bind(sock_server, (struct sockaddr*)&server_addr, sizeof(server_addr))<0){
    printf("Couldn't bind to the pair of IP and Port!\n");
    return -1;
  }
  printf("Socket has been created and binded.\n");

  // Starting to listen for the clients
  if(listen(sock_server, 1) < 0){
    printf("Error while listening!\n");
    return -1;
  }
  printf("Server is listening for incoming connections.\n");

  // Accepting incoming connections.
  client_size = sizeof(client_addr);
  sock_client = accept(sock_server, (struct sockaddr*)&client_addr, &client_size);
  if(sock_client < 0){
    printf("Error while accepting client!\n");
    return -1;
  }
  printf("Client is connected.\n");

  // Receiving client message
  if(recv(sock_client, client_message, sizeof(client_message), 0) < 0){
    printf("Error while receiving client message!\n");
    return -1;
  }
  printf("Message from client: %s. \n", client_message);

  // Sending message to the client
  strcpy(server_message, "Hello, client!");
  if(send(sock_client, server_message, strlen(server_message), 0) < 0){
    printf("Error while sending message to the client!\n");
    return -1;
  } 
  printf("message has been sent to the client.\n");

  // Closing connections
  close(sock_client);
  close(sock_server);

  return 0;
}
