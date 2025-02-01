#ifndef PATIENT_H
#define PATIENT_H

#include "config.h" 

// Struktura pacjenta (przeniesiona z registration.h)
typedef struct {
    int id;
    bool is_vip;
    int target_doctor;
    int age;
} Patient;

// Funkcja generująca pacjentów (przeniesiona z main.c)
void* patient_generator(void* arg); 

#endif // PATIENT_H