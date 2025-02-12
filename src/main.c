#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include "clinic.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

/* Globalne zmienne */
int clinic_open = 1;
pid_t reg_pid;
pid_t doctor_pids[NUM_DOCTORS];
pid_t patient_pids[NUM_PATIENTS];
int patient_count = 0;
int current_time = OPENING_TIME;
pthread_mutex_t time_mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t *clinic_sem = NULL;

/* IPC kolejki */
int msg_queue, poz_queue, kardiolog_queue, okulista_queue, pediatra_queue, medycyny_pracy_queue;

/* Mutex do synchronizacji dostępu do tablicy pacjentów */
pthread_mutex_t patient_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Dla procesów lekarzy – globalna tablica i mutex */
int doctor_count = NUM_DOCTORS;
pthread_mutex_t doctor_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Wątek zbierający (reapujący) procesy pacjentów */
void *patient_waiter_thread(void *arg) {
    int status;
    while (1) {
        int all_reaped = 1;
        pthread_mutex_lock(&patient_mutex);
        for (int i = 0; i < patient_count; i++) {
            if (patient_pids[i] != -1) {  // Proces jeszcze nie został odebrany
                all_reaped = 0;
                pid_t pid = waitpid(patient_pids[i], &status, WNOHANG);
                if (pid > 0) {
                    printf("Patient process %d reaped\n", pid);
                    patient_pids[i] = -1;  // Oznaczamy, że został odebrany
                }
            }
        }
        pthread_mutex_unlock(&patient_mutex);
        if (!clinic_open && all_reaped) {
            break;
        }
        usleep(100000); 
    }
    return NULL;
}

/* Wątek zbierający (reapujący) procesy lekarzy */
void *doctor_waiter_thread(void *arg) {
    int status;
    while (1) {
        int all_reaped = 1;
        pthread_mutex_lock(&doctor_mutex);
        for (int i = 0; i < doctor_count; i++) {
            if (doctor_pids[i] != -1) {  // Proces jeszcze nie został odebrany
                all_reaped = 0;
                pid_t pid = waitpid(doctor_pids[i], &status, WNOHANG);
                if (pid > 0) {
                    printf("Doctor process %d reaped\n", pid);
                    doctor_pids[i] = -1;  // Oznaczamy, że został odebrany
                }
            }
        }
        pthread_mutex_unlock(&doctor_mutex);
        if (!clinic_open && all_reaped) {
            break;
        }
        usleep(100000);  // 100 ms
    }
    return NULL;
}

/* Wątek obsługi sygnału SIGUSR2 (ewakuacja) */
void *signal_handler_thread(void *arg) {
    sigset_t waitset;
    sigemptyset(&waitset);
    sigaddset(&waitset, SIGUSR2);
    int sig;
    if (sigwait(&waitset, &sig) == 0) {
        printf("Dyrektor: Ewakuacja – proszę o opuszczenie placówki.\n");
        fflush(stdout);
        kill(reg_pid, SIGTERM);
        for (int i = 0; i < NUM_DOCTORS; i++) {
            kill(doctor_pids[i], SIGTERM);
        }
        pthread_mutex_lock(&patient_mutex);
        for (int i = 0; i < patient_count; i++) {
            if (patient_pids[i] != -1) {
                kill(patient_pids[i], SIGTERM);
            }
        }
        pthread_mutex_unlock(&patient_mutex);
        sem_close(clinic_sem);
        sem_unlink(SEM_NAME);
        msgctl(msg_queue, IPC_RMID, NULL);
        msgctl(poz_queue, IPC_RMID, NULL);
        msgctl(kardiolog_queue, IPC_RMID, NULL);
        msgctl(okulista_queue, IPC_RMID, NULL);
        msgctl(pediatra_queue, IPC_RMID, NULL);
        msgctl(medycyny_pracy_queue, IPC_RMID, NULL);
        exit(0);
    }
    return NULL;
}

/* Wątek symulacji upływu czasu */
void *time_simulation(void *arg) {
    while (current_time < CLOSING_TIME) {
        sleep(4); // co 4 sekundy symulujemy wzrost godziny o 1
        pthread_mutex_lock(&time_mutex);
        current_time++;
        printf("Czas: %02d:00\n", current_time);
        pthread_mutex_unlock(&time_mutex);
    }
    clinic_open = 0;
    printf("Przychodnia jest zamknięta, lekarze kończą pracę...\n");
    return NULL;
}

