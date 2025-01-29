#ifndef DOCTOR_H
#define DOCTOR_H
#define NUM_DOCTORS 6

#include "registration.h"

#define DOCTOR_POZ_LIMIT 30
#define DOCTOR_CARDIOLOGIST_LIMIT 10
#define DOCTOR_OPHTHALMOLOGIST_LIMIT 10
#define DOCTOR_PEDIATRICIAN_LIMIT 10
#define DOCTOR_WORK_MEDICINE_LIMIT 10

static const int DOCTOR_LIMITS[NUM_DOCTORS] = {
    DOCTOR_POZ_LIMIT,
    DOCTOR_POZ_LIMIT,
    DOCTOR_CARDIOLOGIST_LIMIT,
    DOCTOR_OPHTHALMOLOGIST_LIMIT,
    DOCTOR_PEDIATRICIAN_LIMIT,
    DOCTOR_WORK_MEDICINE_LIMIT
};


typedef struct {
    int id;
    int max_patients;
    int patients_seen;
} Doctor;

void init_doctors();
Doctor* get_doctor_by_id(int id);
bool handle_patient(Doctor *doctor, Patient patient);
void print_doctor_report(Doctor doctor);
bool add_to_specialist_queue(int doctor_id, Patient patient);
Patient process_specialist_queue(int doctor_id);

#endif
