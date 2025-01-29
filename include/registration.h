#ifndef REGISTRATION_H
#define REGISTRATION_H

#include <stdbool.h>

#define MAX_PATIENTS 50 // Maksymalna liczba pacjentów w budynku

extern pthread_cond_t queue_not_empty;


typedef struct {
    int id;               // ID pacjenta
    bool is_vip;          // Czy pacjent jest VIP?
    int target_doctor;    // ID docelowego lekarza
    int age;              // Wiek pacjenta
    bool has_guardian;    // Czy pacjent ma opiekuna (dla dzieci poniżej 18 lat)?
} Patient;

void init_registration();
void add_patient_to_queue(Patient patient);
Patient process_next_patient();
void close_registration();

#endif
