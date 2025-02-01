#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "patient.h"
#include "config.h"

#define MSG_QUEUE_KEY 1234

typedef struct {
    long msg_type;
    Patient patient;
} PatientMessage;

int main() {
    printf("Generator pacjentów uruchomiony\n");

    // Pobranie kolejki komunikatów
    int msg_queue_id = msgget(MSG_QUEUE_KEY, 0666);
    if (msg_queue_id == -1) {
        perror("Błąd otwierania kolejki komunikatów");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 5; i++) { // Generujemy 5 pacjentów na test
        Patient p = {rand() % 70 + 1, rand() % 2, rand() % NUM_DOCTORS, rand() % 90};

        PatientMessage msg;
        msg.msg_type = 1;  // Typ wiadomości (musi być > 0)
        msg.patient = p;

        // Wysłanie pacjenta do rejestracji
        if (msgsnd(msg_queue_id, &msg, sizeof(Patient), 0) == -1) {
            perror("Błąd wysyłania pacjenta");
            continue;
        }

        printf("Pacjent ID %d wysłany do rejestracji\n", p.id);
        sleep(1);
    }

    return 0;
}
