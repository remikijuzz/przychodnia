#include <stdbool.h>

#ifndef CONFIG_H
#define CONFIG_H

#define CLINIC_OPEN_HOUR 8
#define CLINIC_CLOSE_HOUR 16

#define MAX_PATIENTS_IN_BUILDING 50
#define QUEUE_SIZE 100

#define NUM_DOCTORS 6
#define DOCTOR_POZ_LIMIT 30
#define DOCTOR_SPECIALIST_LIMIT 10

// Klucz do kolejki komunikatów dla rejestracji i lekarzy
#define MSG_QUEUE_KEY 1234
#define PATIENT_QUEUE_KEY 5678

extern _Bool clinic_open;

#endif
