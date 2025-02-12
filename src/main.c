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

// Definicje parametrów symulacji
#define NUM_DOCTORS 6
#define NUM_PATIENTS 200
#define OPENING_TIME 8   // Godzina otwarcia
#define CLOSING_TIME 16  // Godzina zamknięcia
#define MAX_PATIENTS_INSIDE 30
#define SEM_NAME "/clinic_sem"

// Globalne zmienne
int msg_queue;
int poz_queue;
int kardiolog_queue;
int okulista_queue;
int pediatra_queue;
int medycyny_pracy_queue;


int current_time = OPENING_TIME;
int clinic_open = 1;
pthread_mutex_t time_mutex = PTHREAD_MUTEX_INITIALIZER;

pid_t reg_pid;
pid_t doctor_pids[NUM_DOCTORS];
pid_t patient_pids[NUM_PATIENTS];
int patient_count = 0;
sem_t *clinic_sem = NULL;

// Wątek obsługujący sygnał SIGUSR2 – po jego odebraniu czyścimy zasoby i kończymy działanie.
void *signal_handler_thread(void *arg) {
    sigset_t waitset;
    sigemptyset(&waitset);
    sigaddset(&waitset, SIGUSR2);
    int sig;
    if (sigwait(&waitset, &sig) == 0) {
        printf("Dyrektor: Zarządzono ewakuację, prosimy o opuszczenie placówki.\n");
        fflush(stdout);
        // Zabijamy proces rejestracji, lekarzy oraz pacjentów
        kill(reg_pid, SIGTERM);
        for (int i = 0; i < NUM_DOCTORS; i++) {
            kill(doctor_pids[i], SIGTERM);
        }
        for (int i = 0; i < patient_count; i++) {
            kill(patient_pids[i], SIGTERM);
        }
        // Sprzątamy zasoby
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

// Wątek symulujący upływ czasu – co 4 sekundy symulowany czas zwiększa się o 1 godzinę.
void* time_simulation(void* arg) {
    while (current_time < CLOSING_TIME) {
        sleep(4);  // Co 4 sekundy symulujemy przeskok o 1 godzinę
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
    // Zapisujemy PID głównego procesu do pliku main_pid.txt
    FILE *fp_main = fopen("main_pid.txt", "w");
    if (fp_main) {
        fprintf(fp_main, "%d\n", getpid());
        fclose(fp_main);
    } else {
        perror("Błąd przy zapisie main_pid.txt");
    }
    
    // Blokujemy SIGUSR2 we wszystkich wątkach, aby tylko dedykowany wątek go odbierał.
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR2);
    sigaddset(&set, SIGUSR1); // Blokujemy też SIGUSR1
    if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
        perror("Błąd przy blokowaniu SIGUSR2");
        exit(1);
    }
    
    key_t key = ftok("clinic", 65);
    msg_queue = msgget(key, IPC_CREAT | 0666);
    if (msg_queue == -1) {
        perror("Błąd przy tworzeniu kolejki komunikatów");
        exit(1);
    }

    key_t key_poz = ftok("clinic_poz", 65); // Różne pliki klucza
    poz_queue = msgget(key_poz, IPC_CREAT | 0666);
    if (poz_queue == -1) {
        perror("Błąd przy tworzeniu kolejki komunikatów POZ");
        exit(1);
    }

    key_t key_kardiolog = ftok("clinic_kardiolog", 65);
    kardiolog_queue = msgget(key_kardiolog, IPC_CREAT | 0666);
    if (kardiolog_queue == -1) {
        perror("Błąd przy tworzeniu kolejki komunikatów kardiolog");
        exit(1);
    }

    key_t key_okulista = ftok("clinic_okulista", 65);
    okulista_queue = msgget(key_okulista, IPC_CREAT | 0666);
    if (okulista_queue == -1) {
        perror("Błąd przy tworzeniu kolejki komunikatów okulista");
        exit(1);
    }

    key_t key_pediatra = ftok("clinic_pediatra", 65);
    pediatra_queue = msgget(key_pediatra, IPC_CREAT | 0666);
    if (pediatra_queue == -1) {
        perror("Błąd przy tworzeniu kolejki komunikatów pediatra");
        exit(1);
    }

    key_t key_medycyny_pracy = ftok("clinic_medycyny_pracy", 65);
    medycyny_pracy_queue = msgget(key_medycyny_pracy, IPC_CREAT | 0666);
    if (medycyny_pracy_queue == -1) {
        perror("Błąd przy tworzeniu kolejki komunikatów medycyny pracy");
        exit(1);
    }
    
    sem_unlink(SEM_NAME);
    clinic_sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0666, MAX_PATIENTS_INSIDE);
    if (clinic_sem == SEM_FAILED) {
        perror("Błąd przy tworzeniu semafora");
        exit(1);
    }
    
    printf("Przychodnia otwarta od 08:00 do 16:00\n");
    
    pthread_t time_thread, sig_thread;
    pthread_create(&time_thread, NULL, time_simulation, NULL);
    pthread_create(&sig_thread, NULL, signal_handler_thread, NULL);
    
    // Tworzymy proces rejestracji
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
    
    // Tworzymy procesy lekarzy
    const char *doctor_roles[NUM_DOCTORS] = {"POZ", "POZ", "kardiolog", "okulista", "pediatra", "lekarz_medycyny_pracy"};
    const char *doctor_filenames[NUM_DOCTORS] = {"poz1_pid.txt", "poz2_pid.txt", "kardiolog_pid.txt", "okulista_pid.txt", "pediatra_pid.txt", "lekarz_medycyny_pracy_pid.txt"}; // Nazwy plików PID dla każdej roli

    FILE *fp_doc = fopen("doctor_pids.txt", "w"); // Ten plik "doctor_pids.txt" przestaje być potrzebny, ale można go zostawić i ignorować
    if (!fp_doc) {
        perror("Błąd przy otwieraniu doctor_pids.txt do zapisu");
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

        // Zapisz PID do pliku o nazwie zależnej od roli
        FILE *fp_role_pid = fopen(doctor_filenames[i], "w"); // Użyj doctor_filenames[i]
        if (fp_role_pid) {
            fprintf(fp_role_pid, "%d\n", doctor_pids[i]);
            fclose(fp_role_pid);
        } else {
            perror("Błąd przy zapisie pliku PID dla roli lekarza"); // Dodana obsługa błędu zapisu pliku PID dla roli
        }
    }
    fclose(fp_doc);
    
    // Tworzymy procesy pacjentów
    FILE *fp_pat = fopen("patient_pids.txt", "w");
    if (!fp_pat) {
        perror("Błąd przy otwieraniu patient_pids.txt do zapisu");
        exit(1);
    }
    while (current_time < CLOSING_TIME && patient_count < NUM_PATIENTS) {
        int sem_val;
        sem_getvalue(clinic_sem, &sem_val);
        if (sem_val > 0) {
            // Mamy wolne miejsce – tworzymy nowego pacjenta.
            patient_pids[patient_count] = fork();
            if (patient_pids[patient_count] == 0) {
                execl("./patient", "patient", NULL);
                perror("Błąd uruchamiania pacjenta");
                exit(1);
            }
            fprintf(fp_pat, "%d\n", patient_pids[patient_count]);
            patient_count++;
            sleep(1);
        } else {
            // Przychodnia pełna – czekamy na zwolnienie miejsca.
            printf("Przychodnia pełna, pacjent czeka na zewnątrz...\n");
            usleep(100000);  
        }
    }
    fclose(fp_pat);
    
    // Wysyłamy SIGTERM do procesów: rejestracji, lekarzy oraz (jeśli jeszcze działają) pacjentów.
    kill(reg_pid, SIGTERM);
    for (int i = 0; i < NUM_DOCTORS; i++) {
        kill(doctor_pids[i], SIGTERM);
    }
    for (int i = 0; i < patient_count; i++) {
        if (patient_pids[i] != -1) {
            kill(patient_pids[i], SIGTERM);
        }
    }
    
    // Dajemy szansę procesom na porządne zakończenie, a w razie potrzeby wymuszamy zabicie.
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
    msgctl(msg_queue, IPC_RMID, NULL);
    msgctl(poz_queue, IPC_RMID, NULL);
    msgctl(kardiolog_queue, IPC_RMID, NULL);
    msgctl(okulista_queue, IPC_RMID, NULL);
    msgctl(pediatra_queue, IPC_RMID, NULL);
    msgctl(medycyny_pracy_queue, IPC_RMID, NULL);

    
    printf("Przychodnia zamknięta.\n");
    return 0;
}