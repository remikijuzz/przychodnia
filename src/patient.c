//patient.c

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>

#define SEM_NAME "/clinic_sem"

struct message {
    long msg_type;
    int patient_id;
    char msg_text[100];
};

volatile sig_atomic_t leave_flag = 0;
sem_t *clinic_sem = NULL;
volatile int accompanied = 0;

void sigusr2_handler(int signum) {
    leave_flag = 1;
    printf("Pacjent %d: Otrzymałem SIGUSR2 - natychmiast opuszczam przychodnię.\n", getpid());
    fflush(stdout);
    if (accompanied) {
        sem_post(clinic_sem);
        sem_post(clinic_sem);
    } else {
        sem_post(clinic_sem);
    }
    exit(0);
}

int main() {
    struct sigaction sa;
    sa.sa_handler = sigusr2_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("Błąd przy instalowaniu handlera SIGUSR2");
        exit(1);
    }
    
    clinic_sem = sem_open(SEM_NAME, 0);
    if (clinic_sem == SEM_FAILED) {
        perror("Błąd przy otwieraniu semafora w pacjencie");
        exit(1);
    }
    
    srand(time(NULL) ^ getpid());
    accompanied = (rand() % 2); // 50% szans na przyjście z dzieckiem
    
    if (accompanied) {
        sem_wait(clinic_sem);
        sem_wait(clinic_sem);
        printf("Pacjent %d (dorosły) wraz z dzieckiem wszedł do przychodni.\n", getpid());
    } else {
        sem_wait(clinic_sem);
        printf("Pacjent %d wszedł do przychodni.\n", getpid());
    }
    
    key_t key = ftok("clinic", 65);
    int msg_queue = msgget(key, 0666);
    struct message msg;
    msg.msg_type = 1;
    msg.patient_id = getpid();
    snprintf(msg.msg_text, sizeof(msg.msg_text), "Pacjent %d wchodzi do rejestracji", getpid());
    if (msgsnd(msg_queue, &msg, sizeof(msg) - sizeof(long), 0) < 0) {
        perror("Błąd przy wysyłaniu komunikatu rejestracyjnego");
    }
    printf("Pacjent %d: Zarejestrowano.\n", getpid());
    
    sleep(1);
    
    msg.msg_type = 2;
    if (msgsnd(msg_queue, &msg, sizeof(msg) - sizeof(long), 0) < 0) {
        perror("Błąd przy wysyłaniu komunikatu do lekarza");
    }
    printf("Pacjent %d: Udał się do lekarza.\n", getpid());
    
    sleep(2);
    
    if (!leave_flag) {
        sem_post(clinic_sem);
        printf("Pacjent %d (dorosły) opuszcza przychodnię.\n", getpid());
    }
    
    sem_close(clinic_sem);
    return 0;
}
