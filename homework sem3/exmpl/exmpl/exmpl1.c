
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>

unsigned long long factorial(unsigned long long start) {
    if (start < 2) {
        return 1;
    }
    unsigned long long result = 1;

    for (unsigned long long i = 2; i <= start; ++i) {
        result *= i;

        if (result < i) {
            printf("overflow");
            return 0;
        }
    }

    return result;

}

unsigned long long fibonachi(unsigned long long start) {
    unsigned long long first = 0;
    unsigned long long second = 1;

    if (start == 1) {
        return 1;
    }

    for (unsigned long long i = 2; i <= start; ++i) {
        if (first > ULLONG_MAX - second) {
            printf("overflow");
            return 0;
        }
        unsigned long long temp = first + second;
        first = second;
        second = temp;

    }
    return second;
}

int main(int argc, char *argv[]) {
    pid_t pid, ppid, chpid;
    chpid = fork();
    pid  = getpid();
    ppid = getppid();
    unsigned long long start = strtoull(argv[1], NULL, 10);;

    if(chpid <= -1) {
        printf("Incorrect fork syscall\n");
    } else if (chpid == 0) { // Ребенок делает работу по вычислению факториала
        unsigned long long result = factorial(start);
        printf("Child %llu\n", result);
    } else { // Родитель делает работу по вычислению чисел Фибоначи
        unsigned long long result = fibonachi(start);
        printf("Parent %llu\n", result);
    }
    return 0;
}