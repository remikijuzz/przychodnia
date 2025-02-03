#include "structs.h"
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

#define MAX_DOCTORS 6

volatile bool running = true;
pthread_t doctors[MAX_DOCTORS];

sem_t *poz_queue_sem;
sem_t *specialist_queue_sems[4];
sem_t *building_capacity;  // Nowe: zwalnianie miejsca w przychodni

pthread_mutex_t poz_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t specialist_mutexes[4];

const char *doctor_specializations[] = {
    "Lekarz POZ 1",
    "Lekarz POZ 2",
    "Lekarz Kardiolog",
    "Lekarz Okulista",
    "Lekarz Pediatra",
    "Lekarz Medycyny Pracy"
};

void handle_sigusr1(int sig) {
    (void)sig;
    printf("Lekarze: Kończę na dzisiaj.\n");
=======
#include <sys/msg.h>
#include <signal.h>

#define DOCTOR_QUEUE_KEY 2000

volatile bool running = true;

void handle_sigusr1(int sig) {
    (void)sig;
    printf("Lekarz: Kończę przyjmowanie pacjentów.\n");
>>>>>>> a8f733d (Rebuild projektu zgodnie z konsultacjami)
    running = false;

    for (int i = 0; i < 4; i++) {
        sem_post(specialist_queue_sems[i]);
    }
    sem_post(poz_queue_sem);
}

void handle_sigusr2(int sig) {
    (void)sig;
<<<<<<< HEAD
    printf("Lekarze: Dyrektor zarządził ewakuację, opuszczamy budynek.\n");
    exit(0);
}

void *doctor_thread(void *arg) {
    int doctor_id = *(int *)arg;
    free(arg);

    printf("%s rozpoczął pracę.\n", doctor_specializations[doctor_id]);

    while (running) {
        bool patient_found = false;

        if (doctor_id < 2) {
            if (sem_trywait(poz_queue_sem) == 0) {
                pthread_mutex_lock(&poz_mutex);
                patient_found = true;
                pthread_mutex_unlock(&poz_mutex);
            }
        } else {
            int spec_index = doctor_id - 2;
            if (sem_trywait(specialist_queue_sems[spec_index]) == 0) {
                pthread_mutex_lock(&specialist_mutexes[spec_index]);
                patient_found = true;
                pthread_mutex_unlock(&specialist_mutexes[spec_index]);
            }
        }

        if (patient_found) {
            printf("%s: Przyjmuje pacjenta.\n", doctor_specializations[doctor_id]);
            sleep(2);
            sem_post(building_capacity);  // **Nowe: zwalnianie miejsca w budynku**
        } else {
            sleep(1);
        }
    }

    printf("%s zakończył pracę.\n", doctor_specializations[doctor_id]);
    pthread_exit(NULL);
}

int main() {
    signal(SIGUSR1, handle_sigusr1);
    signal(SIGUSR2, handle_sigusr2);

    printf("Lekarze rozpoczynają pracę...\n");

    poz_queue_sem = sem_open("/poz_queue", O_CREAT, 0666, 0);
    building_capacity = sem_open("/building_capacity", O_CREAT, 0666, 50);
    
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

    sem_close(poz_queue_sem);
    sem_unlink("/poz_queue");
    sem_close(building_capacity);
    sem_unlink("/building_capacity");

    for (int i = 0; i < 4; i++) {
        sem_close(specialist_queue_sems[i]);
        sem_unlink("/spec_queue");
    }

    printf("Wszyscy lekarze zakończyli pracę.\n");
=======
    printf("Lekarz: Natychmiastowa ewakuacja!\n");
    exit(0);
}

int main(int argc, char *argv[]) {
    (void)argc;
    signal(SIGUSR1, handle_sigusr1);
    signal(SIGUSR2, handle_sigusr2);

    int doctor_id = atoi(argv[1]);
    int msgid = msgget(DOCTOR_QUEUE_KEY + doctor_id, 0666 | IPC_CREAT);
    
    Message msg;
    while (running) {
        if (msgrcv(msgid, &msg, sizeof(Patient), 0, IPC_NOWAIT) > 0) {
            printf("Lekarz %d: Przyjmuję pacjenta %d\n", doctor_id, msg.patient.id);
            sleep(2);
        }
    }

    printf("Lekarz %d zakończył pracę.\n", doctor_id);
>>>>>>> a8f733d (Rebuild projektu zgodnie z konsultacjami)
    return 0;
}
