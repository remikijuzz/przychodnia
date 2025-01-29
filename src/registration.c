#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "registration.h"

static Patient queue[QUEUE_SIZE];
static int front = 0, rear = 0;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_not_empty = PTHREAD_COND_INITIALIZER;

void init_registration() {
    printf("Rejestracja rozpoczęta.\n");
}

void add_patient_to_queue(Patient patient) {
    pthread_mutex_lock(&queue_mutex);
    queue[rear] = patient;
    rear = (rear + 1) % QUEUE_SIZE;
    printf("Pacjent %d dodany do kolejki.\n", patient.id);
    pthread_cond_signal(&queue_not_empty);
    pthread_mutex_unlock(&queue_mutex);
}

Patient process_next_patient() {
    pthread_mutex_lock(&queue_mutex);
    if (front == rear) {
        pthread_mutex_unlock(&queue_mutex);
        return (Patient){ -1, false, -1, 0 };
    }

    Patient patient = queue[front];
    front = (front + 1) % QUEUE_SIZE;
    pthread_mutex_unlock(&queue_mutex);
    return patient;
}

int is_queue_empty() {
    return front == rear;  // Kolejka jest pusta, jeśli front == rear
}

void close_registration() {
    printf("Rejestracja zamknięta.\n");
}
