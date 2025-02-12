#define _POSIX_C_SOURCE 200809L
//director.c

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

void sendSignalToPID(const char *filename, int sig) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Błąd przy otwieraniu pliku z PID");
        exit(EXIT_FAILURE);
    }
    int pid;
    if (fscanf(fp, "%d", &pid) == 1) {
        if (kill(pid, sig) == -1) {
            perror("Błąd przy wysyłaniu sygnału");
        } else {
            printf("Dyrektor: Wysłano sygnał %d do PID %d\n", sig, pid);
        }
    }
    fclose(fp);
}

int main(int argc, char *argv[]) {
    if (argc != 2 && argc != 3) { // Teraz 2 LUB 3 argumenty
        fprintf(stderr, "Użycie: %s -s2 | %s -s1 <rola>\n", argv[0], argv[0]); // Dodane użycie dla -s1
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "-s2") == 0) {
        printf("Dyrektor: Wysyłam SIGUSR2 do głównego procesu (main)...\n");
        sendSignalToPID("main_pid.txt", SIGUSR2);
    } else if (strcmp(argv[1], "-s1") == 0) { // Obsługa -s1
        if (argc != 3) {
            fprintf(stderr, "Użycie: %s -s1 <rola>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        char *role = argv[2];
        char filename[50];
        snprintf(filename, sizeof(filename), "%s_pid.txt", role); // Nazwa pliku PID na podstawie roli (np. "poz_pid.txt")
        printf("Dyrektor: Wysyłam SIGUSR1 do lekarza o roli %s...\n", role);
        sendSignalToPID(filename, SIGUSR1); // Wyślij SIGUSR1
    } else {
        fprintf(stderr, "Nieprawidłowy argument. Użyj -s2 lub -s1 <rola>.\n"); // Zaktualizowany komunikat o błędzie
        exit(EXIT_FAILURE);
    }
    return 0;
}