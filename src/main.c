#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>

// Definicje parametrów symulacji
#define NUM_DOCTORS 6
#define NUM_PATIENTS 200
#define OPENING_TIME 8   // Godzina otwarcia
#define CLOSING_TIME 16  // Godzina zamknięcia
#define MAX_PATIENTS_INSIDE 30
#define SEM_NAME "/clinic_sem"

// Definicje limitów przyjęć lekarzy
#define POZ_CAPACITY 30
#define KARDIOLOG_CAPACITY 10
#define OKULISTA_CAPACITY 10
#define PEDIATRA_CAPACITY 10
#define LEKARZ_MEDYCYNY_PRACY_CAPACITY 10

// **NOWE: Definicje nazw semaforów dla specjalistów**
#define KARDIOLOG_SEM_NAME "/kardiolog_sem"
#define OKULISTA_SEM_NAME "/okulista_sem"
#define PEDIATRA_SEM_NAME "/pediatra_sem"
#define LEKARZ_MEDYCYNY_PRACY_SEM_NAME "/lekarz_pracy_sem"

// Funkcja pomocnicza do generowania nazw semaforów na podstawie typu lekarza
const char* get_semaphore_name(int doctor_type) {
    switch (doctor_type) {
        case 3: return KARDIOLOG_SEM_NAME;
        case 4: return OKULISTA_SEM_NAME;
        case 5: return PEDIATRA_SEM_NAME;
        case 6: return LEKARZ_MEDYCYNY_PRACY_SEM_NAME;
        default: return "unknown_sem";
    }
}

// Globalne zmienne
int current_time = OPENING_TIME;
int clinic_open = 1;
pthread_mutex_t time_mutex = PTHREAD_MUTEX_INITIALIZER;

pid_t reg_pid;
pid_t doctor_pids[NUM_DOCTORS];
pid_t patient_pids[NUM_PATIENTS];
int patient_count = 0;
sem_t *clinic_sem = NULL;
int msg_queues[7]; // Indeksy 1-6 odpowiadają typom lekarzy, 0 nieużywany
sem_t *specialist_semaphores[7]; // Indeksy 3-6 odpowiadają specjalistom, 0-2 nieużywane


// Wątek obsługujący sygnał SIGUSR2
void *signal_handler_thread(void *arg) {
    sigset_t waitset;
    sigemptyset(&waitset);
    sigaddset(&waitset, SIGUSR2);
    int sig;
    if (sigwait(&waitset, &sig) == 0) {
        printf("Dyrektor: Zarządzono ewakuację, prosimy o opuszczenie placówki.\n");
        fflush(stdout);
        kill(reg_pid, SIGTERM);
        for (int i = 0; i < NUM_DOCTORS; i++) {
            kill(doctor_pids[i], SIGTERM);
        }
        for (int i = 0; i < patient_count; i++) {
            kill(patient_pids[i], SIGTERM);
        }
        sem_close(clinic_sem);
        sem_unlink(SEM_NAME);
        for (int i = 1; i <= 6; i++) {
            msgctl(msg_queues[i], IPC_RMID, NULL);
        }
        for (int i = 3; i <= 6; i++) {
            sem_close(specialist_semaphores[i]);
            sem_unlink(get_semaphore_name(i));
        }
        exit(0);
    }
    return NULL;
}

// Wątek symulujący upływ czasu
void* time_simulation(void* arg) {
    while (current_time < CLOSING_TIME) {
        sleep(4);
        pthread_mutex_lock(&time_mutex);
        current_time++;
        printf("Czas: %02d:00\n", current_time);
        pthread_mutex_unlock(&time_mutex);
    }
    clinic_open = 0;
    printf("Przychodnia jest zamknięta, ale lekarze kończą pracę...\n");
    return NULL;
}

