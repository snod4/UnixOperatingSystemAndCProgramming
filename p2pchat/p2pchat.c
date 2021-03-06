#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "socket.h"
#include "ui.h"

// a "large" arbitrary number of connections a single peer can have
#define MAX_CONNECTIONS 20
/*
each peer needs its main thread to be listening for connections in infiite while loop

the peers need to create a new thread for listening for messages from each connection they have

a peer needs a thread to handle writigin new messages to broadcast to all connenctions 

NEED:
linked list for keeping track of conneceted peers to forward messages to (and listen to)

 */

/********************* Linked List Data Struct ***********************/



typedef struct node {
  int fd;
  struct node * next;
} node_t;

typedef struct peer_list {
  node_t * head;
  pthread_mutex_t big_lock;
} peer_list_t;

// List of peers that this machine is connected to
peer_list_t p_list;

/**
 * Add a new peer node to the global linked list of peers
 *
 * \param fd - the int socket number of the peer to add
 */
void add_peer(int fd) {
  node_t* new_peer = (node_t*) malloc(sizeof(node_t));
  new_peer->fd = fd;

  // add new peer to the global linked list
  pthread_mutex_lock(&p_list.big_lock);

  new_peer->next = p_list.head;
  p_list.head = new_peer;
  
  pthread_mutex_unlock(&p_list.big_lock);
}

/**
 * Remove a peer connection from the list of peers this machine is connected to.
 *
 * \param fd - the socket number of the peer to remove.
 */
void remove_peer(int fd) {
  // lock to prevent atomicity violations
  pthread_mutex_lock(&p_list.big_lock);
  node_t *curr, *prev, *next;
  curr = p_list.head;
  prev = NULL;
  
  while(curr != NULL) {
    next = curr->next;

    //check if curr is peer to remove
    if(curr->fd == fd) {
      //check if curr was head of list
      if(prev == NULL) {
        //reset head to remove
        p_list.head = next;
      } else {
        //remove curr from middle of list
        prev->next = next;
      }

      //free space
      free(curr);
      break;
    } else {
      //advance pointers
      prev = curr;
      curr = curr->next;
    }
  }
  pthread_mutex_unlock(&p_list.big_lock);
}

/*********************************************************************/



// Keep the username & fd in a global so we can access it from the callback
const char* username;
int my_fd;

// Struct that holds message meta-data so that threads know how much to read
typedef struct header {
  int msg_len;
  int usr_len;
} header_t;


/**
 * Send a message to all the peers that this machine is connected to.
 *
 * \param user - string username of message sender
 * \param msg - string message to send
 */
void forward_message(const char* user, const char* msg, int sender_fd) {
  // create header struct
  header_t * metadata =(header_t *) malloc(sizeof(header_t)); 
  metadata->usr_len = strlen(user) + 1; //add 1 for null character
  metadata->msg_len = strlen(msg) + 1;
  char * usr  = strdup(user);
  char * message  = strdup(msg);
  
  // iterate peer linked list, sending message to each one
  node_t* curr = p_list.head;
  while(curr != NULL) {
    int fd = curr->fd;

    // dont send messages back to sender 
    if(fd != sender_fd) {
      // send the meta-data
      if(write(fd, metadata, sizeof(header_t)) < 0){
        fprintf(stderr,"Header Write Failed\n");
      }
      // send username of message sender
      if(write(fd, usr, metadata->usr_len) < 0){
        fprintf(stderr,"Username Write Failed\n");
      }
      // get message 
      if(write(fd, message, metadata->msg_len) < 0){
        fprintf(stderr,"Message Write Failed\n");
      }
    }

    curr = curr->next;
  }
  
}

void input_callback(const char* message);


/**
 * Listen for messages from a particular peer (indicated by file_descriptor)
 * and forward messages from them on to all other peers connected to in this
 * peer-to-peer network. This thread function should be running for each of
 * the peer networks connected to.
 *
 * \param file_descriptor - port number of a peer that this machine is connected to
 */
