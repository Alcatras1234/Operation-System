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
sem_t *sem;

// Функция программиста
void programmer(int id) {
    printf("Programmer %d started.\n", id);
    srand(time(NULL) + id);
    while (1) {
        sleep(rand() % 5 + 1); // Симуляция времени на написание программы
        sem_wait(sem);

        int other = rand() % NUM_PROGRAMMERS;
        while (other == id || data->checking[other]) {
            other = rand() % NUM_PROGRAMMERS;
        }
        data->programs[id]++;
        data->checking[other] = 1;

        printf("Programmer %d is checking the program of Programmer %d\n", other, id);
        sem_post(sem);

        sleep(rand() % 5 + 1); // Симуляция времени на проверку программы
        sem_wait(sem);

        int result = rand() % 2; // Генерация случайного результата проверки
        data->results[id] = result;
        data->checking[other] = 0;

        if (result == 0) {
            printf("Programmer %d: Program from Programmer %d is incorrect.\n", other, id);
        } else {
            printf("Programmer %d: Program from Programmer %d is correct.\n", other, id);
        }
        sem_post(sem);

        if (result == 0) {
            printf("Programmer %d is fixing the program.\n", id);
            continue;
        }
    }
}

int main() {
    // Инициализация семафора
    sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    // Создание или подключение к разделяемой памяти
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(shm_fd, sizeof(shared_data)) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }
    data = (shared_data *) mmap(NULL, sizeof(shared_data), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    memset(data, 0, sizeof(shared_data));

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

    // Ожидание завершения всех дочерних процессов
    for (int i = 0; i < NUM_PROGRAMMERS; i++) {
        if (waitpid(pids[i], NULL, 0) < 0) {
            perror("waitpid");
        }
    }

    // Закрытие и удаление семафора
    sem_close(sem);
    sem_unlink(SEM_NAME);

    // Демонтирование и удаление разделяемой памяти
    munmap(data, sizeof(shared_data));
    shm_unlink(SHM_NAME);

    return 0;
}
