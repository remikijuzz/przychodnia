#include "clinic.h"

volatile sig_atomic_t leave_flag = 0;
sem_t *clinic_sem = NULL;

void sigusr2_handler(int signum) {
    leave_flag = 1;
    printf("Pacjent %d: Otrzymałem SIGUSR2 – opuszczam przychodnię.\n", getpid());
    fflush(stdout);
    sem_post(clinic_sem);
    exit(0);
}

int main(void) {
    struct sigaction sa;
    sa.sa_handler = sigusr2_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("Błąd przy instalacji handlera SIGUSR2");
        exit(1);
    }
    
    clinic_sem = sem_open(SEM_NAME, 0);
    if (clinic_sem == SEM_FAILED) {
        perror("Błąd przy otwieraniu semafora w pacjencie");
        exit(1);
    }
    
    srand(time(NULL) ^ getpid());
    int has_child = (rand() % 2);
    int is_vip = (rand() % 100 < 5);
    printf("Pacjent %d: VIP: %s\n", getpid(), is_vip ? "TAK" : "NIE");
    
    if (has_child) {
        sem_wait(clinic_sem);
        sem_wait(clinic_sem);
        printf("Pacjent %d (dorosły) wraz z dzieckiem wchodzi do przychodni.\n", getpid());
    } else {
        sem_wait(clinic_sem);
        printf("Pacjent %d wchodzi do przychodni.\n", getpid());
    }
    
    /* Wysyłanie komunikatu rejestracyjnego */
    key_t key = ftok("clinic", 65);
    int reg_queue = msgget(key, 0666);
    if (reg_queue < 0) {
        perror("Błąd przy otwieraniu kolejki rejestracji");
        exit(1);
    }
    
    struct message msg;
    msg.msg_type = MSG_TYPE_REGISTRATION;
    msg.patient_id = getpid();
    snprintf(msg.msg_text, sizeof(msg.msg_text), "Pacjent %d rejestruje się", getpid());
    if (msgsnd(reg_queue, &msg, sizeof(msg) - sizeof(long), 0) < 0)
        perror("Błąd przy wysyłaniu komunikatu rejestracyjnego");
    printf("Pacjent %d: Zarejestrowany.\n", getpid());
    sleep(1);
    
    /* Wybór lekarza – 60% do POZ, po 10% do pozostałych */
    double r = rand() / (double)RAND_MAX;
    int doctor_type;
    int actual_msg_type;
    if (r < 0.6) { doctor_type = MSG_TYPE_POZ; actual_msg_type = is_vip ? MSG_TYPE_VIP_POZ : MSG_TYPE_POZ; }
    else if (r < 0.7) { doctor_type = MSG_TYPE_KARDIOLOG; actual_msg_type = is_vip ? MSG_TYPE_VIP_KARDIOLOG : MSG_TYPE_KARDIOLOG; }
    else if (r < 0.8) { doctor_type = MSG_TYPE_OKULISTA; actual_msg_type = is_vip ? MSG_TYPE_VIP_OKULISTA : MSG_TYPE_OKULISTA; }
    else if (r < 0.9) { doctor_type = MSG_TYPE_PEDIATRA; actual_msg_type = is_vip ? MSG_TYPE_VIP_PEDIATRA : MSG_TYPE_PEDIATRA; }
    else { doctor_type = MSG_TYPE_MEDYCYNY_PRACY; actual_msg_type = is_vip ? MSG_TYPE_VIP_MEDYCYNY_PRACY : MSG_TYPE_MEDYCYNY_PRACY; }
    
    msg.msg_type = actual_msg_type;
    switch(doctor_type) {
        case MSG_TYPE_POZ:
            snprintf(msg.msg_text, sizeof(msg.msg_text), "Pacjent %d idzie do POZ", getpid());
            break;
        case MSG_TYPE_KARDIOLOG:
            snprintf(msg.msg_text, sizeof(msg.msg_text), "Pacjent %d idzie do kardiologa", getpid());
            break;
        case MSG_TYPE_OKULISTA:
            snprintf(msg.msg_text, sizeof(msg.msg_text), "Pacjent %d idzie do okulisty", getpid());
            break;
        case MSG_TYPE_PEDIATRA:
            snprintf(msg.msg_text, sizeof(msg.msg_text), "Pacjent %d idzie do pediatry", getpid());
            break;
        case MSG_TYPE_MEDYCYNY_PRACY:
            snprintf(msg.msg_text, sizeof(msg.msg_text), "Pacjent %d idzie do lekarza medycyny pracy", getpid());
            break;
        default:
            snprintf(msg.msg_text, sizeof(msg.msg_text), "Pacjent %d idzie do lekarza", getpid());
            break;
    }
    
    if (msgsnd(reg_queue, &msg, sizeof(msg) - sizeof(long), 0) < 0)
        perror("Błąd przy wysyłaniu komunikatu do lekarza");
    
    switch(doctor_type) {
        case MSG_TYPE_POZ:
            printf("Pacjent %d: Idę do POZ.\n", getpid());
            break;
        case MSG_TYPE_KARDIOLOG:
            printf("Pacjent %d: Idę do kardiologa.\n", getpid());
            break;
        case MSG_TYPE_OKULISTA:
            printf("Pacjent %d: Idę do okulisty.\n", getpid());
            break;
        case MSG_TYPE_PEDIATRA:
            printf("Pacjent %d: Idę do pediatry.\n", getpid());
            break;
        case MSG_TYPE_MEDYCYNY_PRACY:
            printf("Pacjent %d: Idę do lekarza medycyny pracy.\n", getpid());
            break;
        default:
            break;
    }
    
    sleep(2);
    
    if (has_child) {
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
