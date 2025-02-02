#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <stdbool.h>
#include <fcntl.h>
#include <semaphore.h>
#include "patient.h"
#include "config.h"

#define MAX_PATIENTS_IN_BUILDING 50
#define MSG_QUEUE_KEY 1234

volatile bool running = true;
int liczba_pacjentów_w_budynku = 0;

void save_to_report(int patient_id, const char *reason) {
    FILE *file = fopen("report.txt", "a");
    if (file == NULL) {
        perror("Błąd otwierania pliku raportu");
        return;
    }
    fprintf(file, "Pacjent ID %d nie został przyjęty: %s\n", patient_id, reason);
    fclose(file);
}

void handle_sigusr2(int sig) {
    (void)sig;
    printf("Rejestracja: Dyrektor zarządził ewakuację, opuszczamy budynek.\n");
    running = false;
}

int main() {
    signal(SIGUSR2, handle_sigusr2);
    printf("Rejestracja uruchomiona.\n");

    int msg_queue_id = msgget(MSG_QUEUE_KEY, IPC_CREAT | 0666);
    if (msg_queue_id == -1) {
        perror("Błąd otwierania kolejki komunikatów");
        exit(EXIT_FAILURE);
    }

    while (running) {
        PatientMessage msg;
        if (msgrcv(msg_queue_id, &msg, sizeof(Patient), 1, IPC_NOWAIT) != -1) {
            if (liczba_pacjentów_w_budynku >= MAX_PATIENTS_IN_BUILDING) {
                save_to_report(msg.patient.id, "Przychodnia pełna – pacjent czekał przed wejściem.");
            } else {
                liczba_pacjentów_w_budynku++;
                printf("Pacjent ID %d zapisany do rejestracji.\n", msg.patient.id);
            }
        }
        sleep(1);
    }

    printf("Rejestracja zakończyła pracę.\n");
    return 0;
}
