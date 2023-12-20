#include "pdu.hpp"
#include "serialize.hpp"
#include <arpa/inet.h>
#include <curses.h>
#include <fcntl.h>
#include <getopt.h>
#include <mutex>
#include <ncurses.h>
#include <netinet/in.h>
#include <poll.h>
#include <random>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

int client_fd = 0;
std::mutex mutex;
std::vector<std::string> messages;
std::thread listenerThread;
WINDOW *inputWindow;
WINDOW *chatWindow;
bool running = true;

// Signal handlers
void handleSigTerm(int signum);
void handleCtrlC(int signum);
void handleResize(int signum);

// Helpers
void setAddressAndPort(char *arg, char **IP, int *PORT);
void randomNameGenerator(std::string *name);
static void finish(int sig);

// Thread function
void *socketListener(uint8_t *username, int usernameLength,
                     struct sockaddr *serv_addr);

// Start index is the index of the first message to display
void displayMessages(int inputOffsetX, int startIndex);
void displayInput(int inputOffsetX, int cursorIndex, std::string message);

int main(int argc, char *argv[]) {

  char *IP = NULL;
  int PORT = 0;

  // Initialize username to something and overwrite it later if needed
  std::string username = "";
  randomNameGenerator(&username);

  // Define the available options
  const char *shortOptions =
      "u:i:p:h"; // "u:" indicates that -u requires an argument

  // Use getopt to parse the command-line options
  int opt;
  while ((opt = getopt(argc, argv, shortOptions)) != -1) {
    switch (opt) {
    case 'i':
      // optarg contains the argument to the -i option
      setAddressAndPort(optarg, &IP, &PORT);
      break;
    case 'u':
      // optarg contains the argument to the -u option
      username = optarg;
      break;
    case 'h':
      // Print help
      std::cout << " ========== Help ========== " << std::endl;
      std::cout << "Usage: " << argv[0] << " -i <IP>:<PORT> -u <username>\n"
                << std::endl;
      exit(EXIT_SUCCESS);
      break;

    case '?':
      // Handle unknown options or missing arguments
      if (optopt == 'u') {
        std::cerr << "Option -u requires an argument." << std::endl;
      } else if (optopt == 'i') {
        std::cerr << "Option -i requires an argument." << std::endl;
      } else {
        std::cerr << "Unknown option: -" << static_cast<char>(optopt)
                  << std::endl;
      }
      exit(EXIT_FAILURE);

    default:
      // I don't know how this could happen
      break;
    }
  }

  if (PORT == 0) {
    fprintf(stderr,
            "Currently a address & port number has to be supplied. Use -i\n");
    exit(EXIT_FAILURE);
  }

  // Socket handling
  struct sockaddr_in serv_addr;

  if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("\n Socket creation error \n");
    return -1;
  }
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);

  // Convert IPv4 and IPv6 addresses from text to binary
  // form
  if (inet_pton(AF_INET, IP, &serv_addr.sin_addr) <= 0) {
    printf("\nInvalid address/ Address not supported \n");
    return -1;
  }
  /*
      ================== NCURSES ==================
  */
  (void)initscr();      /* initialize the curses library */
  keypad(stdscr, TRUE); /* enable keyboard mapping */
  (void)raw();
  (void)noecho(); /* echo input - in color */

  // Windows init
  int inputOffsetX = 3;
  int height = 3, width = COLS;
  int starty = LINES - height;
  int startx = 0;

  // Chat window
  chatWindow = newwin(starty, width, 0, 0);

  // Input window
  inputWindow = newwin(height, width, starty, startx);
  box(inputWindow, 0, 0);

  refresh();
  wrefresh(inputWindow);
  wrefresh(chatWindow);

  listenerThread =
      std::thread(socketListener, (uint8_t *)username.data(), username.length(),
                  (struct sockaddr *)&serv_addr);

  listenerThread.detach();

  // Move cursor to the input window
  mutex.lock();
  wmove(inputWindow, 1, inputOffsetX);
  wrefresh(inputWindow);
  mutex.unlock();

  std::string message = "";
  int x = 0;

  while (1) {
    int c = getch(); /* refresh, accept single keystroke of input */

    if (c != ERR) {

      mutex.lock();

      if (c == KEY_BACKSPACE || c == 127) {
        if (x > 0) {
          // Delete the last character in the input buffer
          message.pop_back();
          mvwaddch(inputWindow, 1, inputOffsetX + x - 1, ' ');
          wmove(inputWindow, 1, inputOffsetX + x - 1);
          x -= 1;
        }
        displayInput(inputOffsetX, x, message);
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

        msg->msg = (unsigned char *)malloc(message.length());
        memcpy(msg->msg, message.c_str(), message.length());

        uint8_t *serialized = serializeMsgSend(msg);

        if (send(client_fd, serialized,
                 sizeof(struct MSG_SEND_PDU) + message.length(), 0) <= 0) {
          perror("Error sending message to server");
        }

        // Clean up a bit
        message.clear();
        free(msg->msg);
        free(msg);

      } else if (c == KEY_RESIZE) {
        handleResize(SIGWINCH);
      } else if (c == SIGQUIT) {
        handleCtrlC(SIGINT);

      } else {
        message.push_back(c);
        x++;
        displayInput(inputOffsetX, x, message);

        mutex.unlock();
      }
    }
  }

  return 0;
}
void randomNameGenerator(std::string *name) {

  // Initialize random number generator
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> distribution(
      1, 100); // Adjust the range as needed

  // Generate a random number
  int randomNumber = distribution(gen);

  *name = "default-user-" + std::to_string(randomNumber);
}

