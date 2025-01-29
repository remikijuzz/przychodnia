#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include "doctor.h"
#include "config.h"

const char* doctor_names[NUM_DOCTORS] = {
    "Lekarz POZ 1",
    "Lekarz POZ 2",
    "Kardiolog",
    "Okulista",
    "Pediatra",
    "Lekarz medycyny pracy"
};


static Doctor doctors[NUM_DOCTORS];

void init_doctors() {
    for (int i = 0; i < NUM_DOCTORS; i++) {
        doctors[i] = (Doctor){i, i < 2 ? DOCTOR_POZ_LIMIT : DOCTOR_SPECIALIST_LIMIT, 0};
    }
}

bool handle_patient(Doctor *doctor, Patient patient) {
    if (doctor->patients_seen >= doctor->max_patients) {
        printf("%s osiągnął limit pacjentów (%d).\n", doctor_names[doctor->id], patient.id);
        return false;
    }

    doctor->patients_seen++;
    printf("%s przyjmuje pacjenta ID %d.\n", doctor_names[doctor->id], patient.id);
    return true;
}

void* doctor_thread(void* arg) {
    int doctor_id = *(int*)arg;
    free(arg);

    while (clinic_open || !is_queue_empty()) {  
        pthread_mutex_lock(&queue_mutex);
        while (is_queue_empty() && clinic_open) {  
            pthread_cond_wait(&queue_not_empty, &queue_mutex);  
        }
        pthread_mutex_unlock(&queue_mutex);

        Patient p = process_next_patient();
        if (p.id != -1) {
            Doctor* doctor = get_doctor_by_id(doctor_id);
            if (doctor && doctor->patients_seen < doctor->max_patients) {
                handle_patient(doctor, p);
            } else {
                printf("Pacjent %d nie może zostać przyjęty przez %s(limit osiągnięty).\n", p.id, doctor_names[doctor->id]);
            }
        }
    }

    printf("%s zakończył pracę.\n", doctor_names[doctor_id]);
    return NULL;
}



Doctor* get_doctor_by_id(int id) {
    if (id < 0 || id >= NUM_DOCTORS) return NULL;
    return &doctors[id];
}
