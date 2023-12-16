# Compiler and flags
CXX = c++
CXXFLAGS = -std=c++11 -Wall -lncurses

# Source files for program1
SRC_CLIENT = smolclient.cpp serialize.cpp

# Object files for program1
OBJ_FILES_CLIENT = $(SRC_CLIENT:.cpp=.o)

# Target executable for program1
CLIENT_TARGET = SmolClient

# Source files for program2
SRC_SERVER = smolserver.cpp serialize.cpp

# Object files for program2
OBJ_FILES_SERVER = $(SRC_SERVER:.cpp=.o)

# Target executable for program2
SERVER_TARGET = SmolServer

# Default target
all: $(CLIENT_TARGET) $(SERVER_TARGET)

# Linking rule for program1
$(CLIENT_TARGET): $(OBJ_FILES_CLIENT)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Linking rule for program2
$(SERVER_TARGET): $(OBJ_FILES_SERVER)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compilation rule for object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Clean rule to remove generated files
clean:
	rm -f $(OBJ_FILES_CLIENT) $(CLIENT_TARGET) $(OBJ_FILES_SERVER) $(SERVER_TARGET)
