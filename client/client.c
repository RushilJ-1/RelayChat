#include <arpa/inet.h>
#include <stdatomic.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 8080
#define BUFFER_SIZE 512
static atomic_int running = 1;

static void *receive_messages(void *arg) {
    int socket = *(int *)arg;
    char buffer[BUFFER_SIZE];

    while (atomic_load(&running)) {
        ssize_t received = recv(socket, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) {
            atomic_store(&running, 0);
            printf("\nDisconnected from server.\n");
            break;
        }
        buffer[received] = '\0';
        printf("%s", buffer);
        fflush(stdout);
    }

    return NULL;
}

static int connect_to_server(const char *host, int port) {
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        perror("socket");
        return -1;
    }
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons((uint16_t)port);

    if (inet_pton(AF_INET, host, &serverAddress.sin_addr) <= 0) {
        perror("inet_pton");
        close(clientSocket);
        return -1;
    }
    if (connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("connect");
        close(clientSocket);
        return -1;
    }

    return clientSocket;
}

int main(int argc, char *argv[]) {
    const char *host = DEFAULT_HOST;
    int port = DEFAULT_PORT;
    if (argc > 1) {
        host = argv[1];
    }
    if (argc > 2) {
        port = atoi(argv[2]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Invalid port.\n");
            return 1;
        }
    }
    int clientSocket = connect_to_server(host, port);
    if (clientSocket < 0) {
        return 1;
    }
    pthread_t receiverThread;
    if (pthread_create(&receiverThread, NULL, receive_messages, &clientSocket) != 0) {
        perror("pthread_create");
        close(clientSocket);
        return 1;
    }
    char buffer[BUFFER_SIZE];
    while (atomic_load(&running) && fgets(buffer, sizeof(buffer), stdin) != NULL) {
        if (send(clientSocket, buffer, strlen(buffer), 0) < 0) {
            perror("send");
            break;
        }
        if (strncmp(buffer, "/quit", 5) == 0) {
            atomic_store(&running, 0);
            break;
        }
    }
    shutdown(clientSocket, SHUT_RDWR);
    close(clientSocket);
    pthread_join(receiverThread, NULL);
    return 0;
}
