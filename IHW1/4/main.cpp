#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>
#include <regex.h>
#include <sys/stat.h>
#include <cstring>

#define BUFFER_SIZE 5000

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int count_identificator(char buf[], ssize_t size) {
    int count = 0;
    char identifiers[100][100];
    for (ssize_t i = 0; buf[i] != '\0'; i++) {
        // Проверяем, является ли символ началом идентификатора
        if (isalpha(buf[i])) {
            ssize_t j = 0;
            identifiers[count][j++] = buf[i];
            // Считываем оставшуюся часть идентификатора
            while (isalnum(buf[i + 1])) {
                identifiers[count][j++] = buf[++i];
            }
            identifiers[count][j] = '\0'; // Завершаем строку
            count++;
        }
    }

    // Удаляем дубликаты идентификаторов
    for (ssize_t i = 0; i < count; i++) {
        for (ssize_t j = i + 1; j < count;) {
            if (strcmp(identifiers[i], identifiers[j]) == 0) {
                // Сдвигаем оставшиеся идентификаторы влево
                memmove(&identifiers[j], &identifiers[j + 1], (count - j - 1) * sizeof(identifiers[0]));
                count--;
            } else {
                j++;
            }
        }
    }
    for (ssize_t i = 0; i < count; i++) {
        printf("%s\n", identifiers[i]); // Print each identifier as a string
    }

    return count;
}

int main(int argc, char *argv[]) {
    // создание неименованных каналов
    int pipe1_fd[2];
    int pipe2_fd[2];
    char buffer[BUFFER_SIZE];
    ssize_t read_bytes;




    if (argc != 3) {
        fprintf(stderr, "Использование: %s <входной_файл> <выходной_файл>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (pipe(pipe1_fd) == -1 || pipe(pipe2_fd) == -1) {
        printf("Can't open pipes");
        exit(EXIT_FAILURE);
    }
    // channel1 - read input file
    pid_t channel1 = fork();
    if (channel1 < 0) {
        printf("Problem with create channel1");
        exit(EXIT_FAILURE);
    } else if (channel1 == 0) {

        close(pipe1_fd[0]); // close READ

        int fd = open(argv[1], O_RDONLY);
        if (fd == -1) {
            perror("Ошибка при открытии входного файла");
            exit(EXIT_FAILURE);
        }

        while((read_bytes = read(fd, buffer, BUFFER_SIZE)) > 0) {
            if (write(pipe1_fd[1], buffer, read_bytes) != read_bytes) {
                printf("Error write in pipe1_fd");
                exit(EXIT_FAILURE);
            }
        }

        close(fd);
        close(pipe1_fd[1]); // close write
        exit(EXIT_SUCCESS);
    }
    // channel2 - do work
    pid_t channel2 = fork();
    if (channel2 < 0) {
        printf("Problem with create channel2");
        exit(EXIT_FAILURE);
    } else if (channel2 == 0) {
        close(pipe1_fd[1]); // close WRITE
        close(pipe2_fd[0]); // close READ


        int count = 0; // count of iterators



        while ((read_bytes = read(pipe1_fd[0], buffer, BUFFER_SIZE)) > 0) {
            count += count_identificator(buffer, read_bytes);

        }
        if (read_bytes == -1) {
            error("Ошибка при чтении из канала");
            exit(EXIT_FAILURE);
        }

        char count_str[25];
        sprintf(count_str, "Кол-во слов: %d\n", count);

        // Ensure writing the correct string length
        if (write(pipe2_fd[1], count_str, strlen(count_str)) == -1) {
            error("Ошибка при записи в канал");
            exit(EXIT_FAILURE);
        }
        close(pipe1_fd[0]); //close READ
        close(pipe2_fd[1]); //close WRITE
        exit(EXIT_SUCCESS); //EXIT WITH SUCCESS STATUS
    }

    // create channel3 - writter
    pid_t channel3 = fork();
    if (channel3 < 0) {
        printf("Problem with create channel3");
        exit(EXIT_FAILURE);
    } else if (channel3 == 0) {
        close(pipe1_fd[0]); // close READ
        close(pipe1_fd[1]); // close WRITE
        close(pipe2_fd[1]); // close WRITE

        //open file to write
        int fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666 );
        if (fd == -1) {
            printf("Error open exit file");
            exit(EXIT_FAILURE);
        }

        ssize_t bytes_read;
        // Read until EOF or error
        while ((bytes_read = read(pipe2_fd[0], buffer, BUFFER_SIZE)) > 0) {
            if (write(fd, buffer, bytes_read) != bytes_read) {
                printf("Error writing to output file\n");
                exit(EXIT_FAILURE);
            }
        }
        if (bytes_read == -1) {
            printf("Error read pipe2\n");
            exit(EXIT_FAILURE);
        }


        close(fd); //close output file
        if (close(pipe2_fd[0]) < 0) {
            printf("Problem to close FIFO\n");
        } //close READ
        exit(EXIT_SUCCESS);

        close(fd); //close output file
        close(pipe2_fd[0]); //close READ
        exit(EXIT_SUCCESS);
    }

    close(pipe1_fd[0]);
    close(pipe1_fd[1]);
    close(pipe2_fd[0]);
    close(pipe2_fd[1]);

    waitpid(channel1, NULL, 0);
    waitpid(channel2, NULL, 0);
    waitpid(channel3, NULL, 0);


    return 0;
}
