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
#include <pthread.h>
#include <string.h>

#define SEM_NAME "/clinic_sem"

struct message {
    long msg_type;
    int patient_id;
    char msg_text[100];
    int specialist_type; // Dodane pole do przechowywania typu specjalisty (dla skierowań)
};

volatile sig_atomic_t leave_flag = 0;
sem_t *clinic_sem = NULL;


void sigusr2_handler(int signum) {
    leave_flag = 1;
    printf("Pacjent %d: Otrzymałem SIGUSR2 - natychmiast opuszczam przychodnię.\n", getpid());
    fflush(stdout);
    sem_post(clinic_sem);
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
    int has_child = (rand() % 2); // Możesz usunąć, jeśli upraszczasz model dziecka
    int is_vip = (rand() % 100 < 5); // np. 5% szans na VIP
    printf("Pacjent %d: VIP: %s\n", getpid(), is_vip ? "TAK" : "NIE"); // Komunikat o statusie VIP

    
    if (has_child) { // Możesz uprościć rezerwację do 1 miejsca, jeśli upraszczasz model dziecka
        sem_wait(clinic_sem);
        sem_wait(clinic_sem);
        printf("Pacjent %d (dorosły) wraz z dzieckiem wszedł do przychodni.\n", getpid());

    } else {
        sem_wait(clinic_sem);
        printf("Pacjent %d wszedł do przychodni.\n", getpid());
    }
    
    /* Wysyłamy komunikat rejestracyjny */
    key_t key = ftok("clinic", 65);
    int msg_queue = msgget(key, 0666);
    if (msg_queue < 0) {
        perror("Błąd przy otwieraniu kolejki komunikatów");
        exit(1);
    }
    
    struct message msg;
    msg.msg_type = 1;  // Komunikat rejestracyjny
    msg.patient_id = getpid();
    snprintf(msg.msg_text, sizeof(msg.msg_text), "Pacjent %d wchodzi do rejestracji", getpid());
    if (msgsnd(msg_queue, &msg, sizeof(msg) - sizeof(long), 0) < 0) {
        perror("Błąd przy wysyłaniu komunikatu rejestracyjnego");
    }
    printf("Pacjent %d: Zarejestrowano.\n", getpid());
    
    sleep(1);
    
    /* Wybór lekarza:
       - 60% pacjentów idzie do POZ (msg_type = 2),
       - 10% do kardiologa (msg_type = 3),
       - 10% do okulisty (msg_type = 4),
       - 10% do pediatry (msg_type = 5),
       - 10% do lekarza medycyny pracy (msg_type = 6). */
    double r = rand() / (double)RAND_MAX;
    int doctor_type;
    int actual_msg_type; // Zmienna na rzeczywisty msg_type, uwzględniający VIP

    if (r < 0.6) { doctor_type = 2; actual_msg_type = is_vip ? 8 : 2; } // msg_type 8 dla VIP POZ, 2 dla zwykłego POZ
    else if (r < 0.7) { doctor_type = 3; actual_msg_type = is_vip ? 9 : 3; } // msg_type 9 dla VIP kardiolog, 3 dla zwykłego kardiolog
    else if (r < 0.8) { doctor_type = 4; actual_msg_type = is_vip ? 10 : 4; } // msg_type 10 dla VIP okulista, 4 dla zwykłego okulista
    else if (r < 0.9) { doctor_type = 5; actual_msg_type = is_vip ? 11 : 5; } // msg_type 11 dla VIP pediatra, 5 dla zwykłego pediatra
    else { doctor_type = 6; actual_msg_type = is_vip ? 12 : 6; } // msg_type 12 dla VIP medycyny pracy, 6 dla zwykłego medycyny pracy
    
    msg.msg_type = actual_msg_type;
    switch(doctor_type) {
        case 2:
            snprintf(msg.msg_text, sizeof(msg.msg_text), "Pacjent %d udaje się do lekarza POZ", getpid());
            break;
        case 3:
            snprintf(msg.msg_text, sizeof(msg.msg_text), "Pacjent %d udaje się do kardiologa", getpid());
            break;
        case 4:
            snprintf(msg.msg_text, sizeof(msg.msg_text), "Pacjent %d udaje się do okulisty", getpid());
            break;
        case 5:
            snprintf(msg.msg_text, sizeof(msg.msg_text), "Pacjent %d udaje się do pediatry", getpid());
            break;
        case 6:
            snprintf(msg.msg_text, sizeof(msg.msg_text), "Pacjent %d udaje się do lekarza medycyny pracy", getpid());
            break;
        default:
            snprintf(msg.msg_text, sizeof(msg.msg_text), "Pacjent %d udaje się do lekarza", getpid());
            break;
    }
    
    if (msgsnd(msg_queue, &msg, sizeof(msg) - sizeof(long), 0) < 0) {
        perror("Błąd przy wysyłaniu komunikatu do lekarza");
    }
    
    switch(doctor_type) {
        case 2:
            printf("Pacjent %d: Udałem się do lekarza POZ.\n", getpid());
            break;
        case 3:
            printf("Pacjent %d: Udałem się do kardiologa.\n", getpid());
            break;
        case 4:
            printf("Pacjent %d: Udałem się do okulisty.\n", getpid());
            break;
        case 5:
            printf("Pacjent %d: Udałem się do pediatry.\n", getpid());
            break;
        case 6:
            printf("Pacjent %d: Udałem się do lekarza medycyny pracy.\n", getpid());
            break;
        default:
            break;
    }
    
    /* Symulacja pobytu u lekarza */
    sleep(2);
    
    if (has_child) { // Możesz uprościć zwalnianie semafora do 1 posta, jeśli upraszczasz model dziecka
        sem_post(clinic_sem);
        sem_post(clinic_sem);
        printf("Pacjent %d (dorosły) wraz z dzieckiem opuszcza przychodnię.\n", getpid());
    } else {
        sem_post(clinic_sem);
        printf("Pacjent %d opuszcza przychodnię.\n", getpid());
    }
    
    sem_close(clinic_sem);
    return 0;
}