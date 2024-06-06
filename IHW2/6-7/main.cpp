#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>

#define SHM_KEY 1234
#define SEM_KEY 5678
#define SHM_SIZE 1024
#define NUM_PROGRAMMERS 3

typedef struct {
    int programs[NUM_PROGRAMMERS];
    int results[NUM_PROGRAMMERS];
    int checking[NUM_PROGRAMMERS];
} shared_data;

shared_data *data;
int shm_id;
int sem_id;

// Функция программиста
void programmer(int id) {
    printf("Programmer %d started.\n", id);
    srand(time(NULL) + id);
    while (1) {
        sleep(rand() % 5 + 1); // Симуляция времени на написание программы

        struct sembuf sem_lock = {0, -1, SEM_UNDO}; // Блокировка семафора
        semop(sem_id, &sem_lock, 1);

        int other = rand() % NUM_PROGRAMMERS;
        while (other == id || data->checking[other]) {
            other = rand() % NUM_PROGRAMMERS;
        }
        data->programs[id]++;
        data->checking[other] = 1;

        printf("Programmer %d is checking the program of Programmer %d\n", other, id);

        struct sembuf sem_unlock = {0, 1, SEM_UNDO}; // Разблокировка семафора
        semop(sem_id, &sem_unlock, 1);

        sleep(rand() % 5 + 1); // Симуляция времени на проверку программы

        semop(sem_id, &sem_lock, 1);

        int result = rand() % 2; // Генерация случайного результата проверки
        data->results[id] = result;
        data->checking[other] = 0;

        if (result == 0) {
            printf("Programmer %d: Program from Programmer %d is incorrect.\n", other, id);
        } else {
            printf("Programmer %d: Program from Programmer %d is correct.\n", other, id);
        }

        semop(sem_id, &sem_unlock, 1);

        if (result == 0) {
            printf("Programmer %d is fixing the program.\n", id);
            continue;
        }
    }
}

int main() {
    // Создание или подключение к разделяемой памяти
    shm_id = shmget(SHM_KEY, sizeof(shared_data), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    data = (shared_data*) shmat(shm_id, NULL, 0);
    if (data == (void*) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    memset(data, 0, sizeof(shared_data));

    // Создание семафора
    sem_id = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    // Инициализация семафора
    union semun {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
    } arg;
    arg.val = 1;
    if (semctl(sem_id, 0, SETVAL, arg) == -1) {
        perror("semctl");
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

    // Ожидание завершения всех дочерних процессов
    for (int i = 0; i < NUM_PROGRAMMERS; i++) {
        if (waitpid(pids[i], NULL, 0) < 0) {
            perror("waitpid");
        }
    }

    // Отключение от разделяемой памяти
    if (shmdt(data) == -1) {
        perror("shmdt");
    }

    // Удаление разделяемой памяти
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("shmctl");
    }

    // Удаление семафора
    if (semctl(sem_id, 0, IPC_RMID) == -1) {
        perror("semctl");
    }

    return 0;
}