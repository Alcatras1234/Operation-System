#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "../common.h"

#define PORT 12345
#define MAX_CLIENTS 3
#define BUFFER_SIZE 1024

typedef struct {
    struct sockaddr_in address;
    socklen_t addr_len;
    int index;
} client_t;

client_t clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
int client_count = 0;

void add_client(struct sockaddr_in *addr, socklen_t addr_len) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i].address.sin_addr.s_addr == addr->sin_addr.s_addr && clients[i].address.sin_port == addr->sin_port) {
            pthread_mutex_unlock(&clients_mutex);
            return;
        }
    }
    if (client_count < MAX_CLIENTS) {
        clients[client_count].address = *addr;
        clients[client_count].addr_len = addr_len;
        clients[client_count].index = client_count;
        client_count++;
    }
    pthread_mutex_unlock(&clients_mutex);
}

void send_message(int sockfd, message_t *msg, struct sockaddr_in *exclude_addr) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (exclude_addr && clients[i].address.sin_addr.s_addr == exclude_addr->sin_addr.s_addr && clients[i].address.sin_port == exclude_addr->sin_port) {
            continue;
        }
        sendto(sockfd, msg, sizeof(message_t), 0, (struct sockaddr *)&clients[i].address, clients[i].addr_len);
    }
    pthread_mutex_unlock(&clients_mutex);
}

int main(int argc, char *argv[]) {
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    message_t msg;

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Could not create socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind the socket to the port
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Server: Waiting for messages...\n");

    while (1) {
        int len = recvfrom(sockfd, &msg, sizeof(message_t), 0, (struct sockaddr *)&client_addr, &addr_len);
        if (len < 0) {
            perror("Recvfrom failed");
            continue;
        }

        add_client(&client_addr, addr_len);
        printf("Server: Received message from client %d\n", msg.sender_id);

        send_message(sockfd, &msg, &client_addr);
    }

    close(sockfd);
    return 0;
}
