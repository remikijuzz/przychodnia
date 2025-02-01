#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "patient.h"
#include "registration.h"  // Do funkcji add_patient_to_queue()
#include "config.h"

extern bool clinic_closing; // Zadeklarowana w main.c
extern pthread_mutex_t clinic_mutex; // Zadeklarowana w main.c

void* patient_generator(void* arg) {
    (void)arg;
    for (int hour = CLINIC_OPEN_HOUR; hour <= CLINIC_CLOSE_HOUR; hour++) {
        pthread_mutex_lock(&clinic_mutex);
        if (clinic_closing) {  // Jeśli rejestracja jest zamknięta, przerywamy generowanie pacjentów
            pthread_mutex_unlock(&clinic_mutex);
            printf("Rejestracja zamknięta - nowi pacjenci nie są przyjmowani.\n");
            break;
        }
        pthread_mutex_unlock(&clinic_mutex);

        printf("Godzina %d:00 - Pacjenci przychodzą.\n", hour);
        
        for (int i = 0; i < 10; i++) { // 10 pacjentów na godzinę
            pthread_mutex_lock(&clinic_mutex);
            if (clinic_closing) {  // Sprawdzamy ponownie wewnątrz pętli
                pthread_mutex_unlock(&clinic_mutex);
                printf("Rejestracja zamknięta - reszta pacjentów nie jest przyjmowana.\n");
                break;
            }
            pthread_mutex_unlock(&clinic_mutex);

            Patient p = { rand() % 70 + 1, rand() % 2, rand() % NUM_DOCTORS, rand() % 90 };
            add_patient_to_queue(p);
        }

        sleep(1);  // Symulacja godziny
    }
    return NULL;
}