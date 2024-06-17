#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "../common.h"

#define PORT 12346
#define MAX_CLIENTS 3
#define BUFFER_SIZE 1024
#define MAX_OBSERVERS 10

typedef struct {
    int sock;
    struct sockaddr_in address;
    int addr_len;
    int index;
} client_t;

client_t *clients[MAX_CLIENTS];
client_t *observers[MAX_OBSERVERS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t observers_mutex = PTHREAD_MUTEX_INITIALIZER;

void add_client(client_t *cl) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (!clients[i]) {
            clients[i] = cl;
            cl->index = i;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int index) {
    pthread_mutex_lock(&clients_mutex);
    clients[index] = NULL;
    pthread_mutex_unlock(&clients_mutex);
}

void add_observer(client_t *cl) {
    pthread_mutex_lock(&observers_mutex);
    for (int i = 0; i < MAX_OBSERVERS; ++i) {
        if (!observers[i]) {
            observers[i] = cl;
            break;
        }
    }
    pthread_mutex_unlock(&observers_mutex);
}

void remove_observer(int sock) {
    pthread_mutex_lock(&observers_mutex);
    for (int i = 0; i < MAX_OBSERVERS; ++i) {
        if (observers[i] && observers[i]->sock == sock) {
            observers[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&observers_mutex);
}

void send_message_to_clients(message_t *msg, struct sockaddr_in *client_addr, socklen_t client_len, int exclude_index, int sock) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] && i != exclude_index) {
            if (sendto(sock, msg, sizeof(message_t), 0, (struct sockaddr *) &clients[i]->address, clients[i]->addr_len) < 0) {
                perror("Send to client failed");
                remove_client(i);
            } else {
                printf("Server: Sent message from client %d to client %d\n", msg->sender_id, clients[i]->index);
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    pthread_mutex_lock(&observers_mutex);
    for (int i = 0; i < MAX_OBSERVERS; ++i) {
        if (observers[i]) {
            if (sendto(sock, msg, sizeof(message_t), 0, (struct sockaddr *) &observers[i]->address, observers[i]->addr_len) < 0) {
                perror("Send to observer failed");
                remove_observer(observers[i]->sock);
            } else {
                printf("Server: Sent message from client %d to observer\n", msg->sender_id);
            }
        }
    }
    pthread_mutex_unlock(&observers_mutex);
}

void *handle_client(void *arg) {
    client_t *cli = (client_t *)arg;
    message_t msg;
    int read_size;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while ((read_size = recvfrom(cli->sock, &msg, sizeof(message_t), 0, (struct sockaddr *) &client_addr, &client_len)) > 0) {
        printf("Server: Received message from client %d\n", msg.sender_id);
        send_message_to_clients(&msg, &client_addr, client_len, cli->index, cli->sock);
    }

    if (read_size == 0) {
        printf("Server: Client %d disconnected\n", cli->index);
        remove_client(cli->index);
    } else if (read_size == -1) {
        perror("Recv failed");
    }

    free(cli);
    pthread_exit(NULL);
    return NULL;
}

int main(int argc, char *argv[]) {
    int server_sock;
    struct sockaddr_in server, client;
    socklen_t client_len = sizeof(client);

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

    while (1) {
        message_t msg;
        int read_size = recvfrom(server_sock, &msg, sizeof(message_t), 0, (struct sockaddr *) &client, &client_len);

        if (read_size < 0) {
            perror("Recvfrom failed");
            continue;
        }

        // Determine if the client is a regular client or an observer
        char client_type = msg.content[0];
        if (client_type == 'O') {
            client_t *cli = (client_t *)malloc(sizeof(client_t));
            cli->sock = server_sock; // Store the server socket for observers
            cli->address = client;
            cli->addr_len = client_len;

            add_observer(cli);
            pthread_t thread;
            if (pthread_create(&thread, NULL, handle_client, (void *)cli) < 0) {
                perror("Could not create thread for observer");
                free(cli);
            }
            pthread_detach(thread);
            printf("Server: Observer client connected\n");
        } else {
            client_t *cli = (client_t *)malloc(sizeof(client_t));
            cli->sock = server_sock; // Store the server socket for clients
            cli->address = client;
            cli->addr_len = client_len;

            add_client(cli);
            pthread_t thread;
            if (pthread_create(&thread, NULL, handle_client, (void *)cli) < 0) {
                perror("Could not create thread");
                free(cli);
            }
            pthread_detach(thread);
            printf("Server: Client connected\n");
        }
    }

    close(server_sock);
    return 0;
}

