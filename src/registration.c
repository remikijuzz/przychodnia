#include "structs.h"
#include <sys/msg.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define REGISTRATION_QUEUE_KEY 1234
#define DOCTOR_QUEUE_KEY 2000
#define MAX_REGISTRATION_WINDOWS 2

pthread_t registration_threads[MAX_REGISTRATION_WINDOWS];
volatile bool running = true;
sem_t available_windows;

// Obsługa sygnału SIGUSR1 - zamknięcie rejestracji
void handle_sigusr1(int sig) {
    (void)sig;
    printf("Rejestracja: Zamykamy rejestrację.\n");
    running = false;
}

// Obsługa sygnału SIGUSR2 - ewakuacja
void handle_sigusr2(int sig) {
    (void)sig;
    printf("Rejestracja: Ewakuacja! Wszyscy pacjenci muszą opuścić budynek.\n");
    exit(0);
}

// Funkcja obsługi rejestracji pacjentów
void *registration_handler(void *arg) {
    int msgid = msgget(REGISTRATION_QUEUE_KEY, 0666 | IPC_CREAT);
    Message msg;

    while (running) {
        if (msgrcv(msgid, &msg, sizeof(Patient), 0, IPC_NOWAIT) > 0) {
            printf("Rejestracja: Pacjent %d zarejestrowany, kierujemy do lekarza %d\n",
                   msg.patient.id, msg.patient.target_doctor);

            // Sprawdzenie, czy lekarz ma miejsce
            int doctor_queue_id = msgget(DOCTOR_QUEUE_KEY + msg.patient.target_doctor, 0666 | IPC_CREAT);
            msgsnd(doctor_queue_id, &msg, sizeof(Patient), 0);
        }
        usleep(50000);  // Krótka pauza dla optymalizacji CPU
    }
    pthread_exit(NULL);
}

int main() {
    signal(SIGUSR1, handle_sigusr1);
    signal(SIGUSR2, handle_sigusr2);

    sem_init(&available_windows, 0, MAX_REGISTRATION_WINDOWS);
    printf("Rejestracja: Rozpoczynamy pracę.\n");

    for (int i = 0; i < MAX_REGISTRATION_WINDOWS; i++) {
        pthread_create(&registration_threads[i], NULL, registration_handler, NULL);
    }

    for (int i = 0; i < MAX_REGISTRATION_WINDOWS; i++) {
        pthread_join(registration_threads[i], NULL);
    }

    msgctl(msgget(REGISTRATION_QUEUE_KEY, 0666), IPC_RMID, NULL);
    printf("Rejestracja zakończyła pracę.\n");
    return 0;
}
