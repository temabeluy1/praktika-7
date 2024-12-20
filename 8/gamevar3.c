#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

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

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Процесс угадывающий
        close(pipefd[0]); // Закрываем чтение

        srand(time(NULL) ^ getpid());
        int attempts = 0;
        while (1) {
            int guess = rand() % N + 1;
            attempts++;

            // Отправляем предположение в pipe
            write(pipefd[1], &guess, sizeof(guess));

            // Читаем результат из pipe
            int result;
            read(pipefd[1], &result, sizeof(result));

            if (result == 1) {
                printf("Process %d: Number guessed successfully in %d attempts!\n", getpid(), attempts);
                break;
            } else {
                printf("Process %d: Guessed %d, but it was incorrect.\n", getpid(), guess);
            }
        }

        close(pipefd[1]);
        exit(0);
    } else {
        // Процесс загадывающий
        close(pipefd[1]); // Закрываем запись

        srand(time(NULL));
        int number_to_guess = rand() % N + 1;
        printf("Process %d: Number to guess: %d\n", getpid(), number_to_guess);

        int total_attempts = 0;

        while (1) {
            // Читаем предположение из pipe
            int guess;
            read(pipefd[0], &guess, sizeof(guess));

            total_attempts++;

            // Проверяем предположение
            int result = (guess == number_to_guess) ? 1 : 0;

            // Отправляем результат обратно в pipe
            write(pipefd[0], &result, sizeof(result));

            if (result == 1) {
                printf("Process %d: Number %d guessed correctly after %d attempts!\n", getpid(), number_to_guess, total_attempts);
                number_to_guess = rand() % N + 1;
                printf("Process %d: New number to guess: %d\n", getpid(), number_to_guess);
                total_attempts = 0;
            }
        }

        close(pipefd[0]);
    }

    return 0;
}

