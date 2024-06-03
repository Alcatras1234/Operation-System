#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_KEY 1234 // Ключ для разделяемой памяти
#define SHM_SIZE sizeof(int) // Размер разделяемой памяти

int main() {
    // Получаем идентификатор сегмента разделяемой памяти
    int shmid = shmget(SHM_KEY, SHM_SIZE, 0666);
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

    // Читаем случайное число из разделяемой памяти
    int random_number = *shared_memory;

    // Выводим случайное число
    printf("Сервер: Получено число %d из разделяемой памяти.\n", random_number);

    // Отключаем сегмент разделяемой памяти
    shmdt(shared_memory);

    // Удаляем сегмент разделяемой памяти
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}
