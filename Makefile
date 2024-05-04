CC = gcc
CFLAGS = -lm -Wall -Wextra

CLIENT_SRC = client.c utils.c  
SERVER_SRC = server.c utils.c  

CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
SERVER_OBJ = $(SERVER_SRC:.c=.o)

BIN_DIR = ./bin

all: directories client server

directories: 
	mkdir -p $(BIN_DIR)

client: $(CLIENT_OBJ)
	$(CC) -o $(BIN_DIR)/client $(CLIENT_OBJ) $(CFLAGS)

server: $(SERVER_OBJ)
	$(CC) -o $(BIN_DIR)/server $(SERVER_OBJ) $(CFLAGS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm - $(CLIENT_OBJ) $(SERVER_OBJ) $(BIN_DIR)/client $(BIN_DIR)/server