void* handle_messages(void* file_descriptor) {
  int fd = (int)file_descriptor;
  header_t header;
  
  while(1) {
    // Get the header with message meta data
    int disconnectVal;
    if((disconnectVal = read(fd, &header, sizeof(header_t))) <= 0){
      if(disconnectVal != 0){
        fprintf(stderr,"Header Read Failed\n");
      }
      break; // peer connection broken, discontinue communication
    }
    
    else {
      char username[header.usr_len];
      char message[header.msg_len];

      //get username of message sender
      if(read(fd, username, header.usr_len) <= 0){
        fprintf(stderr,"Username Read Failed\n");
        break; // peer connection broken, discontinue communication
      }
      //get message 
      else if(read(fd, message, header.msg_len) <= 0){
        fprintf(stderr,"Message Read Failed\n");
        break; // peer connection broken, discontinue communication
      }
      else { 
        // Display the message on the UI and forward to other peers
        ui_display(username, message);

        // forward message data to peers
        forward_message(username, message, fd);
      }
    }

  }

  // remove this thread's peer fd from peer linked list
  // dont send them any more messsages
  remove_peer(fd);
  close(fd);
  
  return NULL;
}


// This function is run whenever the user hits enter after typing a message
void input_callback(const char* message) {
  if(strcmp(message, ":quit") == 0 || strcmp(message, ":q") == 0) {
    ui_exit();
  } else {
    // display message you typed
    ui_display(username, message);

    // send your message to others
    forward_message(username, message, my_fd);
  }
}


/**
 * Listen for new connections while chat program is running.
 *
 * \param socket_fd - the socket fd that is listening for connections
 */
void * accept_connections(void * socket_fd){
  int server_socket_fd = (int)socket_fd;
  while(1) { 
    // Wait for a peer to connect
    int incoming_peer_socket_fd = server_socket_accept(server_socket_fd);
    if(incoming_peer_socket_fd == -1) {
      fprintf(stderr, "Incoming peer couldn't connect\n");
    }

    // add new connection to peer linked list
    add_peer(incoming_peer_socket_fd);

    //launch threads for htings
    pthread_t peer_thread;
    pthread_create(&peer_thread, NULL, handle_messages, (void *)(intptr_t) incoming_peer_socket_fd);
  
  }
  return NULL;
}


/**
 * Start the socket for this peer and start the necessary threads to listen for connections
 * and send messages.
 */
int main(int argc, char** argv) {
  // Make sure the arguments include a username
  if(argc != 2 && argc != 4) {
    fprintf(stderr, "Usage: %s <username> [<peer> <port number>]\n", argv[0]);
    exit(1);
  }
  
  // Save the username in a global
  username = argv[1];
  
  // Set up a server socket to accept incoming connections
  unsigned short port = 0; //let cpu choose our port
  int peer_socket_fd = server_socket_open(&port);
  if(peer_socket_fd == -1) {
    fprintf(stderr, "Failed to open socket\n");
    exit(2);
  }

  // Start listening for connections
  if(listen(peer_socket_fd, MAX_CONNECTIONS)) {
    fprintf(stderr, "Listen failed\n");
    exit(2);
  }

  // Set up the user interface. The input_callback function will be called
  // each time the user hits enter to send a message.
  ui_init(input_callback);
  
  // Did the user specify a peer we should connect to?
  if(argc == 4) {
    // Unpack arguments
    char* peer_hostname = argv[2];
    unsigned short peer_port = atoi(argv[3]);
    
    // Connect to another existing peer in the chat network
    int socket_fd = socket_connect(peer_hostname, peer_port);
    if(socket_fd == -1) {
      fprintf(stderr, "Failed to connect to given peer\n");
      exit(2);
    }

    //add the peer that acts as a server to the peer list
    add_peer(socket_fd);

    ui_display("INFO", "You've connected to your host!");
    
    //launch threads for htings
    pthread_t peer_thread;
    pthread_create(&peer_thread, NULL, handle_messages, (void *)(intptr_t) socket_fd);
  }
  
  // Once the UI is running, display your port number so others can connect
  size_t log_size = 100;
  char log[log_size];
  snprintf(log, log_size, "You are connected on port number %d.", port);
  ui_display("INFO", log);

  // Launch thread to accept connections from other peers who may be connecting
  pthread_t acceptThread;
  pthread_create(&acceptThread, NULL, accept_connections, (void *)(intptr_t)peer_socket_fd);
  
  // Run the UI loop. This function only returns once we call ui_exit() somewhere in the program.
  ui_run();
  
  return 0;
}
