#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    // Pobranie PID procesu przychodni za pomocą pgrep
    FILE* cmd = popen("pgrep przychodnia", "r"); // 'przychodnia' to nazwa twojego procesu
    if (!cmd) {
        perror("Nie można wykonać pgrep");
        return 1;
    }

    int pid;
    if (fscanf(cmd, "%d", &pid) != 1) {
        printf("Nie znaleziono procesu przychodni!\n");
        pclose(cmd);
        return 1;
    }
    pclose(cmd);

    printf("Znaleziono PID procesu przychodni: %d\n", pid);

    // Wysłanie SIGUSR1 - zamknięcie rejestracji
    printf("Wysyłanie sygnału SIGUSR1 (zamknięcie rejestracji)...\n");
    kill(pid, SIGUSR1);

    sleep(1); // Czekamy chwilę

    // Wysłanie SIGUSR2 - zamknięcie przychodni
    printf("Wysyłanie sygnału SIGUSR2 (zamknięcie przychodni)...\n");
    kill(pid, SIGUSR2);

    return 0;
}
