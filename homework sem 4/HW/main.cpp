
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

const int buf = 4096;

// Циклическое чтение с использованием системного вызова read
// Например, когда буфер небольшой
int main(int argc, char *argv[]) {
    int     fdStart, fdEnd;
    char    buffer[buf];
    ssize_t read_bytes, write_bytes;

    char start[100];
    char end[100];
    printf("Введите файл откуда считывать текст ");
    scanf("%99s", start);
    printf("Введите файл куда считывать текст ");
    scanf("%99s", end);


    if((fdStart = open(start, O_RDONLY)) < 0) {
        printf("Can\'t open file\n");
        exit(-1);
    }


    if ((fdEnd = open(end, O_WRONLY | O_CREAT, 0666)) < 0) {
        printf("Can\'t open output file\n");
        exit(-1);
    }

    do {
        read_bytes = read(fdStart, buffer, buf);
        if (read_bytes == -1) {
            printf("Can\'t write this file\n");
            exit(-1);
        }
        write_bytes = write(fdEnd, buffer, read_bytes);
        if (write_bytes != read_bytes) {
            printf("Error writing to output file");
            exit(-1);
        }

    } while (read_bytes > 0);

    if (close(fdStart) < 0) {
        printf("Can\'t close file\n %s", start);
    }

    if (close(fdEnd) < 0) {
        printf("Can\'t close file\n %s", end);
    }


    return 0;
}
