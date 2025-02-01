#include "director.h"
#include <stdio.h>
#include <stdlib.h>

// Funkcja wysyłająca sygnał 1 do wybranego lekarza
void send_signal_to_doctor(pid_t doctor_pid) {
    if (kill(doctor_pid, SIGUSR1) == -1) {
        perror("Błąd podczas wysyłania sygnału do lekarza");
        exit(EXIT_FAILURE);
    }
    printf("Dyrektor wysłał sygnał 1 do lekarza (PID: %d).\n", doctor_pid);
}

// Funkcja wysyłająca sygnał 2 do wszystkich pacjentów
void send_signal_to_all_patients(pid_t *patient_pids, int num_patients) {
    for (int i = 0; i < num_patients; i++) {
        if (kill(patient_pids[i], SIGUSR2) == -1) {
            perror("Błąd podczas wysyłania sygnału do pacjenta");
            exit(EXIT_FAILURE);
        }
        printf("Dyrektor wysłał sygnał 2 do pacjenta (PID: %d).\n", patient_pids[i]);
    }
}