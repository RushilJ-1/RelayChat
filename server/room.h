#ifndef ROOM_H
#define ROOM_H
#include <pthread.h>
#define MAX_ROOM_CODE_LEN 16
#define MAX_USERNAME_LEN 32
#define MAX_ROOMS 128
#define INITIAL_ROOM_CAPACITY 4
typedef struct {
    int socket;
    char username[MAX_USERNAME_LEN];
} RoomClient;

typedef struct {
    char roomCode[MAX_ROOM_CODE_LEN];
    int clientCount;
    int clientCapacity;
    RoomClient *clients;
} Room;

extern pthread_mutex_t roomLock;
void rooms_init(void);
void rooms_destroy(void);
int room_join(const char *roomCode, int clientSocket, const char *username, char *error, int errorSize);
void room_leave(const char *roomCode, int clientSocket, int *roomDestroyed);
int room_broadcast(const char *roomCode, int senderSocket, const char *senderMessage, const char *othersMessage);
int room_client_count(const char *roomCode);
#endif
