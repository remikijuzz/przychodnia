// patient.c
#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>

#define SEM_NAME "/clinic_sem"  // ta sama nazwa semafora

struct message {
    long msg_type;
    int patient_id;
    char msg_text[100];
};

// Obsługa sygnału SIGUSR2 – ewakuacja
void handle_patient_sigusr2(int sig) {
    printf("Pacjent %d: Otrzymałem SIGUSR2 - natychmiast opuszczam przychodnię.\n", getpid());
    exit(0);
}

// Opcjonalnie można dodać obsługę SIGUSR1, jeśli wymagane
// void handle_patient_sigusr1(int sig) { /* np. przejście do trybu zamknięcia */ }

typedef struct {
    sem_t *sem;
    int parent_pid;
} child_args_t;

void* child_thread_func(void* arg) {
    child_args_t *args = (child_args_t *) arg;
    printf("Dziecko: Dziecko (wątek) towarzyszy pacjentowi %d.\n", args->parent_pid);
    sleep(1);
    sem_post(args->sem);
    printf("Dziecko: Dziecko towarzyszące pacjentowi %d opuszcza przychodnię.\n", args->parent_pid);
    free(args);
    return NULL;
}

int main() {
    signal(SIGUSR2, handle_patient_sigusr2);
    // Jeśli chcesz, możesz także ustawić obsługę SIGUSR1 dla pacjenta

    sem_t *clinic_sem = sem_open(SEM_NAME, 0);
    if (clinic_sem == SEM_FAILED) {
        perror("Błąd otwarcia semafora w pacjencie");
        exit(1);
    }

    srand(time(NULL) ^ getpid());
    int accompanied = (rand() % 2); // 0 – bez dziecka, 1 – z dzieckiem

    if (accompanied) {
        sem_wait(clinic_sem);  // miejsce dla osoby dorosłej
        sem_wait(clinic_sem);  // miejsce dla dziecka
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
    msgsnd(msg_queue, &msg, sizeof(msg) - sizeof(long), 0);
    printf("Pacjent %d: Zarejestrowano.\n", getpid());

    sleep(1); // czas na przetworzenie rejestracji

    msg.msg_type = 2;
    msgsnd(msg_queue, &msg, sizeof(msg) - sizeof(long), 0);
    printf("Pacjent %d: Udał się do lekarza.\n", getpid());

    pthread_t child_thread;
    if (accompanied) {
        child_args_t *args = malloc(sizeof(child_args_t));
        if (!args) {
            perror("Błąd alokacji pamięci dla wątku dziecka");
            sem_post(clinic_sem);
        } else {
            args->sem = clinic_sem;
            args->parent_pid = getpid();
            if (pthread_create(&child_thread, NULL, child_thread_func, args) != 0) {
                perror("Błąd przy tworzeniu wątku dziecka");
                sem_post(clinic_sem);
                free(args);
            }
        }
    }

    sleep(2);

    sem_post(clinic_sem);
    printf("Pacjent %d (dorosły) opuszcza przychodnię.\n", getpid());

    if (accompanied) {
        pthread_join(child_thread, NULL);
    }
    sem_close(clinic_sem);
    return 0;
}
