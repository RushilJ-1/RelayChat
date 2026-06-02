#include "client_handler.h"
#include "room.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define BUFFER_SIZE 512
static void trim_newline(char *text) {
    size_t len = strlen(text);
    while (len > 0 && (text[len - 1] == '\n' || text[len - 1] == '\r')) {
        text[len - 1] = '\0';
        len--;
    }
}
static int send_all(int socket, const char *message) {
    size_t total = 0;
    size_t len = strlen(message);

    while (total < len) {
        ssize_t sent = send(socket, message + total, len - total, 0);
        if (sent <= 0) {
            return -1;
        }
        total += (size_t)sent;
    }
    return 0;
}

static int recv_line(int socket, char *buffer, size_t size) {
    size_t index = 0;
    while (index + 1 < size) {
        char ch;
        ssize_t received = recv(socket, &ch, 1, 0);
        if (received <= 0) {
            return -1;
        }
        buffer[index++] = ch;
        if (ch == '\n') {
            break;
        }
    }
    buffer[index] = '\0';
    trim_newline(buffer);
    return 0;
}

static int read_join_request(Client *client) {
    char error[128];
    while (1) {
        if (send_all(client->socket, "Enter Username: ") < 0) {
            return -1;
        }
        if (recv_line(client->socket, client->username, sizeof(client->username)) < 0) {
            return -1;
        }
        if (client->username[0] == '\0') {
            send_all(client->socket, "Username is required.\n");
            continue;
        }
        break;
    }

    while (1) {
        if (send_all(client->socket, "Enter Room Code: ") < 0) {
            return -1;
        }
        if (recv_line(client->socket, client->roomCode, sizeof(client->roomCode)) < 0) {
            return -1;
        }
        if (client->roomCode[0] == '\0') {
            send_all(client->socket, "Room code is required.\n");
            continue;
        }
        if (room_join(client->roomCode, client->socket, client->username, error, sizeof(error)) == 0) {
            send_all(client->socket, "Joined room.\nType /help for commands.\n");
            return 0;
        }
        send_all(client->socket, error);
    }
}

void *handle_client(void *arg) {
    Client *client = (Client *)arg;
    char buffer[BUFFER_SIZE];
    char senderMessage[BUFFER_SIZE + 8];
    char othersMessage[BUFFER_SIZE + MAX_USERNAME_LEN + 4];
    char disconnectMessage[MAX_USERNAME_LEN + 20];
    if (read_join_request(client) < 0) {
        close(client->socket);
        free(client);
        return NULL;
    }

    printf("%s joined room %s.\n", client->username, client->roomCode);
    while (1) {
        ssize_t received = recv(client->socket, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) {
            break;
        }
        buffer[received] = '\0';
        trim_newline(buffer);
        if (strcmp(buffer, "/quit") == 0) {
            send_all(client->socket, "Goodbye.\n");
            break;
        }
        if (strcmp(buffer, "/help") == 0) {
            send_all(client->socket, "Available commands:\n/help - Show commands\n/quit - Leave chat\n");
            continue;
        }
        snprintf(senderMessage, sizeof(senderMessage), "You: %s\n", buffer);
        snprintf(othersMessage, sizeof(othersMessage), "%s: %s\n", client->username, buffer);
        if (room_broadcast(client->roomCode, client->socket, senderMessage, othersMessage) <= 0) {
            break;
        }
    }

    int roomDestroyed;
    room_leave(client->roomCode, client->socket, &roomDestroyed);
    if (!roomDestroyed) {
        snprintf(disconnectMessage, sizeof(disconnectMessage), "%s disconnected.\n", client->username);
        room_broadcast(client->roomCode, -1, disconnectMessage, disconnectMessage);
    }
    printf("%s left room %s.\n", client->username, client->roomCode);
    close(client->socket);
    free(client);
    return NULL;
}
