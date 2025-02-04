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
#include <string.h>

#define NUM_DOCTORS 6
#define NUM_PATIENTS 100
#define OPENING_TIME 8   // Godzina otwarcia
#define CLOSING_TIME 16  // Godzina zamknięcia

#define MAX_PATIENTS_INSIDE 10     // Maksymalna liczba pacjentów jednocześnie wewnątrz budynku
#define SEM_NAME "/clinic_sem"     // Nazwa semafora

// Globalne zmienne symulacyjne
int msg_queue;
int current_time = OPENING_TIME;
int clinic_open = 1;
pthread_mutex_t time_mutex = PTHREAD_MUTEX_INITIALIZER;

// Funkcja zegara – symulacja upływu czasu
void* time_simulation(void* arg) {
    while (current_time < CLOSING_TIME) {
        sleep(5);  // Co 5 sekund symulujemy 1 godzinę
        pthread_mutex_lock(&time_mutex);
        current_time++;
        printf("Czas: %02d:00\n", current_time);
        pthread_mutex_unlock(&time_mutex);
    }
    clinic_open = 0;
    printf("Przychodnia jest zamknięta, ale lekarze kończą pracę...\n");
    return NULL;
}

int main(int argc, char *argv[]) {
    // Parametr testowy – domyślnie brak
    // test_signal = 0 -> brak, 1 -> wyślij SIGUSR1, 2 -> wyślij SIGUSR2
    int test_signal = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s1") == 0)
            test_signal = 1;
        else if (strcmp(argv[i], "-s2") == 0)
            test_signal = 2;
    }

    key_t key = ftok("clinic", 65);
    msg_queue = msgget(key, IPC_CREAT | 0666);
    if (msg_queue == -1) {
        perror("Błąd przy tworzeniu kolejki komunikatów");
        exit(1);
    }

    // Usunięcie semafora, jeśli już istnieje
    sem_unlink(SEM_NAME);
    sem_t *clinic_sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0666, MAX_PATIENTS_INSIDE);
    if (clinic_sem == SEM_FAILED) {
        perror("Błąd przy tworzeniu semafora");
        exit(1);
    }

    printf("Przychodnia otwarta od 08:00 do 16:00\n");

    // Uruchamiamy zegar symulacyjny
    pthread_t time_thread;
    pthread_create(&time_thread, NULL, time_simulation, NULL);

    pid_t reg_pid, doctor_pids[NUM_DOCTORS], patient_pids[NUM_PATIENTS];

    // Uruchamiamy proces rejestracji
    reg_pid = fork();
    if (reg_pid == 0) {
        execl("./registration", "registration", NULL);
        perror("Błąd uruchamiania registration");
        exit(1);
    }

    // Uruchamiamy procesy lekarzy – tutaj przekazujemy argument określający rolę
    for (int i = 0; i < NUM_DOCTORS; i++) {
        doctor_pids[i] = fork();
        if (doctor_pids[i] == 0) {
            if (i < 2)
                execl("./doctor", "doctor", "POZ", NULL);
            else if (i == 2)
                execl("./doctor", "doctor", "kardiolog", NULL);
            else if (i == 3)
                execl("./doctor", "doctor", "okulista", NULL);
            else if (i == 4)
                execl("./doctor", "doctor", "pediatra", NULL);
            else if (i == 5)
                execl("./doctor", "doctor", "lekarz_medycyny_pracy", NULL);
            perror("Błąd uruchamiania doctor");
            exit(1);
        }
    }

    int patient_count = 0;
    while (clinic_open && patient_count < NUM_PATIENTS) {
        patient_pids[patient_count] = fork();
        if (patient_pids[patient_count] == 0) {
            execl("./patient", "patient", NULL);
            perror("Błąd uruchamiania patient");
            exit(1);
        }
        patient_count++;
        sleep(1);
    }

    // Jeśli mamy aktywowany tryb testowy sygnału, czekamy chwilę i wysyłamy sygnał
    if (test_signal == 1) {
        // Przykładowo: czekamy 15 sekund, a następnie wysyłamy SIGUSR1
        sleep(15);
        printf("Dyrektor: Wysyłam SIGUSR1 (bada bieżącego pacjenta i kończy przyjmowanie) do procesów lekarzy i rejestracji.\n");
        kill(reg_pid, SIGUSR1);
        for (int i = 0; i < NUM_DOCTORS; i++) {
            kill(doctor_pids[i], SIGUSR1);
        }
    } else if (test_signal == 2) {
        // Przykładowo: czekamy 15 sekund, a następnie wysyłamy SIGUSR2
        sleep(15);
        printf("Dyrektor: Wysyłam SIGUSR2 (natychmiastowa ewakuacja) do wszystkich procesów.\n");
        kill(reg_pid, SIGUSR2);
        for (int i = 0; i < NUM_DOCTORS; i++) {
            kill(doctor_pids[i], SIGUSR2);
        }
        for (int i = 0; i < patient_count; i++) {
            kill(patient_pids[i], SIGUSR2);
        }
    }

    // Czekamy na zakończenie zegara
    pthread_join(time_thread, NULL);

    printf("Przychodnia zamyka drzwi, lekarze kończą przyjmowanie pacjentów...\n");

    kill(reg_pid, SIGTERM);
    for (int i = 0; i < NUM_DOCTORS; i++) {
        kill(doctor_pids[i], SIGTERM);
    }
    for (int i = 0; i < patient_count; i++) {
        kill(patient_pids[i], SIGTERM);
    }

    waitpid(reg_pid, NULL, 0);
    for (int i = 0; i < NUM_DOCTORS; i++) {
        waitpid(doctor_pids[i], NULL, 0);
    }
    for (int i = 0; i < patient_count; i++) {
        waitpid(patient_pids[i], NULL, 0);
    }

    sem_close(clinic_sem);
    sem_unlink(SEM_NAME);
    msgctl(msg_queue, IPC_RMID, NULL);
    printf("Przychodnia zamknięta\n");
    return 0;
}
