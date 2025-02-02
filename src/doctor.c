#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>
#include "patient.h"
#include "registration.h"

#define MAX_PATIENTS 10

volatile bool running = true;
int patients_seen = 0;
static int doctor_id;

const char *doctor_specializations[] = {
    "Lekarz POZ 1",
    "Lekarz POZ 2",
    "Lekarz Kardiolog",
    "Lekarz Okulista",
    "Lekarz Pediatra",
    "Lekarz Medycyny Pracy"
};

void save_to_report(int patient_id, const char *doctor_name) {
    FILE *file = fopen("report.txt", "a");
    if (file == NULL) {
        perror("Błąd otwierania pliku raportu");
        return;
    }
    fprintf(file, "Pacjent ID %d nie został przyjęty przez %s – brak miejsc.\n", patient_id, doctor_name);
    fclose(file);
}

void handle_sigusr1(int sig) {
    (void)sig;
    printf("%s: Kończę przyjmowanie pacjentów i zapisuję raport.\n", doctor_specializations[doctor_id]);
    save_to_report(-1, doctor_specializations[doctor_id]);  
    exit(0);
}

void handle_sigusr2(int sig) {
    (void)sig;
    printf("%s: Ewakuacja, opuszczamy budynek.\n", doctor_specializations[doctor_id]);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Błąd: Brak ID lekarza\n");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));

    doctor_id = atoi(argv[1]);
    signal(SIGUSR1, handle_sigusr1);
    signal(SIGUSR2, handle_sigusr2);

    printf("%s rozpoczął pracę, PID: %d\n", doctor_specializations[doctor_id], getpid());

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
                printf("%s: Przyjmuję pacjenta ID %d (%d/%d), VIP: %d\n",
                       doctor_specializations[doctor_id], msg.patient.id, patients_seen + 1, MAX_PATIENTS, msg.patient.is_vip);
                patients_seen++;

                if (doctor_id == 0 || doctor_id == 1) {  // Lekarze POZ
                    if (rand() % 100 < 20) {  // 20% pacjentów dostaje skierowanie
                        int specialist_id = 2 + (rand() % 4);
                        printf("%s skierował pacjenta ID %d do %s\n", 
                               doctor_specializations[doctor_id], msg.patient.id, doctor_specializations[specialist_id]);
                        
                        if (patients_seen >= MAX_PATIENTS) {
                            save_to_report(msg.patient.id, doctor_specializations[specialist_id]);
                        }
                    }
                }
            } else {
                printf("%s: Brak pacjentów w kolejce.\n", doctor_specializations[doctor_id]);
            }
        } else {
            printf("%s: Osiągnięto limit pacjentów.\n", doctor_specializations[doctor_id]);
            running = false;
        }
    }

    printf("%s zakończył pracę i opuszcza przychodnię.\n", doctor_specializations[doctor_id]);
    return 0;
}
