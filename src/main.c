#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "config.h"
#include "registration.h"
#include "doctor.h"

// Prototypy funkcji wątków
void* doctor_thread(void* arg);
void* patient_generator(void* arg);

// Semafor kontrolujący liczbę pacjentów w budynku
sem_t building_capacity;

int main() {
    printf("Przychodnia otwarta od %d:00 do %d:00.\n", CLINIC_OPEN_HOUR, CLINIC_CLOSE_HOUR);

    // Inicjalizacja rejestracji i lekarzy
    init_registration();
    init_doctors();

    // Inicjalizacja semafora
    sem_init(&building_capacity, 0, MAX_PATIENTS_IN_BUILDING);

    // Tworzenie wątków dla lekarzy
    pthread_t doctor_threads[NUM_DOCTORS];
    for (int i = 0; i < NUM_DOCTORS; i++) {
        pthread_create(&doctor_threads[i], NULL, doctor_thread, (void*)(long)i);
    }

    // Tworzenie wątku generatora pacjentów
    pthread_t generator_thread;
    pthread_create(&generator_thread, NULL, patient_generator, NULL);

    // Dołączenie wątków
    pthread_join(generator_thread, NULL);
    for (int i = 0; i < NUM_DOCTORS; i++) {
        pthread_join(doctor_threads[i], NULL);
    }

    // Usunięcie semafora
    sem_destroy(&building_capacity);

    // Zamknięcie rejestracji
    close_registration();

    // Raport dzienny
    printf("\nRaport dzienny:\n");
    for (int i = 0; i < NUM_DOCTORS; i++) {
        Doctor* doctor = get_doctor_by_id(i);
        if (doctor) {
            print_doctor_report(*doctor);
        }
    }

    return 0;
}

// Funkcja wątku lekarza
void* doctor_thread(void* arg) {
    int doctor_id = (long)arg;

    while (1) {
        Patient p = process_next_patient();
        if (p.id == -1) break; // Brak pacjentów do obsłużenia

        Doctor* doctor = get_doctor_by_id(doctor_id);
        if (doctor) {
            handle_patient(doctor, p);
        }
    }
    return NULL;
}

// Funkcja wątku generatora pacjentów
void* patient_generator(void* arg) {
    (void)arg; // Parametr nieużywany

    for (int hour = CLINIC_OPEN_HOUR; hour <= CLINIC_CLOSE_HOUR; hour++) {
        printf("\nGodzina %d:00 - Pacjenci przychodzą.\n", hour);
        for (int i = 0; i < 10; i++) {
            sem_wait(&building_capacity); // Zmniejszenie liczby dostępnych miejsc

            Patient p = {
                .id = rand() % 70 + 1,    // ID pacjenta z zakresu 1–70
                .is_vip = rand() % 5 == 0, // Co 5. pacjent to VIP
                .target_doctor = rand() % NUM_DOCTORS,
                .age = rand() % 90,
                .has_guardian = (rand() % 2 == 0) // Losowy opiekun dla dzieci
            };
            add_patient_to_queue(p);
        }
    }

    printf("\nGodzina %d:00 - Przychodnia kończy przyjmowanie pacjentów.\n", CLINIC_CLOSE_HOUR);
    pthread_cond_broadcast(&queue_not_empty); // Informacja dla lekarzy, że kolejka może być pusta
    return NULL;
}
