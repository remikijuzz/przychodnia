#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "config.h"

#define PATH "./src/"

int main() {
    printf("Przychodnia otwarta od %d:00 do %d:00\n", CLINIC_OPEN_HOUR, CLINIC_CLOSE_HOUR);

    pid_t director_pid, registration_pid, patient_pid;
    pid_t doctor_pids[NUM_DOCTORS];

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
