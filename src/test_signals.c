#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

// Globalne flagi sygnałów
volatile sig_atomic_t sigusr1_received = 0;
volatile sig_atomic_t sigterm_received = 0;
volatile sig_atomic_t sigusr2_received = 0;

void sigusr1_handler(int signum) {
    sigusr1_received = 1;
    printf("SIGUSR1 received: wykonuję czyszczenie zasobów specyficzne dla SIGUSR1.\n");
    // Można tu umieścić np. unlink("temp_resource.txt");
}

void sigterm_handler(int signum) {
    sigterm_received = 1;
    printf("SIGTERM received: wykonuję czyszczenie zasobów specyficzne dla SIGTERM.\n");
    // Można tu umieścić np. unlink("temp_resource.txt");
}

void sigusr2_handler(int signum) {
    sigusr2_received = 1;
    printf("SIGUSR2 received: wykonuję czyszczenie zasobów specyficzne dla SIGUSR2.\n");
    // Można tu umieścić np. unlink("temp_resource.txt");
}

int main(void) {
    struct sigaction sa;

    /* Ustawienie handlera dla SIGUSR1 */
    sa.sa_handler = sigusr1_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction SIGUSR1");
        exit(EXIT_FAILURE);
    }

    /* Ustawienie handlera dla SIGTERM */
    sa.sa_handler = sigterm_handler;
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction SIGTERM");
        exit(EXIT_FAILURE);
    }

    /* Ustawienie handlera dla SIGUSR2 */
    sa.sa_handler = sigusr2_handler;
    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("sigaction SIGUSR2");
        exit(EXIT_FAILURE);
    }

    // Symulacja utworzenia zasobu (np. IPC – tutaj tymczasowy plik)
    const char *temp_file = "temp_resource.txt";
    FILE *fp = fopen(temp_file, "w");
    if (!fp) {
        perror("fopen temp_resource.txt");
        exit(EXIT_FAILURE);
    }
    fprintf(fp, "Dane zasobu tymczasowego\n");
    fclose(fp);
    printf("Utworzono tymczasowy zasób: %s\n", temp_file);

    // Wysyłamy sygnały do siebie
    printf("Wysyłam SIGUSR1...\n");
    if (kill(getpid(), SIGUSR1) == -1) {
        perror("kill SIGUSR1");
        exit(EXIT_FAILURE);
    }
    sleep(1);

    printf("Wysyłam SIGTERM...\n");
    if (kill(getpid(), SIGTERM) == -1) {
        perror("kill SIGTERM");
        exit(EXIT_FAILURE);
    }
    sleep(1);

    printf("Wysyłam SIGUSR2...\n");
    if (kill(getpid(), SIGUSR2) == -1) {
        perror("kill SIGUSR2");
        exit(EXIT_FAILURE);
    }
    sleep(1);

    // Po otrzymaniu sygnałów, symulujemy czyszczenie zasobu – usuwamy plik
    if (sigusr1_received || sigterm_received || sigusr2_received) {
        if (unlink(temp_file) == -1) {
            perror("unlink temp_resource.txt");
        } else {
            printf("Zasób tymczasowy został usunięty.\n");
        }
    }

    // Sprawdzamy, czy wszystkie sygnały zostały odebrane
    if (sigusr1_received && sigterm_received && sigusr2_received) {
        printf("Wszystkie sygnały (SIGUSR1, SIGTERM, SIGUSR2) zostały poprawnie obsłużone.\n");
    } else {
        printf("Nie wszystkie sygnały zostały obsłużone poprawnie.\n");
    }

    return 0;
}
