#ifndef DOCTOR_H
#define DOCTOR_H

#include <stdbool.h>
#include "config.h"
#include "registration.h"
#include "patient.h"

// Struktura reprezentująca lekarza
typedef struct {
    int id;
    int max_patients;
    int patients_seen;
} Doctor;

// Inicjalizacja lekarzy
void init_doctors();

// Obsługa pacjenta przez lekarza
bool handle_patient(Doctor *doctor, Patient patient);

// Pobieranie lekarza po ID
Doctor* get_doctor_by_id(int id);


void* doctor_thread(void* arg);


#endif // DOCTOR_H
