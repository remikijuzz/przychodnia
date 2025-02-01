#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <semaphore.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include "patient.h"
#include "config.h"

#define MSG_QUEUE_KEY 1234
#define MAX_PATIENTS_IN_BUILDING 50  // Limit pacjentów w budynku

volatile bool running = true;

void handle_sigusr2(int sig) {
    (void)sig;
    printf("Pacjenci: Otrzymano SIGUSR2 – wszyscy pacjenci natychmiast opuszczają przychodnię.\n");
    exit(0);
}


int main() {
    signal(SIGUSR2, handle_sigusr2);
    
    printf("Generator pacjentów uruchomiony, PID: %d\n", getpid());

    int msg_queue_id = msgget(MSG_QUEUE_KEY, 0666);
    if (msg_queue_id == -1) {
        perror("Błąd otwierania kolejki komunikatów");
        exit(EXIT_FAILURE);
    }

    sem_t *building_capacity = sem_open("/building_capacity", O_CREAT, 0666, MAX_PATIENTS_IN_BUILDING);
    if (building_capacity == SEM_FAILED) {
        perror("Błąd otwierania semafora budynku");
        exit(EXIT_FAILURE);
    }

    while (running) {
        Patient p;
        p.id = rand() % 1000;
        p.is_vip = rand() % 5 == 0;  // 20% pacjentów to VIP
        p.target_doctor = rand() % NUM_DOCTORS;
        p.age = rand() % 90;  // Wiek pacjenta

        printf("Nowy pacjent ID %d, lekarz %d, VIP: %d\n", p.id, p.target_doctor, p.is_vip);

        // Sprawdzamy, czy jest miejsce w budynku
        if (sem_trywait(building_capacity) == 0) {
            PatientMessage msg;
            msg.msg_type = 1;
            msg.patient = p;
            msgsnd(msg_queue_id, &msg, sizeof(Patient), 0);
        } else {
            printf("Brak miejsca w przychodni, pacjent ID %d czeka przed wejściem.\n", p.id);
        }

        sleep(1);  // Co sekundę generujemy nowego pacjenta
    }

    printf("🏃‍♂️ Generator pacjentów zakończył działanie.\n");
    return 0;
}
