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

client_t *clients[MAX_CLIENTS];
client_t *observer_client = NULL;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void add_client(struct sockaddr_in *client_addr, socklen_t addr_len) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (!clients[i]) {
            clients[i] = (client_t *)malloc(sizeof(client_t));
            clients[i]->address = *client_addr;
            clients[i]->addr_len = addr_len;
            clients[i]->index = i;
            printf("Server: Client added at index %d\n", i);
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int index) {
    pthread_mutex_lock(&clients_mutex);
    if (clients[index]) {
        free(clients[index]);
        clients[index] = NULL;
        printf("Server: Client removed from index %d\n", index);
    }
    pthread_mutex_unlock(&clients_mutex);
}

void send_message_to_clients(int sock, message_t *msg, struct sockaddr_in *exclude_addr) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] && (exclude_addr == NULL || memcmp(&clients[i]->address, exclude_addr, sizeof(struct sockaddr_in)) != 0)) {
            if (sendto(sock, msg, sizeof(message_t), 0, (struct sockaddr *)&clients[i]->address, clients[i]->addr_len) < 0) {
                perror("Send failed");
                remove_client(i);
            } else {
                printf("Server: Sent message from client %d to client %d\n", msg->sender_id, clients[i]->index);
            }
        }
    }
    if (observer_client && (exclude_addr == NULL || memcmp(&observer_client->address, exclude_addr, sizeof(struct sockaddr_in)) != 0)) {
        if (sendto(sock, msg, sizeof(message_t), 0, (struct sockaddr *)&observer_client->address, observer_client->addr_len) < 0) {
            perror("Send to observer failed");
            free(observer_client);
            observer_client = NULL;
        } else {
            printf("Server: Sent message to observer from client %d\n", msg->sender_id);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg) {
    int sock = *(int *)arg;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    message_t msg;
    int read_size;

    while (1) {
        read_size = recvfrom(sock, &msg, sizeof(message_t), 0, (struct sockaddr *)&client_addr, &addr_len);
        if (read_size > 0) {
            printf("Server: Received message from client %d\n", msg.sender_id);

            pthread_mutex_lock(&clients_mutex);
            int client_exists = 0;
            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (clients[i] && memcmp(&clients[i]->address, &client_addr, sizeof(struct sockaddr_in)) == 0) {
                    client_exists = 1;
                    break;
                }
            }
            if (!client_exists) {
                add_client(&client_addr, addr_len);
                printf("Server: New client added\n");
            }
            pthread_mutex_unlock(&clients_mutex);

            send_message_to_clients(sock, &msg, &client_addr);
        } else if (read_size < 0) {
            perror("Recvfrom failed");
            break;
        }
    }

    pthread_exit(NULL);
    return NULL;
}

void *handle_observer(void *arg) {
    int sock = *(int *)arg;
    struct sockaddr_in observer_addr;
    socklen_t addr_len = sizeof(observer_addr);
    message_t msg;
    int read_size;

    while (1) {
        read_size = recvfrom(sock, &msg, sizeof(message_t), 0, (struct sockaddr *)&observer_addr, &addr_len);
        if (read_size > 0) {
            // Observer only receives data
        } else if (read_size < 0) {
            perror("Recvfrom failed");
            break;
        }
    }

    pthread_exit(NULL);
    return NULL;
}

int main(int argc, char *argv[]) {
    int server_sock;
    struct sockaddr_in server;

    server_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_sock == -1) {
        perror("Could not create socket");
        exit(1);
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Bind failed");
        close(server_sock);
        exit(1);
    }

    printf("Server: Waiting for connections...\n");

    pthread_t thread;
    if (pthread_create(&thread, NULL, handle_client, (void *)&server_sock) < 0) {
        perror("Could not create client handler thread");
        close(server_sock);
        exit(1);
    }

    pthread_detach(thread);

    if (pthread_create(&thread, NULL, handle_observer, (void *)&server_sock) < 0) {
        perror("Could not create observer handler thread");
        close(server_sock);
        exit(1);
    }

    pthread_detach(thread);

    while (1) {
        sleep(1); // Main thread can do other tasks or just sleep
    }

    close(server_sock);
    return 0;
}
