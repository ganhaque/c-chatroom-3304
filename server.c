#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

// #define MAX_CLIENTS 10
// #define MAX_MESSAGE_LENGTH 1024
// #define MAX_USERNAME_LENGTH 24

#define LENGTH_NAME 31
#define LENGTH_MSG 101
#define LENGTH_SEND 201

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
// int server_fd = 0;
// int client_fd = 0;
// int client_fds[MAX_CLIENTS] = {};
// int num_clients = 0;
pthread_mutex_t client_fds_mutex = PTHREAD_MUTEX_INITIALIZER;
int server_sockfd = 0, client_sockfd = 0;
ClientList *root, *current;

void catch_ctrl_c_and_exit(int sig) {
  ClientList *tmp;
  char response;
  printf("\nClose down the server? (y/n): ");
  scanf("%c", &response);
  if (response == 'y' || response == 'Y') {
    while (root != NULL) {
      printf("\nClose socketfd: %d\n", root->data);
      close(root->data); // close all socket include server_sockfd
      tmp = root;
      root = root->link;
      free(tmp);
    }
    printf("Goodbye\n");
    exit(EXIT_SUCCESS);
  }
  else if (response == 'n' || response == 'N') {
    // Do nothing and return to the program
  }
  else {
    // Invalid response, print error message and return to the program
    printf("Invalid response. Please enter y or n.\n");
  }
}

void send_to_all_clients(ClientList *np, char tmp_buffer[]) {
  ClientList *tmp = root->link;
  while (tmp != NULL) {
    if (np->data != tmp->data) { // all clients except itself.
      printf("Send to sockfd %d: \"%s\" \n", tmp->data, tmp_buffer);
      send(tmp->data, tmp_buffer, LENGTH_SEND, 0);
    }
    tmp = tmp->link;
  }
}

int check_username_password(char *username, char *password) {
  FILE *fp;
  char line[LENGTH_NAME * 2 + 1];
  char stored_username[LENGTH_NAME], stored_password[LENGTH_NAME];
  int found = 0;

  fp = fopen("userpass.txt", "r");
  if (fp == NULL) {
    printf("Error: cannot open userpass.txt file.\n");
    return -1;
  }

  while (fgets(line, LENGTH_NAME * 2, fp) != NULL) {
    sscanf(line, "%s %s", stored_username, stored_password);
    if (strcmp(username, stored_username) == 0 && strcmp(password, stored_password) == 0) {
      found = 1;
      break;
    }
  }

  fclose(fp);

  return found;
}

void client_handler(void *p_client) {
  int leave_flag = 0;
  char username[LENGTH_NAME] = {};
  char password[LENGTH_NAME] = {};
  char recv_buffer[LENGTH_MSG] = {};
  char send_buffer[LENGTH_SEND] = {};
  ClientList *np = (ClientList *)p_client;

  // Authentication
  while (1) {
    // Username
    if (recv(np->data, username, LENGTH_NAME, 0) <= 0 || strlen(username) < 2 || strlen(username) >= LENGTH_NAME-1) {
      printf("%s didn't input name.\n", np->ip);
      sprintf(send_buffer, "Authentication failed. No username.");
      send(np->data, send_buffer, strlen(send_buffer), 0);
      leave_flag = 1;
      break;
    }

    // Password
    if (recv(np->data, password, LENGTH_NAME, 0) <= 0 || strlen(password) < 2 || strlen(password) >= LENGTH_NAME-1) {
      printf("%s didn't input password.\n", np->ip);
      sprintf(send_buffer, "Authentication failed. No password.");
      send(np->data, send_buffer, strlen(send_buffer), 0);
      leave_flag = 1;
      break;
    }

    // Authenticate against the credentials stored in the file
    if (check_username_password(username, password) != 1) {
      printf("%s failed to authenticate.\n", np->ip);
      printf("attempted username: %s.\n", username);
      printf("attempted password: %s.\n", password);
      sprintf(send_buffer, "Authentication failed.");
      send(np->data, send_buffer, strlen(send_buffer), 0);
      leave_flag = 1;
      break;
    }

    // If authenticated, save the username
    strncpy(np->name, username, LENGTH_NAME);
    printf("%s(%s)(%d) join the chatroom.\n", np->name, np->ip, np->data);
    sprintf(send_buffer, "%s(%s) join the chatroom.", np->name, np->ip);
    send_to_all_clients(np, send_buffer);
    sprintf(send_buffer, "Authentication successful. Welcome to the chatroom!");
    send(np->data, send_buffer, strlen(send_buffer), 0);
    break;
  }

  // Conversation
  while (1) {
    if (leave_flag) {
      break;
    }
    int receive = recv(np->data, recv_buffer, LENGTH_MSG, 0);
    if (receive > 0) {
      if (strlen(recv_buffer) == 0) {
        continue;
      }
      if (recv_buffer[0] != '\\') { // message
        printf("Received: %s\n", recv_buffer); // Debugging line
        sprintf(send_buffer, "%s (%s)：%s", np->name, np->ip, recv_buffer);
      }
      else { // command
      }
    }
    else if (receive == 0) {
      printf("%s(%s)(%d) leave the chatroom.\n", np->name, np->ip, np->data);
      sprintf(send_buffer, "%s(%s) leave the chatroom.", np->name, np->ip);
      leave_flag = 1;
    }
    else {
      printf("Fatal Error: -1\n");
      leave_flag = 1;
    }
    send_to_all_clients(np, send_buffer);
  }

  // Remove Node
  close(np->data);
  if (np == current) { // remove an edge node
    current = np->prev;
    current->link = NULL;
  }
  else { // remove a middle node
    np->prev->link = np->link;
    np->link->prev = np->prev;
  }
  free(np);
}

int main() {
  signal(SIGINT, catch_ctrl_c_and_exit);

  // Create socket
  server_sockfd = socket(AF_INET , SOCK_STREAM , 0);
  if (server_sockfd == -1) {
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
  if(bind(server_sockfd, (struct sockaddr *)&server_info, s_addrlen) < 0) {
    perror("Error: bind failed");
    exit(EXIT_FAILURE);
  }
  listen(server_sockfd, 5);

  // Print Server IP
  getsockname(server_sockfd, (struct sockaddr*) &server_info, (socklen_t*) &s_addrlen);
  printf("Start Server on: %s:%d\n", inet_ntoa(server_info.sin_addr), ntohs(server_info.sin_port));

  // Initial linked list for clients
  root = newNode(server_sockfd, inet_ntoa(server_info.sin_addr));
  current = root;

  while (1) {
    client_sockfd = accept(server_sockfd, (struct sockaddr*) &client_info, (socklen_t*) &c_addrlen);

    // Print Client IP
    getpeername(client_sockfd, (struct sockaddr*) &client_info, (socklen_t*) &c_addrlen);
    printf("Client %s:%d come in.\n", inet_ntoa(client_info.sin_addr), ntohs(client_info.sin_port));

    // Append linked list for clients
    ClientList *client = newNode(client_sockfd, inet_ntoa(client_info.sin_addr));
    client->prev = current;
    current->link = client;
    current = client;

    pthread_t id;
    if (pthread_create(&id, NULL, (void *)client_handler, (void *)client) != 0) {
      perror("Create pthread error!\n");
      exit(EXIT_FAILURE);
    }
  }

  return 0;
}

