#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "patient.h"
#include "registration.h"

#define MSG_QUEUE_KEY 1234
#define MAX_PATIENTS 10

volatile bool running = true;
int patients_seen = 0;

void handle_sigusr1(int sig) {
    printf("Lekarz: Otrzymano SIGUSR1, kończę przyjmowanie pacjentów.\n");
    running = false;
}

void handle_sigusr2(int sig) {
    printf("Lekarz: Otrzymano SIGUSR2, natychmiastowe zamknięcie!\n");
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Błąd: Brak ID lekarza\n");
        exit(EXIT_FAILURE);
    }

    int doctor_id = atoi(argv[1]);
    signal(SIGUSR1, handle_sigusr1);
    signal(SIGUSR2, handle_sigusr2);

    printf("Lekarz ID %d uruchomiony, PID: %d\n", doctor_id, getpid());

    int msg_queue_id = msgget(MSG_QUEUE_KEY, 0666);
    if (msg_queue_id == -1) {
        perror("Błąd otwierania kolejki komunikatów");
        exit(EXIT_FAILURE);
    }

    while (running) {
        sleep(2);

        if (patients_seen < MAX_PATIENTS) {
            PatientMessage msg;
            if (msgrcv(msg_queue_id, &msg, sizeof(Patient), 1, IPC_NOWAIT) != -1) {
                printf("Lekarz %d: Przyjmuję pacjenta ID %d (%d/%d), VIP: %d\n",
                       doctor_id, msg.patient.id, patients_seen + 1, MAX_PATIENTS, msg.patient.is_vip);
                patients_seen++;
            } else {
                printf("Lekarz %d: Brak pacjentów w kolejce.\n", doctor_id);
            }
        } else {
            printf("Lekarz %d: Osiągnięto limit pacjentów.\n", doctor_id);
            running = false;
        }
    }

    printf("Lekarz %d zakończył pracę.\n", doctor_id);
    return 0;
}
