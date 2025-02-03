#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdbool.h>

typedef struct {
    int id;
    int age;
    int is_vip;
    int target_doctor;
    bool requires_specialist;
    int specialist_id;
} Patient;

typedef struct {
    int doctor_id;
    int max_patients;
    int patients_seen;
} Doctor;

typedef struct {
    long type;
    Patient patient;
} Message;

#endif
