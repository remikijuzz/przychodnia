#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include "registration.h"
#include "doctor.h"
#include "patient.h"  

#define MAX_QUEUE_SIZE 100
#define SECOND_WINDOW_THRESHOLD 25
#define CLOSE_SECOND_WINDOW_THRESHOLD 15

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_not_empty = PTHREAD_COND_INITIALIZER;

int msg_queue_id;
int patient_count = 0;
volatile bool running = true;

void handle_sigusr2(int sig) {
    (void)sig;
    printf("Rejestracja: Otrzymano SIGUSR2, kończę pracę.\n");
    running = false;
}

void* registration_window(void* arg) {
    int window_id = *(int*)arg;
    free(arg);

    while (running) {
        pthread_mutex_lock(&queue_mutex);
        while (patient_count == 0 && running) {
            pthread_cond_wait(&queue_not_empty, &queue_mutex);
        }

        if (!running) {
            pthread_mutex_unlock(&queue_mutex);
            break;
        }

        PatientMessage msg;
        msgrcv(msg_queue_id, &msg, sizeof(Patient), 1, 0);
        patient_count--;
        pthread_mutex_unlock(&queue_mutex);

        printf("Rejestracja okienko %d: Pacjent ID %d zapisany do lekarza %d.\n",
               window_id, msg.patient.id, msg.patient.target_doctor);
        sleep(2);
    }

    printf("Rejestracja okienko %d zamyka się.\n", window_id);
    return NULL;
}

int main() {
    signal(SIGUSR2, handle_sigusr2);

    msg_queue_id = msgget(MSG_QUEUE_KEY, IPC_CREAT | 0666);
    if (msg_queue_id == -1) {
        perror("Błąd tworzenia kolejki komunikatów");
        exit(EXIT_FAILURE);
    }

    pthread_t reg_window1, reg_window2;
    int* id1 = malloc(sizeof(int));
    *id1 = 1;
    pthread_create(&reg_window1, NULL, registration_window, id1);

    while (running) {
        sleep(5);
        pthread_mutex_lock(&queue_mutex);
        if (patient_count > SECOND_WINDOW_THRESHOLD) {
            printf("Otwieram drugie okienko rejestracji!\n");
            int* id2 = malloc(sizeof(int));
            *id2 = 2;
            pthread_create(&reg_window2, NULL, registration_window, id2);
        } else if (patient_count < CLOSE_SECOND_WINDOW_THRESHOLD) {
            printf("Zamykam drugie okienko rejestracji.\n");
            pthread_cancel(reg_window2);
        }
        pthread_mutex_unlock(&queue_mutex);
    }

    printf("Rejestracja zakończyła pracę.\n");
    return 0;
}
