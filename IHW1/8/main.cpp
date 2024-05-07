#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <regex.h>
#include <ctype.h>
#include <sys/stat.h>

#define BUFFER_SIZE 5000

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Использование: %s <входной_файл> <выходной_файл>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char    pipe1_fd[] = "pipe1";
    char    pipe2_fd[] = "pipe2";
    char buffer[BUFFER_SIZE];


    if (mknod(pipe1_fd, S_IFIFO | 0666, 0) == -1||
        mknod(pipe2_fd, S_IFIFO | 0666, 0) == -1) {
        perror("Error create name channel");
    }

    int pipe1 = open(pipe1_fd, O_WRONLY); // Закрываем конец для чтения
    int pipe2 = open(pipe2_fd, O_RDONLY); // Закрываем конец для записи
    ssize_t read_bytes;
    int fd_in = open(argv[1], O_RDONLY);
    if (fd_in == -1) {
        perror("Ошибка при открытии входного файла");
        exit(EXIT_FAILURE);
    }

    int fd_out = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd_out == -1) {
        perror("Ошибка при открытии выходного файла");
        exit(EXIT_FAILURE);
    }

    while ((read_bytes = read(fd_in, buffer, BUFFER_SIZE)) > 0) {
        // Отправка данных из файла второму процессу через канал
        if (write(pipe1, buffer, read_bytes) != read_bytes) {
            perror("Ошибка при записи в канал");
            exit(EXIT_FAILURE);
        }
    }
    close(fd_in);
    close(pipe1); // Закрываем конец для записи



    // Получение обработанных данных от второго процесса через канал
    ssize_t bytes_read;
    while ((bytes_read = read(pipe2, buffer, BUFFER_SIZE)) > 0) {

        // Запись обработанных данных в выходной файл
        if (write(fd_out, buffer, bytes_read) != bytes_read) {
            perror("Ошибка при записи в файл");
            exit(EXIT_FAILURE);
        }
    }

    if (bytes_read == -1) {
        perror("Ошибка при чтении из канала");
        exit(EXIT_FAILURE);
    }

    close(fd_in);
    close(fd_out);
    close(pipe2); // Закрываем конец для чтения

    if (unlink(pipe1_fd) == -1 || unlink(pipe2_fd) == -1) {
        perror("Ошибка при удалении именованных каналов");
    }
    exit(EXIT_SUCCESS);

    return 0;
}