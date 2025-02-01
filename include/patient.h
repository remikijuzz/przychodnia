#ifndef PATIENT_H
#define PATIENT_H

#include <stdbool.h>

// Struktura pacjenta
typedef struct {
    int id;
    bool is_vip;
    int target_doctor;
    int age;
} Patient;

// Funkcja generująca pacjentów
void* patient_generator();

#endif // PATIENT_H
