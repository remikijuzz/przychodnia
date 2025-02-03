<<<<<<< HEAD
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include "patient.h"

#define MAX_PATIENTS_IN_BUILDING 50  

volatile bool running = true;
sem_t *building_capacity;
sem_t *registration_queue;

void handle_sigusr1(int sig) {
    (void)sig;
    printf("Pacjenci: Nieczynne.\n");
    running = false;
}
=======
#include "structs.h"
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define REGISTRATION_QUEUE_KEY 1234
>>>>>>> a8f733d (Rebuild projektu zgodnie z konsultacjami)

void handle_sigusr2(int sig) {
    (void)sig;
    printf("Pacjenci: Dyrektor zarządził ewakuację, opuszczamy przychodnię.\n");
    exit(0);
}

void *patient_thread(void *arg) {
    (void)arg;

    Patient p;
    p.id = rand() % 1000;
    p.target_doctor = rand() % 6;

    if (sem_trywait(building_capacity) == 0) {
        printf("Pacjent ID %d wszedł do budynku i idzie do rejestracji.\n", p.id);
        sem_post(registration_queue);
    } else {
        printf("Brak miejsca w przychodni, pacjent ID %d czeka przed wejściem.\n", p.id);
    }

    pthread_exit(NULL);
}

int main() {
<<<<<<< HEAD
    signal(SIGUSR1, handle_sigusr1);
    signal(SIGUSR2, handle_sigusr2);

    printf("Pacjenci przychodzą do placówki.\n");

    building_capacity = sem_open("/building_capacity", O_CREAT, 0666, MAX_PATIENTS_IN_BUILDING);
    registration_queue = sem_open("/registration_queue", O_CREAT, 0666, 0);

    while (running) {
        pthread_t patient_tid;
        pthread_create(&patient_tid, NULL, patient_thread, NULL);
        pthread_detach(patient_tid);
        sleep(1);
    }

    sem_close(building_capacity);
    sem_unlink("/building_capacity");
    sem_close(registration_queue);
    sem_unlink("/registration_queue");

=======
    Patient p = {rand() % 1000, rand() % 90, rand() % 5 == 0, rand() % 6, false, -1};

    int msgid = msgget(REGISTRATION_QUEUE_KEY, 0666);
    if (msgid == -1) {
        perror("Błąd uzyskania dostępu do kolejki rejestracji");
        exit(1);
    }

    Message msg = {1, p};
    if (msgsnd(msgid, &msg, sizeof(Patient), 0) == -1) {
        perror("Błąd wysyłania do rejestracji");
        exit(1);
    }

    printf("Pacjent %d zgłosił się do rejestracji.\n", p.id);
>>>>>>> a8f733d (Rebuild projektu zgodnie z konsultacjami)
    return 0;
}
