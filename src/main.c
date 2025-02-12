#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include "clinic.h"

pid_t reg_pid;
pid_t doctor_pids[NUM_DOCTORS];
pid_t patient_pids[NUM_PATIENTS];
int patient_count = 0;
int current_time = OPENING_TIME;
int clinic_open = 1;
pthread_mutex_t time_mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t *clinic_sem = NULL;

// Kolejki komunikatów – globalnie
int msg_queue, poz_queue, kardiolog_queue, okulista_queue, pediatra_queue, medycyny_pracy_queue;

void *signal_handler_thread(void *arg) {
    sigset_t waitset;
    int sig;
    sigemptyset(&waitset);
    sigaddset(&waitset, SIGUSR2);
    if (sigwait(&waitset, &sig) == 0) {
        printf("Dyrektor: Zarządzono ewakuację, prosimy o opuszczenie placówki.\n");
        kill(reg_pid, SIGTERM);
        for (int i = 0; i < NUM_DOCTORS; i++) {
            kill(doctor_pids[i], SIGTERM);
        }
        for (int i = 0; i < patient_count; i++) {
            kill(patient_pids[i], SIGTERM);
        }
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

int main(void) {
    /* Zapisanie PID głównego procesu do pliku */
    FILE *fp_main = fopen("main_pid.txt", "w");
    if (fp_main) {
        fprintf(fp_main, "%d\n", getpid());
        fclose(fp_main);
    } else {
        perror("Błąd przy zapisie main_pid.txt");
    }

    /* Blokowanie SIGUSR2 i SIGUSR1 we wszystkich wątkach */
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

    /* Utworzenie semafora symulującego maksymalną liczbę pacjentów wewnątrz */
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

    /* Tworzenie procesu rejestracji */
    reg_pid = fork();
    if (reg_pid == 0) {
        execl("./registration", "registration", NULL);
        perror("Błąd przy uruchamianiu registration");
        exit(1);
    }
    FILE *fp_reg = fopen("registration_pid.txt", "w");
    if (fp_reg) { fprintf(fp_reg, "%d\n", reg_pid); fclose(fp_reg); }
    else { perror("Błąd przy zapisie registration_pid.txt"); }

    /* Tworzenie procesów lekarzy */
    const char *doctor_roles[NUM_DOCTORS] = {"POZ", "POZ", "kardiolog", "okulista", "pediatra", "lekarz_medycyny_pracy"};
    const char *doctor_pid_files[NUM_DOCTORS] = {"poz1_pid.txt", "poz2_pid.txt", "kardiolog_pid.txt", "okulista_pid.txt", "pediatra_pid.txt", "lekarz_medycyny_pracy_pid.txt"};
    FILE *fp_doc = fopen("doctor_pids.txt", "w");
    if (!fp_doc) { perror("Błąd przy otwieraniu doctor_pids.txt"); exit(1); }
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
    if (!fp_pat) { perror("Błąd przy otwieraniu patient_pids.txt"); exit(1); }
    while (current_time < CLOSING_TIME && patient_count < NUM_PATIENTS) {
        int sem_val;
        sem_getvalue(clinic_sem, &sem_val);
        if (sem_val > 0) {
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
            printf("Przychodnia pełna, pacjent czeka...\n");
            usleep(100000);
        }
    }
    fclose(fp_pat);

    /* Powiadomienie procesów o zamknięciu */
    kill(reg_pid, SIGTERM);
    for (int i = 0; i < NUM_DOCTORS; i++) {
        kill(doctor_pids[i], SIGTERM);
    }
    for (int i = 0; i < patient_count; i++) {
        kill(patient_pids[i], SIGTERM);
    }

    /* Oczekiwanie na zakończenie procesów */
    int status;
    waitpid(reg_pid, &status, 0);
    for (int i = 0; i < NUM_DOCTORS; i++) {
        waitpid(doctor_pids[i], NULL, 0);
    }
    for (int i = 0; i < patient_count; i++) {
        waitpid(patient_pids[i], NULL, 0);
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
