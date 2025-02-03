#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include "patient.h"

#define MAX_PATIENTS_IN_BUILDING 50  
#define MAX_QUEUE_SIZE 20  

volatile bool running = true;
sem_t *building_capacity;
sem_t *registration_queue;

Patient queue[MAX_QUEUE_SIZE];
int queue_index = 0;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

void handle_sigusr1(int sig) {
    (void)sig;
    printf("Pacjenci: Nieczynne.\n");
    running = false;
}

void *patient_thread(void *arg) {
    (void)arg; 

    Patient p;
    p.id = rand() % 1000;
    p.is_vip = rand() % 5 == 0;
    p.target_doctor = rand() % 6;
    p.age = rand() % 90;

    printf("Nowy pacjent ID %d, lekarz %d, VIP: %d\n", p.id, p.target_doctor, p.is_vip);

    if (sem_trywait(building_capacity) == 0) {
        pthread_mutex_lock(&queue_mutex);
        if (queue_index < MAX_QUEUE_SIZE) {
            queue[queue_index++] = p;
            sem_post(registration_queue);
        }
        pthread_mutex_unlock(&queue_mutex);
    }

    pthread_exit(NULL);
}

int main() {
    signal(SIGUSR1, handle_sigusr1);

    while (running) {
        pthread_t patient_tid;
        pthread_create(&patient_tid, NULL, patient_thread, NULL);
        pthread_detach(patient_tid);
        sleep(1);
    }

    return 0;
}
