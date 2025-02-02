#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "config.h"
#include "patient.h"
#include "doctor.h"
#include "registration.h"

volatile bool running = true;
pid_t doctor_pids[NUM_DOCTORS];  // Tablica PID-ów lekarzy
pid_t registration_pid;           // PID procesu rejestracji
pid_t patient_pid;     

void handle_sigint(int sig) {
    (void)sig;
    printf("Dyrektor: Otrzymano SIGINT – zamykam przychodnię.\n");
    running = false;
}

void send_signal_to_doctor(int doctor_id) {
    if (doctor_id < 0 || doctor_id >= NUM_DOCTORS) {
        printf("Dyrektor: Niepoprawne ID lekarza.\n");
        return;
    }
    printf("Dyrektor: Wysyłam SIGUSR1 do %d (PID %d)...\n", doctor_id, doctor_pids[doctor_id]);
    kill(doctor_pids[doctor_id], SIGUSR1);
}

void close_clinic() {
    printf("\nDyrektor: Informuję pacjentów i lekarzy, że przychodnia jest zamykana – proszę opuścić budynek.\n");

    // Zamknięcie generatora pacjentów
    kill(patient_pid, SIGUSR2);
    sleep(1);

    // Zamknięcie rejestracji
    kill(registration_pid, SIGUSR2);
    sleep(1);

    // Zamknięcie lekarzy
    for (int i = 0; i < NUM_DOCTORS; i++) {
        kill(doctor_pids[i], SIGUSR2);
    }
    sleep(2);

    printf("Dyrektor: Przychodnia została zamknięta – wszyscy pacjenci i lekarze opuścili budynek.\n");
    exit(0);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, handle_sigint);

    if (argc > 1) {
        if (argv[1][0] == '-' && argv[1][1] == 'd') {
            if (argv[1][2] >= '0' && argv[1][2] <= '9') {
                int doctor_id = argv[1][2] - '0';
                send_signal_to_doctor(doctor_id);
                return 0;
            } else {
                close_clinic();
            }
        }
    }

    printf("Dyrektor: Przychodnia działa normalnie.\n");

    while (running) {
        sleep(1);
    }

    return 0;
}