void setAddressAndPort(char *arg, char **IP, int *PORT) {
  char *token = strtok(arg, ":");

  // First get the address
  if (token == NULL) {
    fprintf(stderr, "Invalid address. Format: 127.0.0.1:<PORT>\n");
    exit(EXIT_FAILURE);
  }
  *IP = token;

  // Now get the port
  token = strtok(NULL, ":");
  if (token == NULL) {
    fprintf(stderr, "Invalid port number. Format: <ADDRESS>:8080\n");
    exit(EXIT_FAILURE);
  }
  *PORT = atoi(token);
}

void *socketListener(uint8_t *username, int usernameLength,
                     struct sockaddr *serv_addr) {
  // Draw window to avoid user thinking the window is frozen incase it takes a
  // while to connect
  mutex.lock();
  box(chatWindow, 0, 0);
  wrefresh(chatWindow);
  mutex.unlock();

  int valread = 0;
  char buffer[1024];
  int inputOffsetX = 3;

  // Connect to the server
  if (connect(client_fd, serv_addr, sizeof(*serv_addr)) < 0) {
    printf("\nConnection Failed \n");
    return NULL;
  }

  // Get welcome message from server
  valread = recv(client_fd, buffer, 1024, 0);
  if (valread < 0) {
    perror("Error on receiving welcome message\n");
    return NULL;
  }
  buffer[valread] = '\0';

  messages.push_back(buffer);
  displayMessages(inputOffsetX, 0);

  // Clear buffer
  memset(buffer, 0, 1024);

  // Send client info to server
  struct NET_JOIN_PDU *netJoinPdu =
      (struct NET_JOIN_PDU *)malloc(sizeof(struct NET_JOIN_PDU));
  netJoinPdu->type = NET_JOIN;
  netJoinPdu->name = username;
  netJoinPdu->name_length = usernameLength;
  uint8_t *serializedNetJoin = serializeNetJoin(netJoinPdu);
  // Send information about client
  if (send(client_fd, serializedNetJoin,
           sizeof(struct NET_JOIN_PDU) + netJoinPdu->name_length, 0) < 0) {
    perror("Error on sending client info\n");
    return NULL;
  }

  // Construct the pollfd struct to check for incoming messages
  struct pollfd pollFd = {0};
  pollFd.fd = client_fd;
  pollFd.events = POLLIN;

  while (running) {

    // Check if there are incoming messages
    valread = poll(&pollFd, 1, 1);
    if (valread == -1) {
      perror("Error in poll");
      finish(SIGINT);
      break;
    }
    if (pollFd.revents == POLLIN) {
      if ((valread = recv(client_fd, buffer, 1024, 0)) > 0) {

        // Buffer[0] contains the type of message
        if (buffer[0] == NET_MSG_SEND) {
          struct MSG_SEND_PDU *client_msg =
              deserializeMsgSend((unsigned char *)buffer);

          std::string message = "";
          message.append((char *)client_msg->msg, client_msg->msg_length);
          messages.insert(messages.begin(), message);

          memset(buffer, 0, 1024);
        } else if (buffer[0] == NET_MSG_BROADCAST) {
          struct MSG_BROADCAST_PDU *client_msg =
              deserializeMsgBroadcast((uint8_t *)buffer);
          std::string message = "";
          message.append((char *)client_msg->time, client_msg->time_length);
          message.append(" ");
          message.append((char *)client_msg->name, client_msg->name_length);
          message.append(": ");
          message.append((char *)client_msg->msg, client_msg->msg_length);
          messages.insert(messages.begin(), message);

          displayMessages(inputOffsetX, 0);

          memset(buffer, 0, 1024);
        } else if (buffer[0] == NET_JOIN) {
          struct NET_JOIN_PDU *client_msg =
              deserializeNetJoin((unsigned char *)buffer);

          std::string message = "";
          message.append((char *)client_msg->name, client_msg->name_length);
          message.append(" has joined");
          messages.insert(messages.begin(), message);

          displayMessages(inputOffsetX, 0);
          memset(buffer, 0, 1024);
        } else if (buffer[0] == NET_LEAVE) {

          struct NET_JOIN_PDU *client_msg =
              deserializeNetJoin((unsigned char *)buffer);

          std::string message = "";
          message.append((char *)client_msg->name, client_msg->name_length);
          message.append(" has left");
          messages.insert(messages.begin(), message);

          displayMessages(inputOffsetX, 0);
          memset(buffer, 0, 1024);
        }
      }
    }
  }
  std::this_thread::sleep_for(std::chrono::seconds(1));
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

// Function to handle SIGINT (Ctrl+C)
void handleCtrlC(int signum) {

  // Wait for listener thread to finish
  if (listenerThread.joinable()) {
    running = false;
    listenerThread.join();
  } else {
    fprintf(stderr, "Thread not joinable, continuing cleanup\n");
  }

  endwin(); // End ncurses mode before exiting
  std::cout << "Received SIGINT. Cleaning up and exiting." << std::endl;
  std::cout << "Done." << std::endl;
  exit(signum);
}

// Function to handle SIGWINCH (Window resize)
void handleResize(int signum) {
  endwin();
  clear();   // Do NOT swap this and
  refresh(); // this... it was a pain to debug

  // Redraw the windows
  int maxY, maxX;
  getmaxyx(stdscr, maxY, maxX);

  // Recreate and redraw the chat window
  wresize(chatWindow, maxY - 3, maxX);
  mvwin(chatWindow, 0, 0);
  wclear(chatWindow);
  box(chatWindow, 0, 0);
  wrefresh(chatWindow);

  // Draw chat messages
  displayMessages(3, 0);

  // Recreate and redraw the input window
  wresize(inputWindow, 3, maxX);
  mvwin(inputWindow, maxY - 3, 0);
  wclear(inputWindow);
  box(inputWindow, 0, 0);
  wmove(inputWindow, 1, 3);
  wrefresh(inputWindow);

  mutex.unlock();
}

// Function to handle SIGTERM (Termination signal)
void handleSigTerm(int signum) {

  // Wait for listener thread to finish
  if (listenerThread.joinable()) {
    running = false;
    listenerThread.join();
  } else {
    fprintf(stderr, "Thread not joinable, continuing cleanup\n");
  }

  endwin(); // End ncurses mode before exiting
  std::cout << "Received SIGTERM. Cleaning up and exiting." << std::endl;
  std::cout << "Done." << std::endl;
  exit(signum);
}

void displayMessages(int inputOffsetX, int startIndex) {

  int maxY, maxX;
  getmaxyx(chatWindow, maxY, maxX);
  // Clear the chat window
  wclear(chatWindow);
  box(chatWindow, 0, 0);
  // Display messages
  int msgIndex = startIndex;
  for (int y = 2; y < maxY; y++) {
    if (messages.size() == msgIndex) {
      break;
    }
    int numLines = messages.at(msgIndex).length() / maxX;
    y += numLines;
    mvwprintw(chatWindow, maxY - y, inputOffsetX, "%s",
              messages.at(msgIndex).c_str());
    msgIndex++;
  }

  // Redraw the windows
  wrefresh(chatWindow);

  // Move cursors to the input window
  wmove(inputWindow, 1, inputOffsetX);
  wrefresh(inputWindow);
}

void displayInput(int inputOffsetX, int cursorIndex, std::string message) {
  int maxY, maxX;
  getmaxyx(inputWindow, maxY, maxX);
  maxX -= inputOffsetX * 2;

  if (message.length() < maxX) {
    mvwprintw(inputWindow, 1, inputOffsetX, "%s", message.c_str());
  } else {
    mvwprintw(inputWindow, 1, inputOffsetX, "%s",
              message.substr(message.length() - maxX).c_str());
  }
  wrefresh(inputWindow);
}
