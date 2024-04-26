#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

pid_t receiver_pid;

void send_bit(int bit) {
    if (bit) {
        kill(receiver_pid, SIGUSR1);
    } else {
        kill(receiver_pid, SIGUSR2);
    }
}

void confirmation_handler(int signum) {
    // Пустой обработчик
}

int main() {
    int data;
    int i;

    printf("PID передатчика: %d\n", getpid());
    printf("Введите PID приемника: ");
    scanf("%d", &receiver_pid);

    printf("Введите целое десятичное число для передачи: ");
    scanf("%d", &data);


    printf("Передаваемое число: %d\n", data);

    signal(SIGUSR1, confirmation_handler);
    signal(SIGUSR2, confirmation_handler);

    for (i = 0; i < sizeof(int) * 8; i++) {
        int bit = (data >> i) & 1;
        send_bit(bit);
        pause(); // Ожидаем подтверждения от приемника
    }

    kill(receiver_pid, SIGINT); // Завершаем передачу

    return 0;
}
