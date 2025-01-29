#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include "config.h"
#include "registration.h"
#include "doctor.h"
#include <signal.h>
#include <stdbool.h>

// Semafor do kontroli liczby pacjentów w budynku
sem_t building_capacity;

bool clinic_open = true;   // Czy przychodnia przyjmuje pacjentów?
bool clinic_closing = false; // Czy zamykamy przychodnię?

pthread_mutex_t clinic_mutex = PTHREAD_MUTEX_INITIALIZER;


void* patient_generator(void* arg) {
    (void)arg;
    for (int hour = CLINIC_OPEN_HOUR; hour <= CLINIC_CLOSE_HOUR; hour++) {
        pthread_mutex_lock(&clinic_mutex);
        if (clinic_closing) {  // Jeśli rejestracja jest zamknięta, przerywamy generowanie pacjentów
            pthread_mutex_unlock(&clinic_mutex);
            printf("Rejestracja zamknięta - nowi pacjenci nie są przyjmowani.\n");
            break;
        }
        pthread_mutex_unlock(&clinic_mutex);

        printf("Godzina %d:00 - Pacjenci przychodzą.\n", hour);
        
        for (int i = 0; i < 10; i++) { // 10 pacjentów na godzinę
            pthread_mutex_lock(&clinic_mutex);
            if (clinic_closing) {  // Sprawdzamy ponownie wewnątrz pętli
                pthread_mutex_unlock(&clinic_mutex);
                printf("Rejestracja zamknięta - reszta pacjentów nie jest przyjmowana.\n");
                break;
            }
            pthread_mutex_unlock(&clinic_mutex);

            Patient p = { rand() % 70 + 1, rand() % 2, rand() % NUM_DOCTORS, rand() % 90 };
            add_patient_to_queue(p);
        }

        sleep(1);  // Symulacja godziny
    }
    return NULL;
}


void* doctor_thread(void* arg);

void handle_director_signal(int sig) {
    pthread_mutex_lock(&clinic_mutex);
    if (sig == SIGUSR1) {
        printf("\n[Dyrektor]: Rejestracja zamknięta, nie przyjmujemy nowych pacjentów.\n");
        clinic_closing = true;  // Nowi pacjenci nie są już rejestrowani
    } else if (sig == SIGUSR2) {
        printf("\n[Dyrektor]: Natychmiastowe zamknięcie przychodni! Pacjenci i lekarze wychodzą.\n");
        clinic_open = false;  // Zmuszamy wszystkich do opuszczenia przychodni
    }
    pthread_mutex_unlock(&clinic_mutex);
}


int main() {
    signal(SIGUSR1, handle_director_signal);
    signal(SIGUSR2, handle_director_signal);

    printf("Przychodnia otwarta od %d:00 do %d:00\n", CLINIC_OPEN_HOUR, CLINIC_CLOSE_HOUR);
    
    pthread_t doctor_threads[NUM_DOCTORS];

    // Inicjalizacja lekarzy PRZED rejestracją pacjentów
    for (int i = 0; i < NUM_DOCTORS; i++) {
        int* doctor_id = malloc(sizeof(int));
        *doctor_id = i;
        pthread_create(&doctor_threads[i], NULL, doctor_thread, doctor_id);
    }

    init_registration();
    init_doctors();

    sem_init(&building_capacity, 0, MAX_PATIENTS_IN_BUILDING);

    pthread_t patient_thread;
    pthread_create(&patient_thread, NULL, patient_generator, NULL);

    pthread_join(patient_thread, NULL);

    // Zamknięcie rejestracji
    close_registration();
    printf("[INFO]: Rejestracja zamknięta.\n");

    pthread_mutex_lock(&clinic_mutex);
    clinic_open = false;
    pthread_mutex_unlock(&clinic_mutex);


    // Obudzenie lekarzy, żeby sprawdzili kolejkę
    pthread_cond_broadcast(&queue_not_empty);

    // Czekamy na lekarzy
    for (int i = 0; i < NUM_DOCTORS; i++) {
        pthread_join(doctor_threads[i], NULL);
    }
    

    printf("Przychodnia zamknięta, wszyscy pacjenci i lekarze opuścili budynek.\n");
    return 0;
}