#ifndef PATIENT_H
#define PATIENT_H


// Struktura pacjenta
typedef struct {
    int id;
    bool is_vip;
    int target_doctor;
    int age;
} Patient;

// Struktura wiadomości dla kolejki komunikatów
typedef struct {
    long msg_type;
    Patient patient;
} PatientMessage;

#endif // PATIENT_H
