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
            printf("Wysłano sygnał %d do PID %d\n", sig, pid);
        }
    }
    fclose(fp);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Użycie: %s -s2\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    if (strcmp(argv[1], "-s2") == 0) {
        printf("Dyrektor: Wysyłam SIGUSR2 do głównego procesu (main)...\n");
        sendSignalToPID("main_pid.txt", SIGUSR2);
    } else {
        fprintf(stderr, "Nieprawidłowy argument. Użyj -s2.\n");
        exit(EXIT_FAILURE);
    }
    return 0;
}
