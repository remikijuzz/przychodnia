#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "config.h"

pid_t registration_pid;
pid_t patient_pid;
pid_t doctor_pids[NUM_DOCTORS];

volatile bool running = true;

void handle_sigusr1(int sig) {
    (void)sig;
    printf("\nDyrektor: Jest godzina %02d:00 – zamykamy przychodnię.\n", CLINIC_CLOSE_HOUR);
    printf("Dyrektor: Pacjenci i lekarze, proszę opuścić budynek.\n");

    if (patient_pid > 0) {
        kill(patient_pid, SIGUSR1);
    }
    sleep(1);

    if (registration_pid > 0) {
        kill(registration_pid, SIGUSR1);
    }
    sleep(1);

    for (int i = 0; i < NUM_DOCTORS; i++) {
        if (doctor_pids[i] > 0) {
            kill(doctor_pids[i], SIGUSR1);
        }
    }
    sleep(2);

    printf("Dyrektor: Przychodnia została zamknięta – wszyscy pacjenci i lekarze opuścili budynek.\n");
    running = false;
}

void handle_sigusr2(int sig) {
    (void)sig;
    printf("\nDyrektor: Zarządzam ewakuację placówki, proszę opuścić budynek.\n");

    if (patient_pid > 0) {
        kill(patient_pid, SIGUSR2);
    }
    sleep(1);

    if (registration_pid > 0) {
        kill(registration_pid, SIGUSR2);
    }
    sleep(1);

    for (int i = 0; i < NUM_DOCTORS; i++) {
        if (doctor_pids[i] > 0) {
            kill(doctor_pids[i], SIGUSR2);
        }
    }
    sleep(2);

    printf("Dyrektor: Przychodnia została zamknięta – wszyscy pacjenci i lekarze opuścili budynek.\n");
    running = false;
}

void handle_sigint(int sig) {
    (void)sig;
    printf("Dyrektor: Otrzymano SIGINT – kończę działanie.\n");
    running = false;
}

int main(int argc, char *argv[]) {
    if (argc < NUM_DOCTORS + 3) {
        fprintf(stderr, "Błąd: Niewystarczająca liczba argumentów dla dyrektora\n");
        exit(EXIT_FAILURE);
    }

    registration_pid = atoi(argv[1]);
    patient_pid = atoi(argv[2]);

    for (int i = 0; i < NUM_DOCTORS; i++) {
        doctor_pids[i] = atoi(argv[i + 3]);
    }

    signal(SIGUSR1, handle_sigusr1);
    signal(SIGUSR2, handle_sigusr2);
    signal(SIGINT, handle_sigint);

    printf("Dyrektor: Rozpoczynam pracę.\n");

    while (running) {
        sleep(1);
    }

    printf("Dyrektor: Kończę pracę.\n");
    return 0;
}