int main(void) {
    /* Zapisanie PID głównego procesu */
    FILE *fp_main = fopen("main_pid.txt", "w");
    if (fp_main) {
        fprintf(fp_main, "%d\n", getpid());
        fclose(fp_main);
    } else {
        perror("Błąd przy zapisie main_pid.txt");
    }

    /* Blokowanie SIGUSR2 oraz SIGUSR1 we wszystkich wątkach */
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR2);
    sigaddset(&set, SIGUSR1);
    if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
        perror("Błąd przy blokowaniu sygnałów");
        exit(1);
    }

    /* Tworzenie kolejek komunikatów */
    key_t key = ftok("clinic", 65);
    msg_queue = msgget(key, IPC_CREAT | 0666);
    if (msg_queue == -1) { perror("Błąd przy tworzeniu msg_queue"); exit(1); }

    key_t key_poz = ftok("clinic_poz", 65);
    poz_queue = msgget(key_poz, IPC_CREAT | 0666);
    if (poz_queue == -1) { perror("Błąd przy tworzeniu poz_queue"); exit(1); }

    key_t key_kardiolog = ftok("clinic_kardiolog", 65);
    kardiolog_queue = msgget(key_kardiolog, IPC_CREAT | 0666);
    if (kardiolog_queue == -1) { perror("Błąd przy tworzeniu kardiolog_queue"); exit(1); }

    key_t key_okulista = ftok("clinic_okulista", 65);
    okulista_queue = msgget(key_okulista, IPC_CREAT | 0666);
    if (okulista_queue == -1) { perror("Błąd przy tworzeniu okulista_queue"); exit(1); }

    key_t key_pediatra = ftok("clinic_pediatra", 65);
    pediatra_queue = msgget(key_pediatra, IPC_CREAT | 0666);
    if (pediatra_queue == -1) { perror("Błąd przy tworzeniu pediatra_queue"); exit(1); }

    key_t key_medycyny_pracy = ftok("clinic_medycyny_pracy", 65);
    medycyny_pracy_queue = msgget(key_medycyny_pracy, IPC_CREAT | 0666);
    if (medycyny_pracy_queue == -1) { perror("Błąd przy tworzeniu medycyny_pracy_queue"); exit(1); }

    /* Utworzenie semafora – ogranicza liczbę pacjentów wewnątrz przychodni */
    sem_unlink(SEM_NAME);
    clinic_sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0666, MAX_PATIENTS_INSIDE);
    if (clinic_sem == SEM_FAILED) {
        perror("Błąd przy tworzeniu semafora");
        exit(1);
    }

    printf("Przychodnia otwarta od 08:00 do 16:00\n");

    pthread_t time_tid, sig_tid, patient_waiter_tid, doctor_waiter_tid;
    if (pthread_create(&time_tid, NULL, time_simulation, NULL) != 0) {
        perror("Błąd przy tworzeniu wątku symulacji czasu");
        exit(1);
    }
    if (pthread_create(&sig_tid, NULL, signal_handler_thread, NULL) != 0) {
        perror("Błąd przy tworzeniu wątku obsługi sygnałów");
        exit(1);
    }
    if (pthread_create(&patient_waiter_tid, NULL, patient_waiter_thread, NULL) != 0) {
        perror("Błąd przy tworzeniu wątku zbierającego pacjentów");
        exit(1);
    }
    if (pthread_create(&doctor_waiter_tid, NULL, doctor_waiter_thread, NULL) != 0) {
        perror("Błąd przy tworzeniu wątku zbierającego lekarzy");
        exit(1);
    }

    /* Tworzenie procesu rejestracji */
    reg_pid = fork();
    if (reg_pid == -1) {
        perror("Błąd przy fork rejestracji");
        exit(1);
    }
    if (reg_pid == 0) {
        execl("./registration", "registration", NULL);
        perror("Błąd uruchamiania registration");
        exit(1);
    }
    FILE *fp_reg = fopen("registration_pid.txt", "w");
    if (fp_reg) { fprintf(fp_reg, "%d\n", reg_pid); fclose(fp_reg); }
    else { perror("Błąd przy zapisie registration_pid.txt"); }

    /* Tworzenie procesów lekarzy */
    const char *doctor_roles[NUM_DOCTORS] = {"POZ", "POZ", "kardiolog", "okulista", "pediatra", "lekarz_medycyny_pracy"};
    const char *doctor_pid_files[NUM_DOCTORS] = {"poz1_pid.txt", "poz2_pid.txt", "kardiolog_pid.txt", "okulista_pid.txt", "pediatra_pid.txt", "lekarz_medycyny_pracy_pid.txt"};
    FILE *fp_doc = fopen("doctor_pids.txt", "w");
    if (!fp_doc) {
        perror("Błąd przy otwieraniu doctor_pids.txt");
        exit(1);
    }
    for (int i = 0; i < NUM_DOCTORS; i++) {
        doctor_pids[i] = fork();
        if (doctor_pids[i] == 0) {
            execl("./doctor", "doctor", doctor_roles[i], NULL);
            perror("Błąd uruchamiania lekarza");
            exit(1);
        }
        fprintf(fp_doc, "%d\n", doctor_pids[i]);
        FILE *fp_role = fopen(doctor_pid_files[i], "w");
        if (fp_role) { fprintf(fp_role, "%d\n", doctor_pids[i]); fclose(fp_role); }
        else { perror("Błąd przy zapisie pliku PID dla lekarza"); }
    }
    fclose(fp_doc);

    /* Tworzenie procesów pacjentów */
    FILE *fp_pat = fopen("patient_pids.txt", "w");
    if (!fp_pat) {
        perror("Błąd przy otwieraniu patient_pids.txt");
        exit(1);
    }
    while (current_time < CLOSING_TIME && patient_count < NUM_PATIENTS) {
        int sem_val;
        sem_getvalue(clinic_sem, &sem_val);
        if (sem_val > 0) {
            pid_t pid = fork();
            if (pid == 0) {
                execl("./patient", "patient", NULL);
                perror("Błąd uruchamiania pacjenta");
                exit(1);
            }
            pthread_mutex_lock(&patient_mutex);
            patient_pids[patient_count] = pid;
            patient_count++;
            pthread_mutex_unlock(&patient_mutex);
            fprintf(fp_pat, "%d\n", pid);
            sleep(1);
        } else {
            printf("Przychodnia pełna, pacjent czeka...\n");
            usleep(100000);
        }
    }
    fclose(fp_pat);

    /* Wysyłamy SIGTERM do procesów: rejestracji, lekarzy i pacjentów */
    kill(reg_pid, SIGTERM);
    for (int i = 0; i < NUM_DOCTORS; i++) {
        kill(doctor_pids[i], SIGTERM);
    }
    pthread_mutex_lock(&patient_mutex);
    for (int i = 0; i < patient_count; i++) {
        if (patient_pids[i] != -1) {
            kill(patient_pids[i], SIGTERM);
        }
    }
    pthread_mutex_unlock(&patient_mutex);

    /* Dajemy szansę na uporządkowane zakończenie */
    waitpid(reg_pid, NULL, 0);
    for (int i = 0; i < NUM_DOCTORS; i++) {
        waitpid(doctor_pids[i], NULL, 0);
    }

    /* Dołączamy wątki zbierające pacjentów i lekarzy */
    pthread_join(patient_waiter_tid, NULL);
    pthread_join(doctor_waiter_tid, NULL);

    /* Sprzątanie zasobów */
    sem_close(clinic_sem);
    sem_unlink(SEM_NAME);
    msgctl(msg_queue, IPC_RMID, NULL);
    msgctl(poz_queue, IPC_RMID, NULL);
    msgctl(kardiolog_queue, IPC_RMID, NULL);
    msgctl(okulista_queue, IPC_RMID, NULL);
    msgctl(pediatra_queue, IPC_RMID, NULL);
    msgctl(medycyny_pracy_queue, IPC_RMID, NULL);

    printf("Przychodnia zamknięta.\n");
    return 0;
}
