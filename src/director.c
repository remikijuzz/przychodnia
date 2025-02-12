#include "clinic.h"
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
        if (kill(pid, sig) == -1)
            perror("Błąd przy wysyłaniu sygnału");
        else
            printf("Dyrektor: Wysłano sygnał %d do PID %d\n", sig, pid);
    } else {
        fprintf(stderr, "Nie udało się odczytać PID z pliku %s\n", filename);
    }
    fclose(fp);
}

int main(int argc, char *argv[]) {
    if (argc != 2 && argc != 3) {
        fprintf(stderr, "Usage: %s -s2 | %s -s1 <rola>\n", argv[0], argv[0]);
        exit(EXIT_FAILURE);
    }
    if (strcmp(argv[1], "-s2") == 0) {
        printf("Dyrektor: Wysyłam SIGUSR2 do głównego procesu (main)...\n");
        sendSignalToPID("main_pid.txt", SIGUSR2);
    } else if (strcmp(argv[1], "-s1") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Usage: %s -s1 <rola>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        char *role = argv[2];
        char filename[50];
        /* Dla ról "poz1" lub "poz2" budujemy nazwę pliku */
        if (strcasecmp(role, "poz1") == 0)
            snprintf(filename, sizeof(filename), "poz1_pid.txt");
        else if (strcasecmp(role, "poz2") == 0)
            snprintf(filename, sizeof(filename), "poz2_pid.txt");
        else if (strcasecmp(role, "POZ") == 0)
            snprintf(filename, sizeof(filename), "POZ_pid.txt");
        else
            snprintf(filename, sizeof(filename), "%s_pid.txt", role);
        printf("Dyrektor: Wysyłam SIGUSR1 do lekarza o roli %s...\n", role);
        sendSignalToPID(filename, SIGUSR1);
    } else {
        fprintf(stderr, "Nieprawidłowy argument. Użyj -s2 lub -s1 <rola>.\n");
        exit(EXIT_FAILURE);
    }
    return 0;
}
