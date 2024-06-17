#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../common.h"

programmer_state_t state = WRITING_PROGRAM;

void write_program(int sock, int client_id) {
    message_t msg;
    msg.type = NEW_PROGRAM;
    msg.sender_id = client_id;
    snprintf(msg.content, BUFFER_SIZE, "Program written by client %d", client_id);
    send(sock, &msg, sizeof(message_t), 0);
    printf("Client %d: Sent new program\n", client_id); // Debug message
    state = SLEEPING;
}

void check_program(int sock, int client_id) {
    message_t msg;
    int read_size;

    if ((read_size = recv(sock, &msg, sizeof(message_t), 0)) > 0) {
        printf("Client %d: Received message of type %d\n", client_id, msg.type);

        if (msg.type == NEW_PROGRAM) {
            printf("Client %d: Checking program: %s\n", client_id, msg.content);
            state = CHECKING_PROGRAM;
            msg.type = CHECK_RESULT;
            snprintf(msg.content, BUFFER_SIZE, rand() % 2 == 0 ? "Program correct" : "Program incorrect");
            msg.sender_id = client_id;
            send(sock, &msg, sizeof(message_t), 0);
            printf("Client %d: Sent check result: %s\n", client_id, msg.content); // Debug message
            state = SLEEPING;
        } else if (msg.type == CHECK_RESULT || msg.type == FIXED_PROGRAM) {
            printf("Client %d: Received result: %s\n", client_id, msg.content);
            if (strcmp(msg.content, "Program correct") == 0) {
                state = WRITING_PROGRAM;
                printf("Client %d: Program correct, writing new program\n", client_id); // Debug message
            } else {
                state = FIXING_PROGRAM;
                snprintf(msg.content, BUFFER_SIZE, "Fixed program by client %d", client_id);
                msg.type = FIXED_PROGRAM;
                msg.sender_id = client_id;
                send(sock, &msg, sizeof(message_t), 0);
                printf("Client %d: Sent fixed program\n", client_id); // Debug message
                state = SLEEPING;
            }
        }
    } else if (read_size == 0) {
        printf("Client %d: Disconnected from server\n", client_id);
        close(sock);
        exit(0);
    } else {
        perror("Recv failed");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <server_ip> <port> <client_id>\n", argv[0]);
        exit(1);
    }

    char *server_ip = argv[1];
    int port = atoi(argv[2]);
    int client_id = atoi(argv[3]);

    int sock;
    struct sockaddr_in server;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Could not create socket");
        exit(1);
    }

    server.sin_addr.s_addr = inet_addr(server_ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Connect failed");
        exit(1);
    }

    printf("Client %d: Connected to server\n", client_id);

    while (1) {
        if (state == WRITING_PROGRAM) {
            write_program(sock, client_id);
        } else if (state == SLEEPING) {
            check_program(sock, client_id);
        } else if (state == FIXING_PROGRAM) {
            message_t msg;
            snprintf(msg.content, BUFFER_SIZE, "Fixed program by client %d", client_id);
            msg.type = FIXED_PROGRAM;
            msg.sender_id = client_id;
            send(sock, &msg, sizeof(message_t), 0);
            printf("Client %d: Sent fixed program\n", client_id); // Debug message
            state = SLEEPING;
        }
        usleep(100000); // Задержка для имитации работы
    }

    close(sock);
    return 0;
}
