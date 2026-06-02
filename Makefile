CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -pthread

SERVER_SRC = server/main.c server/room.c server/client_handler.c
CLIENT_SRC = client/client.c

SERVER_BIN = server/server
CLIENT_BIN = client/client

.PHONY: all clean

all: $(SERVER_BIN) $(CLIENT_BIN)

$(SERVER_BIN): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $@ $(SERVER_SRC)

$(CLIENT_BIN): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $@ $(CLIENT_SRC)

clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN)
