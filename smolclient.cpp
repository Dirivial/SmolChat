#include "pdu.hpp"
#include "serialize.hpp"
#include <arpa/inet.h>
#include <curses.h>
#include <fcntl.h>
#include <mutex>
#include <ncurses.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

static void finish(int sig);
void *socketListener(int socket, struct sockaddr *serv_addr, WINDOW *window,
                     WINDOW *inputWindow);

int client_fd = 0;
int *y = 0;
std::mutex mutex;

int main(int argc, char const *argv[]) {

  // Check for the correct number of arguments
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // Ncurses initialization
  (void)signal(SIGINT, finish); /* arrange interrupts to terminate */

  (void)initscr();      /* initialize the curses library */
  keypad(stdscr, TRUE); /* enable keyboard mapping */
  (void)cbreak();       /* take input chars one at a time, no wait for \n */
  (void)noecho();       /* echo input - in color */

  // Windows init
  int inputOffsetX = 3;
  int height = 3, width = COLS;
  int starty = LINES - height;
  int startx = 0;

  // Chat window
  WINDOW *chatWindow = newwin(starty, width, 0, 0);

  // Input window
  WINDOW *inputWindow = newwin(height, width, starty, startx);
  box(inputWindow, 0, 0);

  refresh();
  wrefresh(inputWindow);
  wrefresh(chatWindow);
  char *IP = (char *)argv[1];
  int PORT = atoi(argv[2]);

  int status;
  struct sockaddr_in serv_addr;

  if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("\n Socket creation error \n");
    return -1;
  }
  // Set socket to nonblocking
  /*
  int flags = fcntl(client_fd, F_GETFL, 0);
  flags = flags & ~O_NONBLOCK;
  int success = fcntl(client_fd, F_SETFL, flags);

  if (success == -1) {
    perror("Error setting socket to blocking");
    return -1;
  }
  */
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);

  // Convert IPv4 and IPv6 addresses from text to binary
  // form
  if (inet_pton(AF_INET, IP, &serv_addr.sin_addr) <= 0) {
    printf("\nInvalid address/ Address not supported \n");
    return -1;
  }

  std::string message = "";
  int x = 0;
  std::thread listenerThread =
      std::thread(socketListener, client_fd, (struct sockaddr *)&serv_addr,
                  chatWindow, inputWindow);

  listenerThread.detach();
  // Move cursor to the input window
  mutex.lock();
  wmove(inputWindow, 1, inputOffsetX);
  wrefresh(inputWindow);
  mutex.unlock();

  for (;;) {
    int c = getch(); /* refresh, accept single keystroke of input */

    if (c != ERR) {

      mutex.lock();
      if (c == KEY_BACKSPACE || c == 127) {
        // Delete the last character in the input buffer
        if (x > 0) {
          message.pop_back();
          mvwaddch(inputWindow, 1, inputOffsetX + x - 1, ' ');
          wmove(inputWindow, 1, inputOffsetX + x - 1);
          x -= 1;
        }

        wrefresh(inputWindow);
        mutex.unlock();
      } else if (c == '\n' || c == '\r') {

        x = 0;
        wclear(inputWindow);
        box(inputWindow, 0, 0);
        wmove(inputWindow, 1, inputOffsetX);

        wrefresh(inputWindow);
        mutex.unlock();

        // Send the message to the server
        struct MSG_SEND_PDU *msg =
            (struct MSG_SEND_PDU *)malloc(sizeof(struct MSG_SEND_PDU));
        msg->type = NET_MSG_SEND;
        msg->msg_length = message.length();

        msg->msg = (unsigned char *)malloc(sizeof(char) * message.length());
        memcpy(msg->msg, message.c_str(), message.length());

        unsigned char *serialized = serializeMsgSend(msg);

        if (send(client_fd, serialized, 1 + msg->msg_length + message.length(),
                 0) <= 0) {
          perror("Error sending message to server");
        }

        // Clean up a bit
        message.clear();
        free(msg->msg);
        free(msg);

      } else {
        message.push_back(c);
        mvwaddch(inputWindow, 1, inputOffsetX + x, c);
        x++;

        wrefresh(inputWindow);
        mutex.unlock();
      }
    } else {
      // Check if data has been fetched
    }
  }

  return 0;
}

void *socketListener(int socket, struct sockaddr *serv_addr, WINDOW *chatWindow,
                     WINDOW *inputWindow) {
  // Draw window to avoid user thinking the window is frozen
  mutex.lock();
  box(chatWindow, 0, 0);
  wrefresh(chatWindow);
  mutex.unlock();

  int valread = 0;
  char buffer[1024];
  int lineHeight = 1;
  int inputOffsetX = 3;

  // Connect to the server
  if (connect(client_fd, serv_addr, sizeof(*serv_addr)) < 0) {
    printf("\nConnection Failed \n");
    return NULL;
  }

  // Get welcom message from server
  valread = recv(client_fd, buffer, 1024, 0);
  buffer[valread] = '\0';

  // Print welcome message to the chat window
  mutex.lock();
  mvwprintw(chatWindow, lineHeight++, 3, "%s\n", buffer);
  box(chatWindow, 0, 0);
  wrefresh(chatWindow);
  wmove(inputWindow, 1, inputOffsetX);
  wrefresh(inputWindow);
  mutex.unlock();

  // Clear buffer
  memset(buffer, 0, 1024);

  // Construct the pollfd struct to check for incoming messages
  struct pollfd pollFd = {0};
  pollFd.fd = client_fd;
  pollFd.events = POLLIN;

  while (1) {

    // Check if there are incoming messages
    valread = poll(&pollFd, 1, 1);
    if (valread == -1) {
      perror("Error in poll");
      finish(SIGINT);
      break;
    }
    if (pollFd.revents == POLLIN) {
      if ((valread = recv(client_fd, buffer, 1024, 0)) > 0) {

        mutex.lock();

        mvwprintw(chatWindow, lineHeight++, inputOffsetX, "%s\n", &buffer);
        box(chatWindow, 0, 0);
        wrefresh(chatWindow);
        wmove(inputWindow, 1, inputOffsetX);
        wrefresh(inputWindow);

        mutex.unlock();

        memset(buffer, 0, 1024);
      }
    }
  }
  return NULL;
}

static void finish(int sig) {
  endwin();

  /* do your non-curses wrapup here */

  // This seems to make shutdown more graceful
  if (shutdown(client_fd, 0) == -1) {
    perror("Error shutting down client socket");
  }
  if (close(client_fd) == -1) {
    perror("Error closing client socket");
  }
  exit(0);
}
