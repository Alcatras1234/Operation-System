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
    struct sockaddr_in server, from_addr;
    socklen_t from_len = sizeof(from_addr);
    message_t msg;
    int read_size;

    // Создание UDP сокета
    sock = socket(AF_INET, SOCK_DGRAM, 0);  // Здесь используется UDP
    if (sock == -1) {
        perror("Could not create socket");
        exit(1);
    }

    // Настройка адреса сервера
    memset(&server, 0, sizeof(server));
    server.sin_addr.s_addr = inet_addr(server_ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    // Отправка сообщения на сервер для регистрации в качестве наблюдателя
    char registration_message[] = "REGISTER_OBSERVER";
    if (sendto(sock, registration_message, sizeof(registration_message), 0, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Sendto failed");
        close(sock);
        exit(1);
    }

    printf("Observer: Sent registration to server\n");

    // Получение сообщений от сервера
    while ((read_size = recvfrom(sock, &msg, sizeof(message_t), 0, (struct sockaddr *)&from_addr, &from_len)) > 0) {
        display_message(&msg);
    }

    if (read_size == 0) {
        printf("Observer: Disconnected from server\n");
    } else if (read_size == -1) {
        perror("Recvfrom failed");
    }

    close(sock);
    return 0;
}

