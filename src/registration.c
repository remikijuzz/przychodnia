#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include "patient.h"

#define MAX_QUEUE_SIZE 20  

volatile bool running = true;
sem_t *registration_queue;
sem_t *building_capacity;
sem_t *poz_queue_sem;
sem_t *specialist_queue_sems[4];

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
Patient queue[MAX_QUEUE_SIZE];
int queue_index = 0;

void handle_sigusr1(int sig) {
    (void)sig;
    printf("Rejestracja: Przychodnia została zamknięta.\n");
    running = false;
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

            sem_post(building_capacity);
        }
        pthread_mutex_unlock(&queue_mutex);
    }
    pthread_exit(NULL);
}

int main() {
    signal(SIGUSR1, handle_sigusr1);

    registration_queue = sem_open("/registration_queue", O_CREAT, 0666, 0);
    building_capacity = sem_open("/building_capacity", O_CREAT, 0666, MAX_QUEUE_SIZE);
    poz_queue_sem = sem_open("/poz_queue", O_CREAT, 0666, 0);
    for (int i = 0; i < 4; i++) {
        specialist_queue_sems[i] = sem_open("/spec_queue", O_CREAT, 0666, 0);
    }

    pthread_t reg_thread;
    pthread_create(&reg_thread, NULL, registration_thread, NULL);
    pthread_join(reg_thread, NULL);

    printf("Rejestracja zakończyła pracę.\n");
    return 0;
}
