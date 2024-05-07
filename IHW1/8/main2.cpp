#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <error.h>

#define BUFFER_SIZE 5000

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

int main() {
    char buffer[BUFFER_SIZE];
    int pipe1 = open("pipe1", O_RDONLY); // open pipe1 for read
    int pipe2 = open("pipe2", O_WRONLY); // open pipe2 for write

    if (pipe1 == -1 || pipe2 == -1) {
        perror("Ошибка при открытии именованных каналов");
    }


    ssize_t read_bytes = read(pipe1, buffer, BUFFER_SIZE);
    int count = count_identificator(buffer, read_bytes);


// Записываем результат обработки в канал pipe2_fd

    char count_str[25];
    sprintf(count_str, "Кол-во слов: %d\n", count);

// Записываем результат обработки в канал pipe2_fd
    if (write(pipe2, count_str, sizeof(count_str)) == -1) {
        perror("Ошибка при записи в канал");
        exit(EXIT_FAILURE);
    }

    close(pipe1); // Закрываем конец для чтения
    close(pipe2); // Закрываем конец для записи
    exit(EXIT_SUCCESS);
}