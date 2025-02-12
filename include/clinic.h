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

extern int clinic_open;

#endif
#ifndef CLINIC_H
#define CLINIC_H

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <string.h>

// Konfiguracja symulacji
#define NUM_DOCTORS 6
#define NUM_PATIENTS 200
#define OPENING_TIME 8   // godzina otwarcia
#define CLOSING_TIME 16  // godzina zamknięcia
#define MAX_PATIENTS_INSIDE 30

#define SEM_NAME "/clinic_sem"

// Typy komunikatów – przyjmowane przez rejestrację i lekarzy
#define MSG_TYPE_REGISTRATION 1
#define MSG_TYPE_POZ 2
#define MSG_TYPE_KARDIOLOG 3
#define MSG_TYPE_OKULISTA 4
#define MSG_TYPE_PEDIATRA 5
#define MSG_TYPE_MEDYCYNY_PRACY 6

// Dla VIP – wyższe numery
#define MSG_TYPE_VIP_POZ 8
#define MSG_TYPE_VIP_KARDIOLOG 9
#define MSG_TYPE_VIP_OKULISTA 10
#define MSG_TYPE_VIP_PEDIATRA 11
#define MSG_TYPE_VIP_MEDYCYNY_PRACY 12

// Typ komunikatu dla skierowania
#define MSG_TYPE_REFERRAL 7

// Kody sygnałów (dla lekarzy)
#define SIGNAL_1 1
#define CLINIC_TERMINATED 3

// Struktura komunikatu – używana w kolejkach
struct message {
    long msg_type;      // Typ komunikatu – decyduje, do której kolejki trafi komunikat
    int patient_id;     // Identyfikator pacjenta
    char msg_text[100]; // Dodatkowy tekst
    int specialist_type; // Typ specjalisty (dla skierowań)
};

#endif // CLINIC_H
