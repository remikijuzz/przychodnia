#include "structs.h"
#include <sys/msg.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
<<<<<<< HEAD
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include "patient.h"

#define MAX_QUEUE_SIZE 20  
=======
#include <signal.h>

#define REGISTRATION_QUEUE_KEY 1234
#define DOCTOR_QUEUE_KEY 2000
#define MAX_REGISTRATION_WINDOWS 2
>>>>>>> a8f733d (Rebuild projektu zgodnie z konsultacjami)

pthread_t registration_threads[MAX_REGISTRATION_WINDOWS];
volatile bool running = true;
<<<<<<< HEAD
sem_t *registration_queue;
sem_t *building_capacity;
sem_t *poz_queue_sem;
sem_t *specialist_queue_sems[4];

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
Patient queue[MAX_QUEUE_SIZE];
int queue_index = 0;
=======
sem_t available_windows;
>>>>>>> a8f733d (Rebuild projektu zgodnie z konsultacjami)

// Obsługa sygnału SIGUSR1 - zamknięcie rejestracji
void handle_sigusr1(int sig) {
    (void)sig;
<<<<<<< HEAD
    printf("Rejestracja: Przychodnia została zamknięta.\n");
=======
    printf("Rejestracja: Zamykamy rejestrację.\n");
>>>>>>> a8f733d (Rebuild projektu zgodnie z konsultacjami)
    running = false;
    sem_post(registration_queue);
}

// Obsługa sygnału SIGUSR2 - ewakuacja
void handle_sigusr2(int sig) {
    (void)sig;
<<<<<<< HEAD
    printf("Rejestracja: Dyrektor zarządził ewakuację, opuszczamy przychodnię.\n");
    exit(0);
}

void *registration_thread(void *arg) {
    (void)arg;
    while (running) {
        sem_wait(registration_queue);

        pthread_mutex_lock(&queue_mutex);
        if (queue_index > 0) {
            Patient p = queue[--queue_index];
            printf("Rejestracja: Pacjent ID %d zarejestrowany!\n", p.id);

            if (p.target_doctor < 2) {
                sem_post(poz_queue_sem);
            } else {
                sem_post(specialist_queue_sems[p.target_doctor - 2]);
            }
        }
        pthread_mutex_unlock(&queue_mutex);
=======
    printf("Rejestracja: Ewakuacja! Wszyscy pacjenci muszą opuścić budynek.\n");
    exit(0);
}

// Funkcja obsługi rejestracji pacjentów
void *registration_handler(void *arg) {
    (void)arg;
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
>>>>>>> a8f733d (Rebuild projektu zgodnie z konsultacjami)
    }
    pthread_exit(NULL);
}

int main() {
    signal(SIGUSR1, handle_sigusr1);
    signal(SIGUSR2, handle_sigusr2);

<<<<<<< HEAD
    registration_queue = sem_open("/registration_queue", O_CREAT, 0666, 0);
    building_capacity = sem_open("/building_capacity", O_CREAT, 0666, MAX_QUEUE_SIZE);
    poz_queue_sem = sem_open("/poz_queue", O_CREAT, 0666, 0);

    for (int i = 0; i < 4; i++) {
        specialist_queue_sems[i] = sem_open("/spec_queue", O_CREAT, 0666, 0);
    }

    pthread_t reg_thread;
    pthread_create(&reg_thread, NULL, registration_thread, NULL);
    pthread_join(reg_thread, NULL);

    sem_close(registration_queue);
    sem_unlink("/registration_queue");
    sem_close(building_capacity);
    sem_unlink("/building_capacity");

=======
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
>>>>>>> a8f733d (Rebuild projektu zgodnie z konsultacjami)
    return 0;
}
