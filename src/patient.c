#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>

#define SEM_NAME "/clinic_sem"  // ta sama nazwa semafora, co w main.c

struct message {
    long msg_type;
    int patient_id;
    char msg_text[100];
};

// Struktura przekazywana do wątku dziecka
typedef struct {
    sem_t *sem;
    int parent_pid;
} child_args_t;

// Funkcja reprezentująca dziecko – symuluje krótką aktywność, a potem zwalnia miejsce w semaforze.
void* child_thread_func(void* arg) {
    child_args_t *args = (child_args_t *) arg;
    // Informujemy, że dziecko (w ramach wątku) już „przyszło”
    printf("Dziecko: Dziecko (wątek) towarzyszy pacjentowi %d.\n", args->parent_pid);
    // Symulacja jakiejś aktywności dziecka
    sleep(1);
    // Zwolnienie miejsca – dziecko opuszcza przychodnię
    sem_post(args->sem);
    printf("Dziecko: Dziecko towarzyszące pacjentowi %d opuszcza przychodnię.\n", args->parent_pid);
    free(args);
    return NULL;
}

int main() {
    // Otwarcie istniejącego semafora
    sem_t *clinic_sem = sem_open(SEM_NAME, 0);
    if (clinic_sem == SEM_FAILED) {
        perror("Błąd otwarcia semafora w pacjencie");
        exit(1);
    }

    // Losowo decydujemy, czy pacjent przychodzi z dzieckiem (np. 50% szans)
    srand(time(NULL) ^ getpid());
    int accompanied = (rand() % 2); // 0 - bez dziecka, 1 - z dzieckiem

    if (accompanied) {
        // Jeśli pacjent przychodzi z dzieckiem – potrzebujemy dwóch miejsc w budynku.
        sem_wait(clinic_sem);  // dla osoby dorosłej
        sem_wait(clinic_sem);  // dla dziecka
        printf("Pacjent %d (dorosły) wraz z dzieckiem wszedł do przychodni.\n", getpid());
    } else {
        // Tylko dorosły pacjent
        sem_wait(clinic_sem);
        printf("Pacjent %d wszedł do przychodni.\n", getpid());
    }

    // Dalej standardowa symulacja rejestracji i skierowania do lekarza
    key_t key = ftok("clinic", 65);
    int msg_queue = msgget(key, 0666);
    struct message msg;

    // Wysyłamy komunikat do rejestracji
    msg.msg_type = 1;
    msg.patient_id = getpid();
    snprintf(msg.msg_text, sizeof(msg.msg_text), "Pacjent %d wchodzi do rejestracji", msg.patient_id);
    msgsnd(msg_queue, &msg, sizeof(msg) - sizeof(long), 0);
    printf("Pacjent %d: Zarejestrowano.\n", getpid());

    sleep(1); // czas na przetworzenie rejestracji

    // Wysyłamy komunikat do lekarza
    msg.msg_type = 2;
    msgsnd(msg_queue, &msg, sizeof(msg) - sizeof(long), 0);
    printf("Pacjent %d: Udał się do lekarza.\n", getpid());

    pthread_t child_thread;
    if (accompanied) {
        // Tworzymy wątek reprezentujący dziecko
        child_args_t *args = malloc(sizeof(child_args_t));
        if (args == NULL) {
            perror("Błąd alokacji pamięci dla argumentów wątku dziecka");
            // Jeśli nie uda się stworzyć wątku, należy zwolnić już zajęte miejsce przez dziecko
            sem_post(clinic_sem);
        } else {
            args->sem = clinic_sem;
            args->parent_pid = getpid();
            if (pthread_create(&child_thread, NULL, child_thread_func, args) != 0) {
                perror("Błąd przy tworzeniu wątku dziecka");
                // Zwolnienie miejsca, jeśli wątek nie zostanie utworzony
                sem_post(clinic_sem);
                free(args);
            }
        }
    }

    // Symulacja aktywności pacjenta (dorosłego)
    sleep(2);

    // Pacjent (dorosły) opuszcza przychodnię – zwalniamy miejsce
    sem_post(clinic_sem);
    printf("Pacjent %d (dorosły) opuszcza przychodnię.\n", getpid());

    if (accompanied) {
        // Czekamy na zakończenie wątku dziecka (który już wcześniej zwolnił swoje miejsce)
        pthread_join(child_thread, NULL);
    }

    sem_close(clinic_sem);
    return 0;
}
