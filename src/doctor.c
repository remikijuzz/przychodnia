#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "doctor.h"
#include "config.h"

// Tablica lekarzy
static Doctor doctors[NUM_DOCTORS];
static pthread_mutex_t doctor_mutex[NUM_DOCTORS]; // Mutexy dla lekarzy

// Inicjalizacja lekarzy
void init_doctors() {
    static const int DOCTOR_LIMITS[NUM_DOCTORS] = {
        DOCTOR_POZ_LIMIT,
        DOCTOR_POZ_LIMIT,
        DOCTOR_CARDIOLOGIST_LIMIT,
        DOCTOR_OPHTHALMOLOGIST_LIMIT,
        DOCTOR_PEDIATRICIAN_LIMIT,
        DOCTOR_WORK_MEDICINE_LIMIT
    };

    for (int i = 0; i < NUM_DOCTORS; i++) {
        doctors[i] = (Doctor){i, DOCTOR_LIMITS[i], 0};
        pthread_mutex_init(&doctor_mutex[i], NULL);
    }
    printf("Lekarze zainicjalizowani.\n");
}

// Funkcja zwracająca wskaźnik do lekarza o podanym ID
Doctor* get_doctor_by_id(int id) {
    if (id < 0 || id >= NUM_DOCTORS) {
        printf("Błąd: Nieprawidłowy ID lekarza (%d).\n", id);
        return NULL;
    }
    return &doctors[id];
}

// Obsługa pacjenta przez lekarza
bool handle_patient(Doctor *doctor, Patient patient) {
    if (doctor->patients_seen >= doctor->max_patients) {
        printf("Lekarz ID %d osiągnął limit pacjentów (%d).\n", doctor->id, doctor->max_patients);
        return false; // Zwróć false, żeby pacjent został przekierowany lub odrzucony
    }

    doctor->patients_seen++;
    printf("Lekarz ID %d przyjmuje pacjenta ID %d.\n", doctor->id, patient.id);
    return true;
}


// Drukowanie raportu lekarza
void print_doctor_report(Doctor doctor) {
    printf("Lekarz ID %d:\n", doctor.id);
    printf("- Przyjętych pacjentów: %d/%d\n", doctor.patients_seen, doctor.max_patients);
}
