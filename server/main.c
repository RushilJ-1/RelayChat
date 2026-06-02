#include "client_handler.h"
#include "room.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_PORT 8080
#define BACKLOG 16
static int create_server_socket(int port) {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("socket");
        return -1;
    }
    int reuse = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        close(serverSocket);
        return -1;
    }
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons((uint16_t)port);
    if (bind(serverSocket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        close(serverSocket);
        return -1;
    }
    if (listen(serverSocket, BACKLOG) < 0) {
        perror("listen");
        close(serverSocket);
        return -1;
    }
    return serverSocket;
}

int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT;
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Invalid port.\n");
            return 1;
        }
    }
    rooms_init();
    int serverSocket = create_server_socket(port);
    if (serverSocket < 0) {
        rooms_destroy();
        return 1;
    }
    printf("Chat server listening on port %d...\n", port);
    while (1) {
        struct sockaddr_in clientAddress;
        socklen_t clientAddressLen = sizeof(clientAddress);
        int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientAddressLen);
        if (clientSocket < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("accept");
            break;
        }
        Client *client = malloc(sizeof(Client));
        if (client == NULL) {
            perror("malloc");
            close(clientSocket);
            continue;
        }
        memset(client, 0, sizeof(Client));
        client->socket = clientSocket;
        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, client) != 0) {
            perror("pthread_create");
            close(clientSocket);
            free(client);
            continue;
        }
        pthread_detach(thread);
    }
    close(serverSocket);
    rooms_destroy();
    return 0;
}
