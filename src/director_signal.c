#include <signal.h>
#include <unistd.h>
#include <stdio.h>

int main() {
    int pid;
    printf("Podaj PID procesu przychodni: ");
    scanf("%d", &pid);

    printf("Wysyłanie sygnału SIGUSR1 (zamknięcie rejestracji)...\n");
    kill(pid, SIGUSR1);

    sleep(1); // Czekamy chwilę
    printf("Wysyłanie sygnału SIGUSR2 (zamknięcie przychodni)...\n");
    kill(pid, SIGUSR2);

    return 0;
}
