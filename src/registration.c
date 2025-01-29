#include <stdio.h>
#include <pthread.h>
#include "registration.h"
#include "doctor.h"

#define QUEUE_SIZE 100
static Patient queue[QUEUE_SIZE];
static int front = 0, rear = 0;
static pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_not_empty = PTHREAD_COND_INITIALIZER;

void init_registration() {
    front = rear = 0;
    printf("Rejestracja zainicjalizowana.\n");
}

void close_registration() {
    printf("Rejestracja zamknięta.\n");
}

// Dodawanie pacjenta do kolejki
void add_patient_to_queue(Patient patient) {
    pthread_mutex_lock(&queue_mutex);

    queue[rear] = patient;
    rear = (rear + 1) % QUEUE_SIZE;
    printf("Pacjent ID %d dodany do kolejki.\n", patient.id);

    pthread_cond_signal(&queue_not_empty); // Informacja, że kolejka nie jest pusta
    pthread_mutex_unlock(&queue_mutex);
}

// Pobieranie pacjenta z kolejki
Patient process_next_patient() {
    pthread_mutex_lock(&queue_mutex);

    while (front == rear) { // Kolejka pusta
        pthread_cond_wait(&queue_not_empty, &queue_mutex);
    }

    Patient patient = queue[front];
    front = (front + 1) % QUEUE_SIZE;

    // Sprawdzenie, czy wybrany lekarz może przyjąć pacjenta
    Doctor* doctor = get_doctor_by_id(patient.target_doctor);
    if (doctor && doctor->patients_seen >= doctor->max_patients) {
        printf("Pacjent ID %d nie może zostać przyjęty przez lekarza ID %d (limit osiągnięty).\n",
               patient.id, patient.target_doctor);
        pthread_mutex_unlock(&queue_mutex);
        return (Patient){-1, false, -1, -1, false};// Pacjent nieobsłużony
    }

    pthread_mutex_unlock(&queue_mutex);
    return patient;
}
