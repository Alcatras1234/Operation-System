#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

volatile sig_atomic_t received_data = 0;
volatile sig_atomic_t bit_count = 0;
pid_t transmitter_pid; // Переменная для хранения PID передатчика

void receive_bit(int signum) {
    int bit = (signum == SIGUSR1) ? 1 : 0;
    received_data |= bit << bit_count;
    bit_count++;

    // Подтверждаем прием бита
    if (bit_count < sizeof(int) * 8) {
        kill(transmitter_pid, SIGUSR1);
    } else {
        printf("Принятое число: %d\n", received_data);
        // Отправляем сигнал обратно передатчику перед завершением работы
        kill(transmitter_pid, SIGUSR1);

    }
}

int main() {
    printf("PID приемника: %d\n", getpid());

    signal(SIGUSR1, receive_bit);
    signal(SIGUSR2, receive_bit);

    printf("Введите PID передатчика: ");
    scanf("%d", &transmitter_pid); // Считываем PID передатчика

    // Ждем сигналов от передатчика
    while (1) {
        pause(); // Ждем сигнала от передатчика
    }

    return 0;
}
