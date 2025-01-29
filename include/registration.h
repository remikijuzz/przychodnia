#ifndef REGISTRATION_H
#define REGISTRATION_H

#include <stdbool.h>
#include <pthread.h>
#include "config.h"

// Struktura pacjenta
typedef struct {
    int id;
    bool is_vip;
    int target_doctor;
    int age;
} Patient;

// Kolejka pacjentów
extern pthread_mutex_t queue_mutex;
extern pthread_cond_t queue_not_empty;

// Inicjalizacja rejestracji
void init_registration();

// Dodanie pacjenta do kolejki
void add_patient_to_queue(Patient patient);

// Pobranie pacjenta z kolejki do obsługi
Patient process_next_patient();

// Zamknięcie rejestracji
void close_registration();

#endif // REGISTRATION_H
