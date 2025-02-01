#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "config.h"

void handle_signal(int sig) {
    if (sig == SIGUSR1) {
        printf("Dyrektor: Polecenie do zamknięcia lekarza!\n");
        // Tu dodamy wysyłanie sygnału do lekarza
    } else if (sig == SIGUSR2) {
        printf("Dyrektor: Wszyscy pacjenci muszą opuścić przychodnię!\n");
        // Tu dodamy wysyłanie sygnału do pacjentów
    }
}

int main() {
    printf("Dyrektor uruchomiony\n");

    // Ustawienie obsługi sygnałów
    signal(SIGUSR1, handle_signal);
    signal(SIGUSR2, handle_signal);

    while (1) {
        pause(); // Oczekiwanie na sygnał
    }

    return 0;
}
