#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "config.h"
#include "registration.h"
#include "doctor.h"

// Semafor do kontroli liczby pacjentów w budynku
sem_t building_capacity;

void* patient_generator(void* arg) {
    for (int hour = CLINIC_OPEN_HOUR; hour <= CLINIC_CLOSE_HOUR; hour++) {
        printf("Godzina %d:00 - Pacjenci przychodzą.\n", hour);
        
        for (int i = 0; i < 10; i++) { // 10 pacjentów na godzinę
            Patient p = { rand() % 70 + 1, rand() % 2, rand() % NUM_DOCTORS, rand() % 90 };
            add_patient_to_queue(p);
        }

        sleep(1);
    }
    return NULL;
}

int main() {
    printf("Przychodnia otwarta od %d:00 do %d:00.\n", CLINIC_OPEN_HOUR, CLINIC_CLOSE_HOUR);
    
    init_registration();
    init_doctors();

    sem_init(&building_capacity, 0, MAX_PATIENTS_IN_BUILDING);

    pthread_t patient_thread;
    pthread_create(&patient_thread, NULL, patient_generator, NULL);

    pthread_join(patient_thread, NULL);

    close_registration();
    printf("Przychodnia zamknięta.\n");

    return 0;
}
