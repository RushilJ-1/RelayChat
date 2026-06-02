#include "room.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

pthread_mutex_t roomLock;
static Room rooms[MAX_ROOMS];
static int roomCount = 0;
static int find_room_index(const char *roomCode) {
    for (int i = 0; i < roomCount; i++) {
        if (strcmp(rooms[i].roomCode, roomCode) == 0) {
            return i;
        }
    }
    return -1;
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

void rooms_init(void) {
    pthread_mutex_init(&roomLock, NULL);
}

void rooms_destroy(void) {
    for (int i = 0; i < roomCount; i++) {
        free(rooms[i].clients);
        rooms[i].clients = NULL;
        rooms[i].clientCount = 0;
        rooms[i].clientCapacity = 0;
    }
    roomCount = 0;
    pthread_mutex_destroy(&roomLock);
}

int room_join(const char *roomCode, int clientSocket, const char *username, char *error, int errorSize) {
    pthread_mutex_lock(&roomLock);
    int index = find_room_index(roomCode);
    if (index == -1) {
        if (roomCount >= MAX_ROOMS) {
            snprintf(error, errorSize, "Server room limit reached.\n");
            pthread_mutex_unlock(&roomLock);
            return -1;
        }
        index = roomCount++;
        memset(&rooms[index], 0, sizeof(Room));
        strncpy(rooms[index].roomCode, roomCode, MAX_ROOM_CODE_LEN - 1);
        rooms[index].clientCapacity = INITIAL_ROOM_CAPACITY;
        rooms[index].clients = calloc((size_t)rooms[index].clientCapacity, sizeof(RoomClient));
        if (rooms[index].clients == NULL) {
            snprintf(error, errorSize, "Could not create room.\n");
            roomCount--;
            pthread_mutex_unlock(&roomLock);
            return -1;
        }
    }
    Room *room = &rooms[index];
    for (int i = 0; i < room->clientCount; i++) {
        if (strcmp(room->clients[i].username, username) == 0) {
            snprintf(error, errorSize, "Username already taken in this room.\n");
            pthread_mutex_unlock(&roomLock);
            return -1;
        }
    }
    if (room->clientCount == room->clientCapacity) {
        int newCapacity = room->clientCapacity * 2;
        RoomClient *resizedClients = realloc(room->clients, (size_t)newCapacity * sizeof(RoomClient));
        if (resizedClients == NULL) {
            snprintf(error, errorSize, "Could not join room.\n");
            pthread_mutex_unlock(&roomLock);
            return -1;
        }
        room->clients = resizedClients;
        room->clientCapacity = newCapacity;
    }
    RoomClient *slot = &room->clients[room->clientCount++];
    slot->socket = clientSocket;
    memset(slot->username, 0, sizeof(slot->username));
    strncpy(slot->username, username, MAX_USERNAME_LEN - 1);
    pthread_mutex_unlock(&roomLock);
    return 0;
}

void room_leave(const char *roomCode, int clientSocket, int *roomDestroyed) {
    *roomDestroyed = 0;
    pthread_mutex_lock(&roomLock);
    int index = find_room_index(roomCode);
    if (index == -1) {
        pthread_mutex_unlock(&roomLock);
        return;
    }
    Room *room = &rooms[index];
    int removeIndex = -1;
    for (int i = 0; i < room->clientCount; i++) {
        if (room->clients[i].socket == clientSocket) {
            removeIndex = i;
        }
    }
    if (removeIndex == -1) {
        pthread_mutex_unlock(&roomLock);
        return;
    }
    for (int i = removeIndex; i < room->clientCount - 1; i++) {
        room->clients[i] = room->clients[i + 1];
    }
    room->clientCount--;
    if (room->clientCount == 0) {
        printf("Room %s destroyed.\n", room->roomCode);
        free(room->clients);
        rooms[index] = rooms[roomCount - 1];
        roomCount--;
        *roomDestroyed = 1;
    }
    pthread_mutex_unlock(&roomLock);
}

int room_broadcast(const char *roomCode, int senderSocket, const char *senderMessage, const char *othersMessage) {
    int sentCount = 0;
    pthread_mutex_lock(&roomLock);
    int index = find_room_index(roomCode);
    if (index != -1) {
        Room *room = &rooms[index];
        for (int i = 0; i < room->clientCount; i++) {
            const char *message = room->clients[i].socket == senderSocket ? senderMessage : othersMessage;
            if (send_all(room->clients[i].socket, message) == 0) {
                sentCount++;
            }
        }
    }

    pthread_mutex_unlock(&roomLock);
    return sentCount;
}

int room_client_count(const char *roomCode) {
    int clientCount = 0;
    pthread_mutex_lock(&roomLock);
    int index = find_room_index(roomCode);
    if (index != -1) {clientCount = rooms[index].clientCount;}
    pthread_mutex_unlock(&roomLock);
    return clientCount;
}
