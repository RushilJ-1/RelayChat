#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H
typedef struct {
    int socket;
    char username[32];
    char roomCode[16];
} Client;
void *handle_client(void *arg);
#endif
