#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

volatile sig_atomic_t number_to_guess = 0;  // Число для угадывания
volatile sig_atomic_t guess = 0;            // Предположение
volatile sig_atomic_t guessed = 0;          // Флаг угаданного числа

void handle_guess_signal(int signo, siginfo_t *info, void *context) {
    guess = info->si_value.sival_int;  // Получаем переданное число
    if (guess == number_to_guess) {
        guessed = 1;
        kill(info->si_pid, SIGUSR1);  // Угадал
    } else {
        guessed = 0;
        kill(info->si_pid, SIGUSR2);  // Не угадал
    }
}

void handle_result_signal(int signo) {
    if (signo == SIGUSR1) {
        printf("Process %d: Correct guess!\n", getpid());
    } else if (signo == SIGUSR2) {
        printf("Process %d: Incorrect guess!\n", getpid());
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <N>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int N = atoi(argv[1]);
    if (N <= 0) {
        fprintf(stderr, "N must be a positive integer.\n");
        exit(EXIT_FAILURE);
    }

    pid_t pid;
    struct sigaction sa_guess, sa_result;

    // Настройка обработки сигналов для угадывания
    sa_guess.sa_flags = SA_SIGINFO;
    sa_guess.sa_sigaction = handle_guess_signal;
    sigemptyset(&sa_guess.sa_mask);
    sigaction(SIGRTMIN, &sa_guess, NULL);

    // Настройка обработки сигналов для результата
    sa_result.sa_flags = 0;
    sa_result.sa_handler = handle_result_signal;
    sigemptyset(&sa_result.sa_mask);
    sigaction(SIGUSR1, &sa_result, NULL);
    sigaction(SIGUSR2, &sa_result, NULL);

    if ((pid = fork()) == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Процесс угадывающий
        srand(time(NULL) ^ getpid());
        while (1) {
            sleep(1);  // Задержка для удобства наблюдения
            int guess = rand() % N + 1;
            printf("Process %d: Guessing %d\n", getpid(), guess);

            // Отправка предположения
            union sigval value;
            value.sival_int = guess;
            sigqueue(getppid(), SIGRTMIN, value);

            pause();  // Ожидание результата (SIGUSR1/SIGUSR2)
            if (guessed) {
                printf("Process %d: Number guessed successfully!\n", getpid());
                break;
            }
        }
        exit(0);
    } else {
        // Процесс загадывающий
        srand(time(NULL));
        for (int round = 1; round <= 10; round++) {
            number_to_guess = rand() % N + 1;
            printf("Process %d: Round %d, Number to guess: %d\n", getpid(), round, number_to_guess);

            pause();  // Ожидание предположения (SIGRTMIN)
        }

        printf("Process %d: Game over. Exiting.\n", getpid());
        kill(pid, SIGKILL);
    }

    return 0;
}

