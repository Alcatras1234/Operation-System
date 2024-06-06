#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>

#define SHM_NAME "/posix_shm"
#define SEM_NAME "/posix_sem"
#define SHM_SIZE 1024
#define NUM_PROGRAMMERS 3

typedef struct {
    int programs[NUM_PROGRAMMERS];
    int results[NUM_PROGRAMMERS];
    int checking[NUM_PROGRAMMERS];
} shared_data;

shared_data *data;
int shm_fd;
sem_t *sem;
volatile sig_atomic_t stop = 0;

// Обработчик сигнала завершения
void handle_sigint(int sig) {
    if (!stop) {
        stop = 1;
        printf("Caught signal %d, stopping...\n", sig);
        fflush(stdout);
        // Отправка сигнала всей группе процессов
        killpg(getpgrp(), SIGINT);
        printf("All programmers terminated. Exiting.\n");
        fflush(stdout);
    }
}

// Инициализация разделяемой памяти и семафора
void init_shared_memory() {
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }
    data = (shared_data*) mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    memset(data, 0, sizeof(shared_data));
}

// Функция программиста
void programmer(int id) {
    printf("Programmer %d started.\n", id);
    srand(time(NULL) + id);
    while (!stop) {
        sleep(3); // Симуляция времени на написание программы
        sem_wait(sem);

        int other = rand() % NUM_PROGRAMMERS;
        while (other == id || data->checking[other]) {
            other = rand() % NUM_PROGRAMMERS;
        }
        data->programs[id]++;
        data->checking[other] = 1;

        printf("Programmer %d is checking the program of Programmer %d\n", other, id);
        fflush(stdout);
        sem_post(sem);

        sleep(3); // Симуляция времени на проверку программы
        sem_wait(sem);

        int result = rand() % 2; // Генерация случайного результата проверки
        data->results[id] = result;
        data->checking[other] = 0;

        if (result == 0) {
            printf("Programmer %d: Program from Programmer %d is incorrect.\n", other, id);
        } else {
            printf("Programmer %d: Program from Programmer %d is correct.\n", other, id);
        }
        fflush(stdout);
        sem_post(sem);

        if (result == 0) {
            printf("Programmer %d is fixing the program.\n", id);
            fflush(stdout);
            continue;
        }
    }

    printf("Programmer %d is terminating.\n", id);
    fflush(stdout);
}

int main() {
    // Установка обработчика сигнала для всех процессов группы
    signal(SIGINT, handle_sigint);

    // Установка группы процессов
    if (setpgid(0, 0) == -1) {
        perror("setpgid");
        exit(EXIT_FAILURE);
    }

    init_shared_memory();

    sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    pid_t pids[NUM_PROGRAMMERS];
    for (int i = 0; i < NUM_PROGRAMMERS; i++) {
        if ((pids[i] = fork()) == 0) {
            programmer(i);
            exit(0);
        } else if (pids[i] < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }

    printf("Press Ctrl+C to stop...\n");

    // Ожидание завершения всех дочерних процессов
    for (int i = 0; i < NUM_PROGRAMMERS; i++) {
        if (waitpid(pids[i], NULL, 0) < 0) {
            perror("waitpid");
        }
    }

    sem_close(sem);
    sem_unlink(SEM_NAME);
    munmap(data, SHM_SIZE);
    shm_unlink(SHM_NAME);

    printf("All programmers terminated. Exiting.\n");
    return 0;
}
