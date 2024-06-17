#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../common.h"

#define BUFFER_SIZE 1024

void display_message(const message_t *msg) {
    printf("Observer: Message from client %d: %s\n", msg->sender_id, msg->content);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        exit(1);
    }

    char *server_ip = argv[1];
    int port = atoi(argv[2]);

    int sock;
    struct sockaddr_in server;
    message_t msg;
    int read_size;

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

    printf("Observer: Connected to server\n");

    while ((read_size = recv(sock, &msg, sizeof(message_t), 0)) > 0) {
        display_message(&msg);
    }

    if (read_size == 0) {
        printf("Observer: Disconnected from server\n");
        close(sock);
    } else if (read_size == -1) {
        perror("Recv failed");
    }

    return 0;
}
