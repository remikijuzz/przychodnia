#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include "config.h"

#define PATH "./src/"

pid_t director_pid, registration_pid, patient_pid;
pid_t doctor_pids[NUM_DOCTORS];

void send_signal_to_doctor(int doctor_id) {
    if (doctor_id < 0 || doctor_id >= NUM_DOCTORS) {
        printf("Błędne ID lekarza.\n");
        return;
    }
    kill(doctor_pids[doctor_id], SIGUSR1);
    printf("Dyrektor: Wysłano SIGUSR1 do lekarza ID %d (PID %d)\n", doctor_id, doctor_pids[doctor_id]);
}

void send_signal_to_all() {
    kill(registration_pid, SIGUSR2);
    kill(patient_pid, SIGUSR2);
    for (int i = 0; i < NUM_DOCTORS; i++) {
        kill(doctor_pids[i], SIGUSR2);
    }
    printf("Dyrektor: Wysłano SIGUSR2 do wszystkich procesów.\n");
}

int main(int argc, char *argv[]) {
    if (argc == 2 && argv[1][0] == '-' && argv[1][1] == 'd') {
        if (argv[1][2] == '\0') {
            // Opcja -d (zamknięcie całej przychodni)
            send_signal_to_all();
            exit(0);
        } else {
            // Opcja -dx (zamknięcie konkretnego lekarza)
            int doctor_id = argv[1][2] - '0';  // Pobiera numer lekarza
            if (doctor_id >= 0 && doctor_id < NUM_DOCTORS) {
                send_signal_to_doctor(doctor_id);
                exit(0);
            } else {
                printf("Błędne ID lekarza! Musi być od 0 do %d.\n", NUM_DOCTORS - 1);
                exit(1);
            }
        }
    }

    printf("Przychodnia otwarta od %d:00 do %d:00\n", CLINIC_OPEN_HOUR, CLINIC_CLOSE_HOUR);

    // Tworzenie procesu rejestracji
    registration_pid = fork();
    if (registration_pid == 0) {
        execl(PATH "registration", "registration", NULL);
        perror("Błąd uruchamiania procesu rejestracji");
        exit(EXIT_FAILURE);
    }

    // Tworzenie procesów lekarzy
    for (int i = 0; i < NUM_DOCTORS; i++) {
        doctor_pids[i] = fork();
        if (doctor_pids[i] == 0) {
            char doctor_id_str[10];
            sprintf(doctor_id_str, "%d", i);
            execl(PATH "doctor", "doctor", doctor_id_str, NULL);
            perror("Błąd uruchamiania procesu lekarza");
            exit(EXIT_FAILURE);
        }
    }

    // Tworzenie procesu generatora pacjentów
    patient_pid = fork();
    if (patient_pid == 0) {
        execl(PATH "patient", "patient", NULL);
        perror("Błąd uruchamiania generatora pacjentów");
        exit(EXIT_FAILURE);
    }

    // Tworzenie procesu dyrektora
    director_pid = fork();
    if (director_pid == 0) {
        execl(PATH "director", "director", NULL);
        perror("Błąd uruchamiania procesu dyrektora");
        exit(EXIT_FAILURE);
    }

    // Oczekiwanie na zakończenie pracy wszystkich procesów
    waitpid(registration_pid, NULL, 0);
    waitpid(patient_pid, NULL, 0);
    waitpid(director_pid, NULL, 0);
    for (int i = 0; i < NUM_DOCTORS; i++) {
        waitpid(doctor_pids[i], NULL, 0);
    }

    printf("Przychodnia zamknięta.\n");
    return 0;
}
