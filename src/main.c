#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>   // dodany nagłówek
#include <fcntl.h>       // dla O_CREAT, O_EXCL

#define NUM_DOCTORS 6
#define NUM_PATIENTS 100
#define OPENING_TIME 8   // Godzina otwarcia
#define CLOSING_TIME 16  // Godzina zamknięcia

#define MAX_PATIENTS_INSIDE 10     // Maksymalna liczba pacjentów wewnątrz budynku
#define SEM_NAME "/clinic_sem"     // Nazwa semafora

int msg_queue;
int current_time = OPENING_TIME;
int clinic_open = 1;

pthread_mutex_t time_mutex = PTHREAD_MUTEX_INITIALIZER;

// Funkcja zegara - działa w tle jako wątek
void* time_simulation(void* arg) {
    while (current_time < CLOSING_TIME) {
        sleep(5);  // Każda sekunda to 1 godzina w symulacji
        pthread_mutex_lock(&time_mutex);
        current_time++;
        printf("Czas: %02d:00\n", current_time);
        pthread_mutex_unlock(&time_mutex);
    }
    clinic_open = 0;  // Oznacza zamknięcie przychodni
    printf("Przychodnia jest zamknięta, ale lekarze kończą pracę...\n");
    return NULL;
}

int main() {
    key_t key = ftok("clinic", 65);
    msg_queue = msgget(key, IPC_CREAT | 0666);

    if (msg_queue == -1) {
        perror("Błąd przy tworzeniu kolejki komunikatów");
        exit(1);
    }

    // Utworzenie semafora ograniczającego liczbę pacjentów wewnątrz budynku.
    sem_t *clinic_sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0666, MAX_PATIENTS_INSIDE);
    if (clinic_sem == SEM_FAILED) {
        perror("Błąd przy tworzeniu semafora");
        exit(1);
    }

    printf("Przychodnia otwarta od 08:00 do 16:00\n");

    // Tworzenie zegara w tle
    pthread_t time_thread;
    pthread_create(&time_thread, NULL, time_simulation, NULL);

    pid_t reg_pid, doctor_pids[NUM_DOCTORS], patient_pids[NUM_PATIENTS];

    reg_pid = fork();
    if (reg_pid == 0) {
        execl("./registration", "registration", NULL);
        perror("Błąd uruchamiania registration");
        exit(1);
    }

    for (int i = 0; i < NUM_DOCTORS; i++) {
        doctor_pids[i] = fork();
        if (doctor_pids[i] == 0) {
            execl("./doctor", "doctor", NULL);
            perror("Błąd uruchamiania doctor");
            exit(1);
        }
    }

    int patient_count = 0;
    while (current_time < CLOSING_TIME && patient_count < NUM_PATIENTS) {
        patient_pids[patient_count] = fork();
        if (patient_pids[patient_count] == 0) {
            execl("./patient", "patient", NULL);
            perror("Błąd uruchamiania patient");
            exit(1);
        }
        patient_count++;
        sleep(1);
    }

    // Czekamy na zamknięcie przychodni (czyli na zakończenie zegara)
    pthread_join(time_thread, NULL);

    // O 16:00 przychodnia zamyka drzwi, ale lekarze kończą pracę
    printf("Przychodnia zamyka drzwi, lekarze kończą przyjmowanie pacjentów...\n");

    kill(reg_pid, SIGTERM);
    for (int i = 0; i < NUM_DOCTORS; i++) {
        kill(doctor_pids[i], SIGTERM);
    }
    for (int i = 0; i < patient_count; i++) {
        kill(patient_pids[i], SIGTERM);
    }

    // Oczekiwanie na zakończenie procesów
    waitpid(reg_pid, NULL, 0);
    for (int i = 0; i < NUM_DOCTORS; i++) {
        waitpid(doctor_pids[i], NULL, 0);
    }
    for (int i = 0; i < patient_count; i++) {
        waitpid(patient_pids[i], NULL, 0);
    }

    // Zamknięcie i usunięcie semafora
    sem_close(clinic_sem);
    sem_unlink(SEM_NAME);

    msgctl(msg_queue, IPC_RMID, NULL);
    printf("Przychodnia zamknięta\n");
    return 0;
}
