#include "pdu.hpp"
#include "serialize.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char const *argv[]) {

  // Check for the correct number of arguments
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int PORT = atoi(argv[1]);
  int maxClients = 30;
  int clientSockets[maxClients];
  int valread;

  int serverSocket;
  struct sockaddr_in address;
  int opt = 1;
  socklen_t addrlen = sizeof(address);
  char buffer[1025] = {0};

  // Initialize client sockets to 0
  for (int i = 0; i < maxClients; i++) {
    clientSockets[i] = 0;
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

      int socket = clientSockets[i];

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
        if (clientSockets[i] == 0) {
          clientSockets[i] = clientSocket;
          fprintf(stdout, "Added client to active connections\n");
          break;
        }
      }
    }

    // Check data from active connections and handle accordingly
    for (int i = 0; i < maxClients; i++) {

      int clientSocket = clientSockets[i];

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
          clientSockets[i] = 0;
        }

        else if (valread > 0) {
          uint8_t type = buffer[0];
          fprintf(stdout, "Received %d bytes containing a PDU of type %d\n",
                  valread, type);
          switch (type) {
          case NET_JOIN:
            fprintf(stdout, "Client %d sent a NET_JOIN PDU\n", i);
            break;
          case NET_MSG_SEND:
            fprintf(stdout, "Client %d sent a MSG_SEND PDU\n", i);
            // Set the string terminating NULL byte on the end of the data read
            buffer[valread] = '\0';
            // Send message to all other clients
            for (int j = 0; j < maxClients; j++) {
              int aClient = clientSockets[j];
              if (aClient != 0) {
                if (send(aClient, buffer, strlen(buffer), 0) < 0) {
                  perror("Error sending message to client");
                }
              }
            }
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
