#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
// #include <signal.h>

#define MAX_CLIENTS 10
#define MAX_USERNAME_LENGTH 24
#define MAX_MESSAGE_LENGTH 1024

typedef struct ClientNode {
  int data;
  struct ClientNode* prev;
  struct ClientNode* link;
  char ip[16];
  char name[31];
} ClientList;

// initialize linked list
ClientList *newNode(int sockfd, char* ip) {
  ClientList *np = (ClientList *)malloc( sizeof(ClientList) );
  np->data = sockfd;
  np->prev = NULL;
  np->link = NULL;
  strncpy(np->ip, ip, 16);
  strncpy(np->name, "NULL", 5);
  return np;
}

// Global
int client_fds[MAX_CLIENTS] = {0};
int num_clients = 0;
pthread_mutex_t client_fds_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to broadcast message to all connected clients
void broadcast_message(char *message, int sender_fd) {
  pthread_mutex_lock(&client_fds_mutex);
  for (int i = 0; i < MAX_CLIENTS; i++) {
    int client_fd = client_fds[i];
    if (client_fd != 0 && client_fd != sender_fd) {
      send(client_fd, message, strlen(message), 0);
    }
  }
  pthread_mutex_unlock(&client_fds_mutex);
}

// Function to handle incoming messages from a client
void *client_handler(void *arg) {
  int client_fd = *(int *)arg;
  char username[MAX_USERNAME_LENGTH] = {};
  int username_length = recv(client_fd, username, MAX_USERNAME_LENGTH, 0);
  if (username_length <= 0) {
    printf("Error: failed to get client username\n");
    return NULL;
  }
  printf("Client %s connected\n", username);
  char connected_message[MAX_MESSAGE_LENGTH] = {};
  snprintf(connected_message, MAX_MESSAGE_LENGTH, "%s connected to the server", username);
  broadcast_message(connected_message, client_fd);

  char received_message[MAX_MESSAGE_LENGTH] = {};
  while (1) {
    int receive = recv(client_fd, received_message, MAX_MESSAGE_LENGTH, 0);
    if (receive <= 0) {
      printf("Client %s disconnected\n", username);
      char disconnected_message[MAX_MESSAGE_LENGTH] = {};
      snprintf(disconnected_message, MAX_MESSAGE_LENGTH, "%s disconnected from the server", username);
      broadcast_message(disconnected_message, client_fd);
      break;
    }
    printf("Received message from %s: %s\n", username, received_message);
    char message_to_broadcast[MAX_MESSAGE_LENGTH + MAX_USERNAME_LENGTH] = {};
    snprintf(message_to_broadcast, MAX_MESSAGE_LENGTH + MAX_USERNAME_LENGTH, "%s: %s", username, received_message);
    broadcast_message(message_to_broadcast, client_fd);
    memset(received_message, 0, MAX_MESSAGE_LENGTH);
  }
  pthread_mutex_lock(&client_fds_mutex);
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (client_fds[i] == client_fd) {
      client_fds[i] = 0;
      break;
    }
  }
  num_clients--;
  pthread_mutex_unlock(&client_fds_mutex);
  close(client_fd);
  return NULL;
}

int main() {
  // Create a socket for the server
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    printf("Fail to create a socket.");
    exit(EXIT_FAILURE);
  }

  // Socket information
  struct sockaddr_in server_info, client_info;
  int s_addrlen = sizeof(server_info);
  int c_addrlen = sizeof(client_info);
  memset(&server_info, 0, s_addrlen);
  memset(&client_info, 0, c_addrlen);
  server_info.sin_family = PF_INET;
  server_info.sin_addr.s_addr = INADDR_ANY;
  server_info.sin_port = htons(8888);

  // Bind and Listen
  int bind_result = bind(server_fd, (struct sockaddr *)&server_info, s_addrlen);
  if (bind_result < 0) {
    perror("Error: bind failed");
    exit(EXIT_FAILURE);
  }
  listen(server_fd, 5);




  // Bind the socket to a specific port
  struct sockaddr_in address;
  // set struct to 0 bytes (to 0 out the unused fields)
  memset(&address, 0, sizeof(struct sockaddr_in));
  // Set 3 address structure members
  address.sin_family = AF_INET; // addr type is Internet (IPv4)
  address.sin_addr.s_addr = INADDR_ANY; // bind to any local address
  address.sin_port = htons(8080);

  int addrlen = sizeof(address);


  // Listen for incoming connections
  listen(server_fd, 3);

  // Accept incoming connections
  int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);

  // Send data to the client
  // char* message = "Welcome to the server\n";
  // send(new_socket, message, strlen(message), 0);

  while (1) {
    // Receive data from the client
    char buffer[1024] = {0};
    int valread = recv(new_socket, buffer, 1024, 0);
    if (valread == 0) {
      printf("Client disconnected\n");
      break;
    }
    printf("Client: %s", buffer);

  }


  return 0;
}
