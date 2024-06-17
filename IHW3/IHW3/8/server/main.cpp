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

void send_message_to_clients(message_t *msg, int exclude_index) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] && i != exclude_index) {
            if (send(clients[i]->sock, msg, sizeof(message_t), 0) < 0) {
                perror("Send failed");
                close(clients[i]->sock);
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
            if (send(observers[i]->sock, msg, sizeof(message_t), 0) < 0) {
                perror("Send to observer failed");
                close(observers[i]->sock);
                remove_observer(observers[i]->sock);
            } else {
                printf("Server: Sent message to observer from client %d\n", msg->sender_id);
            }
        }
    }
    pthread_mutex_unlock(&observers_mutex);
}

void *handle_client(void *arg) {
    client_t *cli = (client_t *)arg;
    message_t msg;
    int read_size;

    while ((read_size = recv(cli->sock, &msg, sizeof(message_t), 0)) > 0) {
        printf("Server: Received message from client %d\n", msg.sender_id);
        send_message_to_clients(&msg, cli->index);
    }

    if (read_size == 0) {
        printf("Server: Client %d disconnected\n", cli->index);
        close(cli->sock);
        remove_client(cli->index);
    } else if (read_size == -1) {
        perror("Recv failed");
    }

    free(cli);
    pthread_exit(NULL);
    return NULL;
}

void *handle_observer(void *arg) {
    client_t *cli = (client_t *)arg;
    message_t msg;
    int read_size;

    while ((read_size = recv(cli->sock, &msg, sizeof(message_t), 0)) > 0) {
        // Do nothing, observer only receives data
    }

    if (read_size == 0) {
        printf("Server: Observer client disconnected\n");
        close(cli->sock);
        remove_observer(cli->sock);
        free(cli);
    } else if (read_size == -1) {
        perror("Recv failed");
    }

    pthread_exit(NULL);
    return NULL;
}

int main(int argc, char *argv[]) {
    int server_sock, client_sock, c;
    struct sockaddr_in server, client;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
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

    listen(server_sock, MAX_CLIENTS + MAX_OBSERVERS);

    printf("Server: Waiting for connections...\n");
    c = sizeof(struct sockaddr_in);

    while ((client_sock = accept(server_sock, (struct sockaddr *)&client, (socklen_t *)&c)) > 0) {
        printf("Server: Connection accepted\n");

        pthread_t thread;
        client_t *cli = (client_t *)malloc(sizeof(client_t));
        cli->sock = client_sock;
        cli->address = client;
        cli->addr_len = c;

        // Determine if the client is a regular client or an observer
        char client_type;
        if (recv(cli->sock, &client_type, sizeof(char), 0) > 0) {
            if (client_type == 'O') {
                add_observer(cli);
                if (pthread_create(&thread, NULL, handle_observer, (void *)cli) < 0) {
                    perror("Could not create thread for observer");
                    free(cli);
                }
                pthread_detach(thread);
                printf("Server: Observer client connected\n");
            } else {
                add_client(cli);
                if (pthread_create(&thread, NULL, handle_client, (void *)cli) < 0) {
                    perror("Could not create thread");
                    free(cli);
                }
                pthread_detach(thread);
            }
        } else {
            perror("Recv client type failed");
            free(cli);
        }
    }

    if (client_sock < 0) {
        perror("Accept failed");
        close(server_sock);
        exit(1);
    }

    close(server_sock);
    return 0;
}
