#include "structs.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <signal.h>

#define DOCTOR_QUEUE_KEY 2000

volatile bool running = true;

void handle_sigusr1(int sig) {
    (void)sig;
    printf("Lekarz: Kończę przyjmowanie pacjentów.\n");
    running = false;
}

void handle_sigusr2(int sig) {
    (void)sig;
    printf("Lekarz: Natychmiastowa ewakuacja!\n");
    exit(0);
}

int main(int argc, char *argv[]) {
    signal(SIGUSR1, handle_sigusr1);
    signal(SIGUSR2, handle_sigusr2);

    int doctor_id = atoi(argv[1]);
    int msgid = msgget(DOCTOR_QUEUE_KEY + doctor_id, 0666 | IPC_CREAT);
    
    Message msg;
    while (running) {
        if (msgrcv(msgid, &msg, sizeof(Patient), 0, IPC_NOWAIT) > 0) {
            printf("Lekarz %d: Przyjmuję pacjenta %d\n", doctor_id, msg.patient.id);
            sleep(2);
        }
    }

    printf("Lekarz %d zakończył pracę.\n", doctor_id);
    return 0;
}
