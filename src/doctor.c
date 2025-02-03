#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include "patient.h"

#define MAX_DOCTORS 6
#define MAX_QUEUE_SIZE 20

volatile bool running = true;
pthread_t doctors[MAX_DOCTORS];

// Semafory kolejek pacjentów
sem_t *poz_queue_sem;
sem_t *specialist_queue_sems[4];

pthread_mutex_t poz_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t specialist_mutexes[4];

Patient poz_queue[MAX_QUEUE_SIZE];
Patient specialist_queues[4][MAX_QUEUE_SIZE];

int poz_queue_size = 0;
int specialist_queue_sizes[4] = {0};

const char *doctor_specializations[] = {
    "Lekarz POZ 1",
    "Lekarz POZ 2",
    "Lekarz Kardiolog",
    "Lekarz Okulista",
    "Lekarz Pediatra",
    "Lekarz Medycyny Pracy"
};

// Obsługa sygnałów
void handle_sigusr1(int sig) {
    (void)sig;
    printf("Lekarze: Kończę na dzisiaj.\n");
    running = false;
}

void handle_sigusr2(int sig) {
    (void)sig;
    printf("Lekarze: Dyrektor zarządził ewakuację, opuszczamy budynek.\n");
    exit(0);
}

// Funkcja obsługi wątku lekarza
void *doctor_thread(void *arg) {
    int doctor_id = *(int *)arg;
    free(arg);

    printf("%s rozpoczął pracę.\n", doctor_specializations[doctor_id]);

    while (running) {
        Patient p;
        bool patient_found = false;

        if (doctor_id < 2) {  // Lekarze POZ mają wspólną kolejkę
            sem_wait(poz_queue_sem);  
            pthread_mutex_lock(&poz_mutex);
            if (poz_queue_size > 0) {
                p = poz_queue[--poz_queue_size];
                patient_found = true;
            }
            pthread_mutex_unlock(&poz_mutex);
        } else {  
            int spec_index = doctor_id - 2;
            sem_wait(specialist_queue_sems[spec_index]);  
            pthread_mutex_lock(&specialist_mutexes[spec_index]);
            if (specialist_queue_sizes[spec_index] > 0) {
                p = specialist_queues[spec_index][--specialist_queue_sizes[spec_index]];
                patient_found = true;
            }
            pthread_mutex_unlock(&specialist_mutexes[spec_index]);
        }

        if (patient_found) {
            printf("%s: Przyjmuje pacjenta ID %d\n", doctor_specializations[doctor_id], p.id);
            sleep(2);  
        }
    }

    printf("%s zakończył pracę.\n", doctor_specializations[doctor_id]);
    pthread_exit(NULL);
}

int main() {
    signal(SIGUSR1, handle_sigusr1);
    signal(SIGUSR2, handle_sigusr2);

    printf("Uruchamianie lekarzy...\n");

    poz_queue_sem = sem_open("/poz_queue", O_CREAT, 0666, 0);
    for (int i = 0; i < 4; i++) {
        specialist_queue_sems[i] = sem_open("/spec_queue", O_CREAT, 0666, 0);
        pthread_mutex_init(&specialist_mutexes[i], NULL);
    }

    for (int i = 0; i < MAX_DOCTORS; i++) {
        int *id = malloc(sizeof(int));
        *id = i;
        pthread_create(&doctors[i], NULL, doctor_thread, id);
    }

    for (int i = 0; i < MAX_DOCTORS; i++) {
        pthread_join(doctors[i], NULL);
    }

    printf("Wszyscy lekarze zakończyli pracę.\n");
    return 0;
}
