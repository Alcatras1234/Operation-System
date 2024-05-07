#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <regex.h>
#include <ctype.h>
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
    if (argc != 3) {
        fprintf(stderr, "Использование: %s <входной_файл> <выходной_файл>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int pipe1_fd[2], pipe2_fd[2];
    char buffer[BUFFER_SIZE];


    if (pipe(pipe1_fd) == -1 || pipe(pipe2_fd) == -1) {
        error("Ошибка при создании каналов");
    }

    pid_t pid = fork();
    if (pid < 0) {
        error("Ошибка при создании дочернего процесса");
    } else if (pid == 0) { // Второй процесс
        close(pipe1_fd[1]); // Закрываем конец для записи
        close(pipe2_fd[0]); // Закрываем конец для чтения
     






        ssize_t read_bytes = read(pipe1_fd[0], buffer, BUFFER_SIZE);
        int count = count_identificator(buffer, read_bytes);
        

        // Записываем результат обработки в канал pipe2_fd

        char count_str[25];
        sprintf(count_str, "Кол-во слов: %d\n", count);

        // Записываем результат обработки в канал pipe2_fd
        if (write(pipe2_fd[1], count_str, sizeof(count_str)) == -1) {
            error("Ошибка при записи в канал");
            exit(EXIT_FAILURE);
        }

        close(pipe1_fd[0]); // Закрываем конец для чтения
        close(pipe2_fd[1]); // Закрываем конец для записи
        exit(EXIT_SUCCESS);
    } else { // Первый процесс
        close(pipe1_fd[0]); // Закрываем конец для чтения
        close(pipe2_fd[1]); // Закрываем конец для записи
        ssize_t read_bytes;
        int fd_in = open(argv[1], O_RDONLY);
        if (fd_in == -1) {
            error("Ошибка при открытии входного файла");
            exit(EXIT_FAILURE);
        }

        int fd_out = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd_out == -1) {
            error("Ошибка при открытии выходного файла");
            exit(EXIT_FAILURE);
        }

        while ((read_bytes = read(fd_in, buffer, BUFFER_SIZE)) > 0) {
            // Отправка данных из файла второму процессу через канал
            if (write(pipe1_fd[1], buffer, read_bytes) != read_bytes) {
                error("Ошибка при записи в канал");
                exit(EXIT_FAILURE);
            }
        }
        close(fd_in);
        close(pipe1_fd[1]); // Закрываем конец для записи

        // Ожидание завершения второго процесса
        wait( NULL);

        // Получение обработанных данных от второго процесса через канал
        ssize_t bytes_read;
        while ((bytes_read = read(pipe2_fd[0], buffer, BUFFER_SIZE)) > 0) {

            // Запись обработанных данных в выходной файл
            if (write(fd_out, buffer, bytes_read) != bytes_read) {
                error("Ошибка при записи в файл");
                exit(EXIT_FAILURE);
            }
        }

        if (bytes_read == -1) {
            error("Ошибка при чтении из канала");
            exit(EXIT_FAILURE);
        }

        close(fd_in);
        close(fd_out);
        close(pipe2_fd[0]); // Закрываем конец для чтения

        exit(EXIT_SUCCESS);
    }

    return 0;
}
