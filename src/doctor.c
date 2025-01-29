#include <stdio.h>
#include <pthread.h>
#include "doctor.h"

static Doctor doctors[NUM_DOCTORS];

void init_doctors() {
    for (int i = 0; i < NUM_DOCTORS; i++) {
        doctors[i] = (Doctor){i, i < 2 ? DOCTOR_POZ_LIMIT : DOCTOR_SPECIALIST_LIMIT, 0};
    }
}

bool handle_patient(Doctor *doctor, Patient patient) {
    if (doctor->patients_seen >= doctor->max_patients) {
        printf("Lekarz ID %d osiągnął limit pacjentów (%d).\n", doctor->id, doctor->max_patients);
        return false;
    }

    doctor->patients_seen++;
    printf("Lekarz ID %d przyjmuje pacjenta ID %d.\n", doctor->id, patient.id);
    return true;
}

Doctor* get_doctor_by_id(int id) {
    if (id < 0 || id >= NUM_DOCTORS) return NULL;
    return &doctors[id];
}
