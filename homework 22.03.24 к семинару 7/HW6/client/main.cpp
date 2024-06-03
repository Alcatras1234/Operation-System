#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_KEY 1234 // Ключ для разделяемой памяти
#define SHM_SIZE sizeof(int) // Размер разделяемой памяти

int main() {
    // Генерируем случайное число
    srand(time(NULL));
    int random_number = rand() % 100;

    // Получаем идентификатор сегмента разделяемой памяти
    int shmid = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    // Подключаем сегмент разделяемой памяти
    int *shared_memory = (int *)shmat(shmid, NULL, 0);
    if (shared_memory == (int *)-1) {
        perror("shmat");
        exit(1);
    }

    // Записываем случайное число в разделяемую память
    *shared_memory = random_number;

    // Отключаем сегмент разделяемой памяти
    shmdt(shared_memory);

    printf("Клиент: Сгенерировано число %d и записано в разделяемую память.\n", random_number);

    return 0;
}
