#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h>
#include <semaphore.h>   // dodany nagłówek
#include <fcntl.h>       // dla O_CREAT, O_EXCL
#include <sys/types.h>

#define SEM_NAME "/clinic_sem"   // ta sama nazwa semafora, co w main.c

struct message {
    long msg_type;
    int patient_id;
    char msg_text[100];
};

int main() {
    // Otwieramy istniejący semafor
    sem_t *clinic_sem = sem_open(SEM_NAME, 0);
    if (clinic_sem == SEM_FAILED) {
        perror("Błąd otwarcia semafora w pacjencie");
        exit(1);
    }

    // Pacjent próbuje wejść do budynku (jeśli limit osiągnięty, proces będzie czekał)
    sem_wait(clinic_sem);
    printf("Pacjent %d: Wszedł do budynku przychodni.\n", getpid());

    key_t key = ftok("clinic", 65);
    int msg_queue = msgget(key, 0666);
    struct message msg;

    // Wysłanie komunikatu do rejestracji
    msg.msg_type = 1;
    msg.patient_id = getpid();
    snprintf(msg.msg_text, sizeof(msg.msg_text), "Pacjent %d wchodzi do rejestracji", msg.patient_id);
    msgsnd(msg_queue, &msg, sizeof(msg) - sizeof(long), 0);
    printf("Pacjent %d: Zarejestrowano.\n", msg.patient_id);
    
    sleep(1); // Dajemy czas na przetworzenie rejestracji

    // Wysłanie komunikatu do lekarza
    msg.msg_type = 2;
    msgsnd(msg_queue, &msg, sizeof(msg) - sizeof(long), 0);
    printf("Pacjent %d: Udał się do lekarza.\n", msg.patient_id);

    // Pacjent opuszcza budynek – zwalniamy miejsce
    sem_post(clinic_sem);
    printf("Pacjent %d: Opuszcza budynek przychodni.\n", msg.patient_id);

    sem_close(clinic_sem);
    return 0;
}
