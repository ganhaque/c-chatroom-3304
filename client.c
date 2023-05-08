#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

// #define MAX_USERNAME_LENGTH 24
// #define MAX_MESSAGE_LENGTH 1024
#define LENGTH_NAME 31
#define LENGTH_MSG 101
#define LENGTH_SEND 201

// Global
volatile sig_atomic_t is_exit = 0;
char username[LENGTH_NAME] = {};
char password[LENGTH_NAME] = {};
int client_fd = 0;

void ctrl_c_prompt(int sig) {
  is_exit = 1;
}

// decoration
void str_overwrite_stdout() {
  printf("\r%s", "> ");
  fflush(stdout);
}

// Function to handle sending messages to the server
void send_message_handler() {
  char send_message[LENGTH_MSG] = {};
  while(1) {
    str_overwrite_stdout();
    while (fgets(send_message, LENGTH_MSG, stdin) != NULL) {
      // Remove the newline character from the message
      send_message[strcspn(send_message, "\n")] = '\0';
      // ignore empty message
      if (strlen(send_message) == 0) {
        str_overwrite_stdout();
      }
      else {
        break;
      }
    }
    // Send the message to the server if is not command
    if (send_message[0] != '\\') {
      send(client_fd, send_message, LENGTH_MSG, 0);
    }
    else {
      if (send_message[1] == 'q') {
        break;
      }
      printf("invalid command. message cannot start w/ \\\n");
      str_overwrite_stdout();
    }
  }
  ctrl_c_prompt(2);
}

// Function to handle receiving messages from the server
void receive_message_handler() {
  char received_message[LENGTH_SEND] = {};
  while(1) {
    int receive = recv(client_fd, received_message, LENGTH_SEND, 0);
    if (receive > 0) {
      printf("\r%s\n", received_message);
      str_overwrite_stdout();
    }
    else if (receive == 0) { // server closed connection
      is_exit = 1;
      break;
    }
    else { // -1 => error => do nothing
    }
  }
}

int main() {
  // Catching Ctrl + C is buggy since the client is constantly reading
  // the user input
  // It required Ctrl + C then a message for new line before
  // the signal handler takes effect

  // signal(SIGINT, ctrl_c_prompt);

  // Create a socket for the client
  client_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (client_fd == -1) {
    printf("Fail to create a socket.");
    exit(EXIT_FAILURE);
  }

  // Connect to the server
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

  // Prompt for the username
  printf("Please enter your username: ");
  fgets(username, LENGTH_NAME, stdin);
  // remove newline character
  username[strcspn(username, "\n")] = '\0';
  if (strlen(username) < 2 || strlen(username) >= LENGTH_NAME - 1) {
    printf("\nUsername must be more than one and less than %d characters.\n", LENGTH_NAME);
    exit(EXIT_FAILURE);
  }

  // Prompt for the password
  printf("Please enter your password: ");
  fgets(password, LENGTH_NAME, stdin);
  // remove newline character
  password[strcspn(password, "\n")] = '\0';

  send(client_fd, username, LENGTH_NAME, 0);
  send(client_fd, password, LENGTH_NAME, 0);


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
      // No prompt because it is buggy
      // If user entered anything other than yes,
      // the user cannot send any messages after that
      printf("\nBye\n");
      break;
    }
  }

  close(client_fd);
  return 0;
}

