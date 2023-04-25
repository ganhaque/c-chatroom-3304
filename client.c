#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define MAX_USERNAME_LENGTH 24
#define MAX_MESSAGE_LENGTH 1024

// Global
volatile sig_atomic_t is_exit = 0;
char username[MAX_USERNAME_LENGTH] = {};
int client_fd = 0;

void catch_ctrl_c_and_exit(int sig) {
  is_exit = 1;
}

void str_overwrite_stdout() {
  printf("\r%s", "> ");
  fflush(stdout);
}

// Function to handle sending messages to the server
void send_message_handler() {
  char message[MAX_MESSAGE_LENGTH] = {};
  while(1) {
    str_overwrite_stdout();
    while (fgets(message, MAX_MESSAGE_LENGTH, stdin) != NULL) {
      // Remove the newline character from the message
      message[strcspn(message, "\n")] = '\0';
      // ignore empty message
      if (strlen(message) == 0) {
        str_overwrite_stdout();
      }
      else {
        break;
      }
    }
    // Send the message to the server
    send(client_fd, message, MAX_MESSAGE_LENGTH, 0);
    if (strcmp(message, "exit") == 0) {
      break;
    }
  }
  catch_ctrl_c_and_exit(2);
}

// Function to handle receiving messages from the server
void receive_message_handler() {
  char received_message[MAX_MESSAGE_LENGTH] = {};
  while(1) {
    int receive = recv(client_fd, received_message, MAX_MESSAGE_LENGTH, 0);
    if (receive > 0) {
      printf("\r%s\n", received_message);
      str_overwrite_stdout();
    }
    else if (receive == 0) { // server closed connection
      break;
    }
    else { // -1 => error => do nothing
    }
  }
}

int main() {
  signal(SIGINT, catch_ctrl_c_and_exit);

  printf("Please enter your name: ");
  fgets(username, MAX_USERNAME_LENGTH, stdin);
  // Remove the newline character from the username
  username[strcspn(username, "\n")] = '\0';
  if (strlen(username) < 2 || strlen(username) >= MAX_USERNAME_LENGTH - 1) {
    printf("\nUsername must be more than one and less than %d characters.\n", MAX_USERNAME_LENGTH);
    exit(EXIT_FAILURE);
  }

  // Create a socket for the client
  client_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (client_fd == -1) {
    printf("Fail to create a socket.");
    exit(EXIT_FAILURE);
  }

  // Connect to the server
  // struct sockaddr_in server_address;
  // server_address.sin_family = AF_INET;
  // server_address.sin_port = htons(8080);
  // inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);
  struct sockaddr_in server_info, client_info;
  int server_address_length = sizeof(server_info);
  int client_address_length = sizeof(client_info);
  memset(&server_info, 0, server_address_length);
  memset(&client_info, 0, client_address_length);
  server_info.sin_family = PF_INET;
  server_info.sin_addr.s_addr = inet_addr("127.0.0.1");
  server_info.sin_port = htons(8888);

  // Connect to the server
  if (connect(client_fd, (struct sockaddr *)&server_info, server_address_length) < 0) {
    printf("Error: connection failed");
    exit(EXIT_FAILURE);
  }

  // Names
  getsockname(client_fd, (struct sockaddr*) &client_info, (socklen_t*) &client_address_length);
  getpeername(client_fd, (struct sockaddr*) &server_info, (socklen_t*) &server_address_length);
  printf("Connect to Server: %s:%d\n", inet_ntoa(server_info.sin_addr), ntohs(server_info.sin_port));
  printf("You are: %s:%d\n", inet_ntoa(client_info.sin_addr), ntohs(client_info.sin_port));

  send(client_fd, username, MAX_USERNAME_LENGTH, 0);

  // Create threads for sending and receiving messages
  pthread_t send_thread;
  if (pthread_create(&send_thread, NULL, (void *) send_message_handler, NULL) != 0) {
    printf("Error: could not create send thread");
    exit(EXIT_FAILURE);
  }
  pthread_t receive_thread;
  if (pthread_create(&receive_thread, NULL, (void *) receive_message_handler, NULL) != 0) {
    printf("Error: could not create receive thread");
    exit(EXIT_FAILURE);
  }

  while (1) {
    if(is_exit) {
      printf("\nBye\n");
      break;
    }
  }

  close(client_fd);
  return 0;
}