int main() {
    // Zapis PID głównego procesu
    FILE *fp_main = fopen("main_pid.txt", "w");
    if (fp_main) {
        fprintf(fp_main, "%d\n", getpid());
        fclose(fp_main);
    } else {
        perror("Błąd przy zapisie main_pid.txt");
    }

    // Blokowanie SIGUSR2
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR2);
    if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
        perror("Błąd przy blokowaniu SIGUSR2");
        exit(1);
    }

    // Tworzenie kolejek komunikatów
    key_t key;
    for (int i = 1; i <= 6; i++) {
        key = ftok("clinic", 64 + i);
        msg_queues[i] = msgget(key, IPC_CREAT | 0666);
        if (msg_queues[i] == -1) {
            perror("Błąd przy tworzeniu kolejki komunikatów");
            exit(1);
        }
        printf("Utworzono kolejkę komunikatów dla typu lekarza %d, ID: %d\n", i, msg_queues[i]);
    }
    key = ftok("clinic", 65);
    msg_queues[1] = msgget(key, 0666);

    // Tworzenie semafora ogólnego przychodni
    sem_unlink(SEM_NAME);
    clinic_sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0666, MAX_PATIENTS_INSIDE);
    if (clinic_sem == SEM_FAILED) {
        perror("Błąd przy tworzeniu semafora");
        exit(1);
    }

    // Tworzenie semaforów dla specjalistów
    sem_unlink(KARDIOLOG_SEM_NAME);
    specialist_semaphores[3] = sem_open(KARDIOLOG_SEM_NAME, O_CREAT | O_EXCL, 0666, KARDIOLOG_CAPACITY);
    sem_unlink(OKULISTA_SEM_NAME);
    specialist_semaphores[4] = sem_open(OKULISTA_SEM_NAME, O_CREAT | O_EXCL, 0666, OKULISTA_CAPACITY);
    sem_unlink(PEDIATRA_SEM_NAME);
    specialist_semaphores[5] = sem_open(PEDIATRA_SEM_NAME, O_CREAT | O_EXCL, 0666, PEDIATRA_CAPACITY);
    sem_unlink(LEKARZ_MEDYCYNY_PRACY_SEM_NAME);
    specialist_semaphores[6] = sem_open(LEKARZ_MEDYCYNY_PRACY_SEM_NAME, O_CREAT | O_EXCL, 0666, LEKARZ_MEDYCYNY_PRACY_CAPACITY);
    for (int i = 3; i <= 6; ++i) {
        if (specialist_semaphores[i] == SEM_FAILED) {
            perror("Błąd przy tworzeniu semafora specjalisty");
            exit(1);
        }
        printf("Utworzono semafor dla specjalisty typu %d, nazwa: %s\n", i, get_semaphore_name(i));
    }

    printf("Przychodnia otwarta od 08:00 do 16:00\n");

    pthread_t time_thread, sig_thread;
    pthread_create(&time_thread, NULL, time_simulation, NULL);
    pthread_create(&sig_thread, NULL, signal_handler_thread, NULL);

    // Tworzenie procesu rejestracji
    reg_pid = fork();
    if (reg_pid == 0) {
        execl("./registration", "registration", NULL);
        perror("Błąd uruchamiania rejestracji");
        exit(1);
    }
    FILE *fp_reg = fopen("registration_pid.txt", "w");
    if (fp_reg) {
        fprintf(fp_reg, "%d\n", reg_pid);
        fclose(fp_reg);
    } else {
        perror("Błąd przy zapisie registration_pid.txt");
    }

    // Tworzenie procesów lekarzy
    const char *doctor_roles[NUM_DOCTORS] = {"POZ", "POZ", "kardiolog", "okulista", "pediatra", "lekarz_medycyny_pracy"};
    int doctor_capacities[NUM_DOCTORS] = {POZ_CAPACITY, POZ_CAPACITY, KARDIOLOG_CAPACITY, OKULISTA_CAPACITY, PEDIATRA_CAPACITY, LEKARZ_MEDYCYNY_PRACY_CAPACITY};
    FILE *fp_doc = fopen("doctor_pids.txt", "w");
    if (!fp_doc) {
        perror("Błąd przy otwieraniu doctor_pids.txt do zapisu");
        exit(1);
    }

    // **NOWE: Konwersja ID kolejek specjalistów na stringi**
    char msg_queues_str[7][10]; // Tablica stringów na ID kolejek, indeksy 1-6, 0 nieużywane
    for (int i = 1; i <= 6; i++) {
        sprintf(msg_queues_str[i], "%d", msg_queues[i]);
    }


    for (int i = 0; i < NUM_DOCTORS; i++) {
        doctor_pids[i] = fork();
        if (doctor_pids[i] == 0) {
            char capacity_str[10];
            sprintf(capacity_str, "%d", doctor_capacities[i]);
            char msg_queue_id_str[10];
            int msg_queue_type;
            if (strcmp(doctor_roles[i], "POZ") == 0) {
                msg_queue_type = 2;
            } else if (strcmp(doctor_roles[i], "kardiolog") == 0) {
                msg_queue_type = 3;
            } else if (strcmp(doctor_roles[i], "okulista") == 0) {
                msg_queue_type = 4;
            } else if (strcmp(doctor_roles[i], "pediatra") == 0) {
                msg_queue_type = 5;
            } else if (strcmp(doctor_roles[i], "lekarz_medycyny_pracy") == 0) {
                msg_queue_type = 6;
            } else {
                msg_queue_type = -1; // Błąd, nieznana rola
            }
            sprintf(msg_queue_id_str, "%d", msg_queues[msg_queue_type]);

            // **POPRAWIONE: Przekazywanie ID kolejek specjalistów jako argumentów**
            if (strcmp(doctor_roles[i], "POZ") == 0) { // Tylko dla lekarzy POZ przekazujemy ID kolejek specjalistów
                execl("./doctor", "doctor", doctor_roles[i], capacity_str, msg_queue_id_str,
                      KARDIOLOG_SEM_NAME, OKULISTA_SEM_NAME, PEDIATRA_SEM_NAME, LEKARZ_MEDYCYNY_PRACY_SEM_NAME,
                      msg_queues_str[3], msg_queues_str[4], msg_queues_str[5], msg_queues_str[6],
                      NULL);
            } else { // Dla specjalistów przekazujemy tylko podstawowe argumenty
                execl("./doctor", "doctor", doctor_roles[i], capacity_str, msg_queue_id_str,
                      KARDIOLOG_SEM_NAME, OKULISTA_SEM_NAME, PEDIATRA_SEM_NAME, LEKARZ_MEDYCYNY_PRACY_SEM_NAME, // Nazwy semaforów są przekazywane, choć nie są używane przez specjalistów w obecnej implementacji
                      NULL);
            }


            perror("Błąd uruchamiania lekarza");
            exit(1);
        }
        fprintf(fp_doc, "%d\n", doctor_pids[i]);
    }
    fclose(fp_doc);

    // Tworzenie procesów pacjentów - bez zmian
    FILE *fp_pat = fopen("patient_pids.txt", "w");
    if (!fp_pat) {
        perror("Błąd przy otwieraniu patient_pids.txt do zapisu");
        exit(1);
    }
    while (current_time < CLOSING_TIME && patient_count < NUM_PATIENTS) {
        int sem_val;
        sem_getvalue(clinic_sem, &sem_val);
        if (sem_val > 0) {
            patient_pids[patient_count] = fork();
            if (patient_pids[patient_count] == 0) {
                char msg_queue_ids_str[200];
                sprintf(msg_queue_ids_str, "%d,%d,%d,%d,%d,%d",
                        msg_queues[2], msg_queues[3], msg_queues[4], msg_queues[5], msg_queues[6], msg_queues[2]);

                execl("./patient", "patient", msg_queue_ids_str, NULL);
                perror("Błąd uruchamiania pacjenta");
                exit(1);
            }
            fprintf(fp_pat, "%d\n", patient_pids[patient_count]);
            patient_count++;
            sleep(1);
        } else {
            printf("Przychodnia pełna, pacjent czeka na zewnątrz...\n");
            sleep(1); // krótszy sleep, bo i tak pacjentów jest dużo
        }
    }
    fclose(fp_pat);


    // Wysyłanie sygnałów SIGTERM i czekanie na zakończenie procesów - bez zmian
    kill(reg_pid, SIGTERM);
    for (int i = 0; i < NUM_DOCTORS; i++) {
        kill(doctor_pids[i], SIGTERM);
    }
    for (int i = 0; i < patient_count; i++) {
        if (patient_pids[i] != -1) {
            kill(patient_pids[i], SIGTERM);
        }
    }


    int status;
    pid_t result = waitpid(reg_pid, &status, WNOHANG);
    if (result == 0) {
        sleep(2);
        kill(reg_pid, SIGKILL);
        waitpid(reg_pid, &status, 0);
    }

    waitpid(reg_pid, NULL, 0);
    for (int i = 0; i < NUM_DOCTORS; i++) {
        waitpid(doctor_pids[i], NULL, 0);
    }
    for (int i = 0; i < patient_count; i++) {
        if (patient_pids[i] != -1) {
            waitpid(patient_pids[i], NULL, 0);
        }
    }


    sem_close(clinic_sem);
    sem_unlink(SEM_NAME);
    for (int i = 1; i <= 6; i++) {
        msgctl(msg_queues[i], IPC_RMID, NULL);
    }


    printf("Przychodnia zamknięta.\n");
    return 0;
}