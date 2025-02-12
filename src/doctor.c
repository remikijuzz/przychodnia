#include "clinic.h"

volatile sig_atomic_t finish_flag = 0;
int finish_flag_reason = 0;
char *role_global = NULL;

void sigusr1_handler(int signum) {
    finish_flag = 1;
    finish_flag_reason = SIGNAL_1;
    printf("Lekarz %s (PID: %d): Otrzymałem SIGUSR1 – kończę pracę po obsłużeniu pacjenta.\n", role_global, getpid());
    fflush(stdout);
}

void sigterm_handler(int signum) {
    finish_flag = 1;
    finish_flag_reason = CLINIC_TERMINATED;
    printf("Lekarz %s (PID: %d): Otrzymałem SIGTERM – przychodnia zamknięta, kończę pracę po obsłużeniu pacjenta.\n", role_global, getpid());
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <rola>\n", argv[0]);
        exit(1);
    }
    role_global = argv[1];
    int capacity = (strcmp(role_global, "POZ") == 0) ? 30 : 10;
    int served_count = 0;

    int msg_type = 0;
    if (strcmp(role_global, "POZ") == 0) {
        msg_type = MSG_TYPE_POZ;
    } else if (strcmp(role_global, "kardiolog") == 0) {
        msg_type = MSG_TYPE_KARDIOLOG;
    } else if (strcmp(role_global, "okulista") == 0) {
        msg_type = MSG_TYPE_OKULISTA;
    } else if (strcmp(role_global, "pediatra") == 0) {
        msg_type = MSG_TYPE_PEDIATRA;
    } else if (strcmp(role_global, "lekarz_medycyny_pracy") == 0) {
        msg_type = MSG_TYPE_MEDYCYNY_PRACY;
    } else {
        fprintf(stderr, "Nieznana rola: %s\n", role_global);
        exit(1);
    }

    /* Wybór kolejki komunikatów na podstawie roli */
    key_t key;
    if (strcmp(role_global, "POZ") == 0)
        key = ftok("clinic_poz", 65);
    else if (strcmp(role_global, "kardiolog") == 0)
        key = ftok("clinic_kardiolog", 65);
    else if (strcmp(role_global, "okulista") == 0)
        key = ftok("clinic_okulista", 65);
    else if (strcmp(role_global, "pediatra") == 0)
        key = ftok("clinic_pediatra", 65);
    else if (strcmp(role_global, "lekarz_medycyny_pracy") == 0)
        key = ftok("clinic_medycyny_pracy", 65);
    int msg_queue = msgget(key, 0666);
    if (msg_queue == -1) {
        perror("Błąd przy otwieraniu kolejki komunikatów");
        exit(1);
    }

    /* Zapis PID do pliku */
    char pid_filename[50];
    snprintf(pid_filename, sizeof(pid_filename), "%s_pid.txt", role_global);
    FILE *pid_fp = fopen(pid_filename, "w");
    if (!pid_fp) { perror("Błąd przy otwieraniu pliku PID"); exit(1); }
    fprintf(pid_fp, "%d\n", getpid());
    fclose(pid_fp);

    signal(SIGUSR1, sigusr1_handler);
    signal(SIGTERM, sigterm_handler);

    srand(time(NULL) ^ getpid());
    struct message msg;
    while (!finish_flag) {
        if (served_count >= capacity) {
            printf("Lekarz %s (PID: %d): Limit pacjentów osiągnięty.\n", role_global, getpid());
            sleep(1);
            continue;
        }

        /* Obsługa komunikatów VIP (tryb nieblokujący) */
        struct message vip_msg;
        int vip_msg_type = 0;
        if (strcmp(role_global, "POZ") == 0) vip_msg_type = MSG_TYPE_VIP_POZ;
        else if (strcmp(role_global, "kardiolog") == 0) vip_msg_type = MSG_TYPE_VIP_KARDIOLOG;
        else if (strcmp(role_global, "okulista") == 0) vip_msg_type = MSG_TYPE_VIP_OKULISTA;
        else if (strcmp(role_global, "pediatra") == 0) vip_msg_type = MSG_TYPE_VIP_PEDIATRA;
        else if (strcmp(role_global, "lekarz_medycyny_pracy") == 0) vip_msg_type = MSG_TYPE_VIP_MEDYCYNY_PRACY;

        ssize_t vip_ret = msgrcv(msg_queue, &vip_msg, sizeof(vip_msg) - sizeof(long), vip_msg_type, IPC_NOWAIT);
        if (vip_ret > 0) {
            msg = vip_msg;
            printf("Lekarz %s (PID: %d): Obsługuję pacjenta VIP %d – %s\n", role_global, getpid(), msg.patient_id, msg.msg_text);
            served_count++;
            if (strcmp(role_global, "POZ") == 0 && served_count < capacity) {
                int referChance = rand() % 100;
                if (referChance < 20) {
                    int specialistIndex = (rand() % 4) + 3;
                    struct message ref_msg;
                    ref_msg.msg_type = MSG_TYPE_REFERRAL;
                    ref_msg.patient_id = msg.patient_id;
                    strcpy(ref_msg.msg_text, "Skierowanie z POZ");
                    ref_msg.specialist_type = specialistIndex;
                    key_t reg_key = ftok("clinic", 65);
                    int reg_queue = msgget(reg_key, 0666);
                    if (reg_queue != -1) {
                        if (msgsnd(reg_queue, &ref_msg, sizeof(ref_msg) - sizeof(long), 0) < 0)
                            perror("Błąd wysyłania skierowania");
                        else
                            printf("Lekarz POZ (PID: %d): Skierowałem pacjenta %d do specjalisty.\n", getpid(), msg.patient_id);
                    }
                }
            }
            continue;
        }

        /* Odbiór komunikatów standardowych – tryb blokujący */
        ssize_t ret = msgrcv(msg_queue, &msg, sizeof(msg) - sizeof(long), msg_type, 0);
        if (ret < 0) {
            if (errno == EINTR && finish_flag) break;
            perror("Błąd odbierania komunikatu");
            continue;
        }

        if (strcmp(role_global, "POZ") == 0) {
            printf("Lekarz POZ (PID: %d): Przyjąłem pacjenta %d – %s\n", getpid(), msg.patient_id, msg.msg_text);
            served_count++;
            if (served_count < capacity) {
                int referChance = rand() % 100;
                if (referChance < 20) {
                    int specialistIndex = (rand() % 4) + 3;
                    struct message ref_msg;
                    ref_msg.msg_type = MSG_TYPE_REFERRAL;
                    ref_msg.patient_id = msg.patient_id;
                    strcpy(ref_msg.msg_text, "Skierowanie z POZ");
                    ref_msg.specialist_type = specialistIndex;
                    key_t reg_key = ftok("clinic", 65);
                    int reg_queue = msgget(reg_key, 0666);
                    if (reg_queue != -1) {
                        if (msgsnd(reg_queue, &ref_msg, sizeof(ref_msg) - sizeof(long), 0) < 0)
                            perror("Błąd wysyłania skierowania");
                        else
                            printf("Lekarz POZ (PID: %d): Skierowałem pacjenta %d do specjalisty.\n", getpid(), msg.patient_id);
                    }
                }
            }
        } else {
            printf("Specjalista %s (PID: %d): Przyjąłem pacjenta %d – %s\n", role_global, getpid(), msg.patient_id, msg.msg_text);
            served_count++;
            if ((rand() % 100) < 10 && served_count < capacity) {
                printf("Specjalista %s (PID: %d): Kieruję pacjenta %d na badanie ambulatoryjne.\n", role_global, getpid(), msg.patient_id);
                sleep(1);
                printf("Specjalista %s (PID: %d): Pacjent %d wrócił po badaniu.\n", role_global, getpid(), msg.patient_id);
                served_count++;
            }
        }
    }
    
    printf("Lekarz %s (PID: %d): Kończę pracę.\n", role_global, getpid());
    /* Obsługa pozostałych komunikatów po sygnale zakończenia */
    while (msgrcv(msg_queue, &msg, sizeof(msg) - sizeof(long), msg_type, IPC_NOWAIT) > 0) {
        printf("Lekarz %s (PID: %d): Obsługuję pacjenta %d z kolejki po zamknięciu.\n", role_global, getpid(), msg.patient_id);
    }
    FILE *fp = fopen("report.txt", "a");
    if (fp) {
        fprintf(fp, "RAPORT %s (PID: %d): Obsłużono %d pacjentów.\n", role_global, getpid(), served_count);
        fclose(fp);
    }
    return 0;
}
