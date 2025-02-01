#ifndef REGISTRATION_H
#define REGISTRATION_H

#include <stdbool.h>
#include <pthread.h>
#include "config.h"
#include "patient.h"


// Kolejka pacjentów
extern pthread_mutex_t queue_mutex;
extern pthread_cond_t queue_not_empty;


// Inicjalizacja rejestracji
void init_registration();

// Dodanie pacjenta do kolejki
void add_patient_to_queue(Patient patient);

// Pobranie pacjenta z kolejki do obsługi
Patient process_next_patient();

int is_queue_empty(); 

// Zamknięcie rejestracji
void close_registration();

#endif // REGISTRATION_H