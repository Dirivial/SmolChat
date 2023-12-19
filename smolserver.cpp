#include "pdu.hpp"
#include "serialize.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <netinet/in.h>
#include <pthread.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

struct Client {
  int socket;
  struct sockaddr_in address;
  socklen_t addrlen;
  int nameLength;
  uint8_t *name;
};

void handleMsgSend(uint8_t *buffer);
void handleNetJoin(uint8_t *buffer, struct Client *clients, int maxClients,
                   int clientIndex);
void handleMsgBroadcast(uint8_t *buffer, struct Client *clients,
                        int clientIndex, int maxClients);

int main(int argc, char const *argv[]) {

  // Check for the correct number of arguments
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int PORT = atoi(argv[1]);
  int maxClients = 30;
  struct Client *clients =
      (struct Client *)std::malloc(sizeof(struct Client) * maxClients);
  int valread;

  int serverSocket;
  struct sockaddr_in address;
  int opt = 1;
  socklen_t addrlen = sizeof(address);
  char buffer[1025] = {0};

  // Initialize client sockets to 0
  for (int i = 0; i < maxClients; i++) {
    clients[i].socket = 0;
  }

  // Creating socket file descriptor
  if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  // Set socket options, to enable local address reuse
  if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  // Construct server address structure
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT); // Host to network short

  // Bind socket to the port provided by user
  if (bind(serverSocket, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }
  // Start listening for connections
  if (listen(serverSocket, 3) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  fprintf(stdout, "Listening on port %d\n", PORT);

  fd_set readSet;
  int maxFd = serverSocket;

  while (1) {
    // Clear socket set
    FD_ZERO(&readSet);

    // Add server socket to the set
    FD_SET(serverSocket, &readSet);
    maxFd = serverSocket;

    // Add child sockets to the set
    for (int i = 0; i < maxClients; i++) {

      int socket = clients[i].socket;

      // If the socket is valid, add it to the set
      if (socket > 0) {
        FD_SET(socket, &readSet);
      }

      // Update the maximum file descriptor
      if (socket > maxFd) {
        maxFd = socket;
      }
    }

    if (select(maxFd + 1, &readSet, NULL, NULL, NULL) == -1) {
      perror("Error in select");
      break;
    }

    // Check if there is a new incoming connection
    if (FD_ISSET(serverSocket, &readSet)) {

      // Accept the new connection
      int clientSocket = accept(serverSocket, NULL, NULL);

      if (clientSocket == -1) {
        perror("Error accepting client connection");
        continue;
      }

      fprintf(stdout,
              "Connection accepted from client, sending a welcome message... ");

      std::string wm = "Welcome to the chat server!";
      // Send welcome message to client
      if (send(clientSocket, wm.c_str(), wm.length(), 0) != wm.length()) {
        perror("Error sending welcome message to client");
      }

      fprintf(stdout, "done.\n");

      // Add the new client socket to the set of active connections
      for (int i = 0; i < maxClients; i++) {

        // If position is empty, insert the socket
        if (clients[i].socket == 0) {
          clients[i].socket = clientSocket;
          fprintf(stdout, "Added client to active connections\n");
          break;
        }
      }
    }

    // Check data from active connections and handle accordingly
    for (int i = 0; i < maxClients; i++) {

      int clientSocket = clients[i].socket;

      if (FD_ISSET(clientSocket, &readSet)) {

        // Check if it was for closing, and also read the incoming message
        if ((valread = read(clientSocket, &buffer, 1024)) == 0) {

          // Somebody disconnected, get their details and print

          getpeername(clientSocket, (struct sockaddr *)&address,
                      (socklen_t *)&addrlen);

          fprintf(stdout, "Client disconnected, ip %s, port %d\n",
                  inet_ntoa(address.sin_addr), ntohs(address.sin_port));

          // Close the socket and mark as 0 in list for reuse
          close(clientSocket);
          clients[i].socket = 0;

          // Inform other clients that a client has disconnected
          // (We use the same struct for NET_JOIN and NET_LEAVE atm.)
          struct NET_JOIN_PDU *netLeave =
              (struct NET_JOIN_PDU *)malloc(sizeof(struct NET_JOIN_PDU));
          netLeave->type = NET_LEAVE;
          netLeave->name_length = clients[i].nameLength;
          netLeave->name = (uint8_t *)malloc(clients[i].nameLength);
          memcpy(netLeave->name, clients[i].name, clients[i].nameLength);

          fprintf(stdout, "Sending NET_LEAVE_PDU to clients for client %.*s\n",
                  netLeave->name_length, netLeave->name);

          for (int i = 0; i < maxClients; i++) {
            if (clients[i].socket != 0) {
              if (send(clients[i].socket, serializeNetJoin(netLeave),
                       sizeof(struct NET_JOIN_PDU) + netLeave->name_length,
                       0) < 0) {
                perror("Error sending NET_LEAVE_PDU to client");
              }
            }
          }
          memset(buffer, 0, sizeof(buffer));
          free(netLeave->name);
          free(netLeave);
        }

        else if (valread > 0) {

          fprintf(stdout, "%d bytes received -> ", valread);
          // Handle message depending on type
          uint8_t type = buffer[0];
          switch (type) {
          case NET_JOIN:
            fprintf(stdout, "Client %d sent a NET_JOIN PDU\n", i);

            handleNetJoin((uint8_t *)buffer, clients, maxClients, i);

            memset(buffer, 0, sizeof(buffer));
            break;
          case NET_MSG_SEND:
            fprintf(stdout, "Client %d sent a MSG_SEND\n", i);

            // handleMsgSend((uint8_t *)buffer);
            handleMsgBroadcast((uint8_t *)buffer, clients, i, maxClients);

            memset(buffer, 0, sizeof(buffer));

            break;
          case NET_ALIVE:
            fprintf(stdout, "Client %d sent a NET_ALIVE PDU\n", i);
            break;
          default:
            fprintf(stdout, "Client %d sent an unknown PDU\n", i);
            break;
          }
        } else {
          perror("Error reading from client");
        }
      }
    }
  }

  // closing the listening socket
  close(serverSocket);
  return 0;
}

void handleMsgSend(uint8_t *buffer) {

  struct MSG_SEND_PDU *client_msg = deserializeMsgSend((uint8_t *)buffer);
  fprintf(stdout, " :: '%s'\n", client_msg->msg);
}

void handleNetJoin(uint8_t *buffer, struct Client *clients, int maxClients,
                   int clientIndex) {
  struct NET_JOIN_PDU *netJoin = deserializeNetJoin(buffer);

  clients[clientIndex].name = (uint8_t *)malloc(netJoin->name_length);
  memcpy(clients[clientIndex].name, netJoin->name, netJoin->name_length);
  clients[clientIndex].nameLength = netJoin->name_length;

  fprintf(stderr, "Client no. %d with name %s has joined\n", clientIndex,
          clients[clientIndex].name);

  // Inform other clients that a new client has joined
  for (int i = 0; i < maxClients; i++) {
    if (i != clientIndex) {
      if (clients[i].socket != 0 && clients[i].socket != 0) {
        if (send(clients[i].socket, buffer, sizeof(struct NET_JOIN_PDU), 0) <
            0) {
          perror("Error sending NET_JOIN_PDU to client");
        }
      }
    }
  }
}

void handleMsgBroadcast(uint8_t *buffer, struct Client *clients,
                        int clientIndex, int maxClients) {
  struct MSG_SEND_PDU *message = deserializeMsgSend(buffer);

  // Incredibly annoying way to get the time in HH:MM format
  std::time_t t = std::time(nullptr);
  uint8_t *time;
  std::ostringstream oss;
  oss << std::put_time(std::localtime(&t), "%H:%M");
  time = (uint8_t *)oss.str().data();

  int timeLength = (int)std::strlen((char *)time);

  struct MSG_BROADCAST_PDU *newMessage =
      (struct MSG_BROADCAST_PDU *)malloc(sizeof(MSG_BROADCAST_PDU));

  newMessage->type = NET_MSG_BROADCAST;
  newMessage->msg_length = message->msg_length;
  newMessage->msg = message->msg;
  newMessage->name_length = clients[clientIndex].nameLength;
  newMessage->name = clients[clientIndex].name;
  newMessage->time = (uint8_t *)malloc(timeLength);
  memcpy(newMessage->time, time, timeLength);
  newMessage->time_length = timeLength;

  uint8_t *serializedMessage = serializeMsgBroadcast(newMessage);
  int sizeOfMessage = sizeof(struct MSG_BROADCAST_PDU) +
                      newMessage->msg_length + newMessage->name_length +
                      newMessage->time_length;

  // Send message to all clients
  for (int j = 0; j < maxClients; j++) {
    int aClient = clients[j].socket;
    if (aClient != 0) {
      if (send(aClient, serializedMessage, sizeOfMessage, 0) < 0) {
        perror("Error sending message to client");
      }
    }
  }
}
