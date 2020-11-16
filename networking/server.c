#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include "socket.h"
#include <poll.h>

#define BUFFER_LEN 256

void* worker_th(void* arg) {

  
  printf("Client connected!\n");
  int client_socket_fd = *((int*)arg);
  // Set up file streams to access the socket
  FILE* to_client = fdopen(dup(client_socket_fd), "wb");
  if(to_client == NULL) {
    perror("Failed to open stream to client");
    exit(2);
  }
  
  FILE* from_client = fdopen(dup(client_socket_fd), "rb");
  if(from_client == NULL) {
    perror("Failed to open stream from client");
    exit(2);
  }
  
 
  // Send a message to the client
  
  // Flush the output buffer
  fflush(to_client);
  
  // Receive a message from the client
  char buffer[BUFFER_LEN];
  while(1){
    if(fgets(buffer, BUFFER_LEN, from_client) == NULL) {
      perror("Reading from client failed");
      exit(2);
    }
    if(strcmp(buffer, "quit\n") == 0){
      
      break;
    }
    for(int i = 0; i < strlen(buffer) - 1; i++) {
      buffer[i] = toupper(buffer[i]);
    }
    fprintf(to_client, "%s", buffer);
    fflush(to_client);
  }
	
  // Close file streams
  fclose(to_client);
  fclose(from_client);
  
  
  // Close sockets
  close(client_socket_fd);
  return NULL;
}


int main() {
  // Open a server socket
  unsigned short port = 0;
  int server_socket_fd = server_socket_open(&port);
  if(server_socket_fd == -1) {
    perror("Server socket was not opened");
    exit(2);
  }
	
  // Start listening for connections, with a maximum of one queued connection
  if(listen(server_socket_fd, 1)) {
    perror("listen failed");
    exit(2);
  }
	
  printf("Server listening on port %u\n", port);

  pthread_t th1;
  
  // Wait for a client to connect
  while(1) {
    int client_socket_fd = server_socket_accept(server_socket_fd);
    if(client_socket_fd == -1) {
      perror("accept failed");
      exit(2);
    }

    if(pthread_create(&th1, NULL, worker_th, &client_socket_fd) != 0) {
      perror("pthread_create failed\n");
      exit(2);
    }
      
  }	
    
  close(server_socket_fd);
	
  return 0;
}





















