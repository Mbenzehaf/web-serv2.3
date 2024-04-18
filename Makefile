CC = c++
CFLAGS = -fsanitize=address -g3 #-Wall -Wextra -Werror #-std=c++98
SRC_DIR = .
OBJ_DIR = ./obj
BIN_DIR = ./bin

# List of source files
SRCS = $(wildcard $(SRC_DIR)/tcp_epool.cpp) $(wildcard request/*.cpp) $(wildcard client/*.cpp) $(wildcard response/*.cpp) $(wildcard config/*.cpp) $(wildcard cgi/*.cpp)
# List of object files
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))
# List of executables
EXEC = $(BIN_DIR)/webserver

.PHONY: all clean

all: $(EXEC)

$(EXEC): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
fclean : clean
	rm -rf ${EXEC}
re: fclean all
