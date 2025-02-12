#define _POSIX_C_SOURCE 200809L
#include "clinic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sys/msg.h>
#include <unistd.h>
#include <poll.h>
#include <sys/signalfd.h>

/* Globalne zmienne */
volatile sig_atomic_t finish_flag = 0;
int finish_flag_reason = 0;
char *role_global = NULL;

/* Self-pipe deskryptor – obsługa SIGUSR1 za pomocą signalfd */
int sfd;

void sigterm_handler(int signum) {
    finish_flag = 1;
    finish_flag_reason = CLINIC_TERMINATED;
    printf("Lekarz %s (PID: %d): Otrzymałem SIGTERM – przychodnia zamknięta, kończę pracę.\n", role_global, getpid());
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <rola>\n", argv[0]);
        exit(1);
    }
    role_global = argv[1];
    int capacity = ( (strcasecmp(role_global, "poz1") == 0 || strcasecmp(role_global, "poz2") == 0) ? 30 : 10 );
    int served_count = 0;

    int msg_type = 0;
    if (strcasecmp(role_global, "poz1") == 0 || strcasecmp(role_global, "poz2") == 0)
        msg_type = MSG_TYPE_POZ;
    else if (strcasecmp(role_global, "kardiolog") == 0)
        msg_type = MSG_TYPE_KARDIOLOG;
    else if (strcasecmp(role_global, "okulista") == 0)
        msg_type = MSG_TYPE_OKULISTA;
    else if (strcasecmp(role_global, "pediatra") == 0)
        msg_type = MSG_TYPE_PEDIATRA;
    else if (strcasecmp(role_global, "lekarz_medycyny_pracy") == 0)
        msg_type = MSG_TYPE_MEDYCYNY_PRACY;
    else {
        fprintf(stderr, "Nieznana rola: %s\n", role_global);
        exit(1);
    }

    /* Wybór kolejki komunikatów na podstawie roli */
    key_t key;
    if (strcasecmp(role_global, "poz1") == 0 || strcasecmp(role_global, "poz2") == 0)
        key = ftok("clinic_poz", 65);
    else if (strcasecmp(role_global, "kardiolog") == 0)
        key = ftok("clinic_kardiolog", 65);
    else if (strcasecmp(role_global, "okulista") == 0)
        key = ftok("clinic_okulista", 65);
    else if (strcasecmp(role_global, "pediatra") == 0)
        key = ftok("clinic_pediatra", 65);
    else if (strcasecmp(role_global, "lekarz_medycyny_pracy") == 0)
        key = ftok("clinic_medycyny_pracy", 65);
    int msg_queue = msgget(key, 0666);
    if (msg_queue == -1) {
        perror("Błąd przy otwieraniu kolejki komunikatów");
        exit(1);
    }

    /* Zapis PID do pliku – dla POZ (poz1/poz2) używamy nazw: poz1_pid.txt lub poz2_pid.txt */
    {
        char pid_filename[50];
        if (strcasecmp(role_global, "poz1") == 0)
            snprintf(pid_filename, sizeof(pid_filename), "poz1_pid.txt");
        else if (strcasecmp(role_global, "poz2") == 0)
            snprintf(pid_filename, sizeof(pid_filename), "poz2_pid.txt");
        else
            snprintf(pid_filename, sizeof(pid_filename), "%s_pid.txt", role_global);
        FILE *pid_fp = fopen(pid_filename, "w");
        if (!pid_fp) { perror("Błąd przy otwieraniu pliku PID"); exit(1); }
        fprintf(pid_fp, "%d\n", getpid());
        fclose(pid_fp);
    }

    /* Blokujemy SIGUSR1 globalnie i tworzymy signalfd do jego monitorowania */
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
        perror("sigprocmask");
        exit(1);
    }
    sfd = signalfd(-1, &mask, 0);
    if (sfd == -1) {
        perror("signalfd");
        exit(1);
    }

    /* Instalacja handlera SIGTERM */
    {
        struct sigaction sa_term;
        sa_term.sa_handler = sigterm_handler;
        sigemptyset(&sa_term.sa_mask);
        sa_term.sa_flags = 0;
        if (sigaction(SIGTERM, &sa_term, NULL) == -1) {
            perror("sigaction SIGTERM");
            exit(1);
        }
    }

    srand(time(NULL) ^ getpid());
    struct message msg;

    /* Ustawiamy poll dla signalfd – timeout 10 ms */
    struct pollfd pfds[1];
    pfds[0].fd = sfd;
    pfds[0].events = POLLIN;

    while (1) {
        if (finish_flag) {
            printf("Finish flag ustawione, wychodzę z pętli.\n");
            break;
        }
        /* Poll signalfd */
        int poll_ret = poll(pfds, 1, 10);
        if (poll_ret > 0 && (pfds[0].revents & POLLIN)) {
            struct signalfd_siginfo fdsi;
            ssize_t s = read(sfd, &fdsi, sizeof(fdsi));
            if (s != sizeof(fdsi))
                perror("read signalfd");
            else {
                printf("Otrzymano SIGUSR1 poprzez signalfd, wychodzę z pętli.\n");
            }
            finish_flag = 1;
            break;
        }
        
        /* Obsługa komunikatów VIP (tryb nieblokujący) */
        struct message vip_msg;
        int vip_msg_type = 0;
        if (strcasecmp(role_global, "poz1") == 0 || strcasecmp(role_global, "poz2") == 0)
            vip_msg_type = MSG_TYPE_VIP_POZ;
        else if (strcasecmp(role_global, "kardiolog") == 0)
            vip_msg_type = MSG_TYPE_VIP_KARDIOLOG;
        else if (strcasecmp(role_global, "okulista") == 0)
            vip_msg_type = MSG_TYPE_VIP_OKULISTA;
        else if (strcasecmp(role_global, "pediatra") == 0)
            vip_msg_type = MSG_TYPE_VIP_PEDIATRA;
        else if (strcasecmp(role_global, "lekarz_medycyny_pracy") == 0)
            vip_msg_type = MSG_TYPE_VIP_MEDYCYNY_PRACY;
        
        ssize_t vip_ret = msgrcv(msg_queue, &vip_msg, sizeof(vip_msg) - sizeof(long), vip_msg_type, IPC_NOWAIT);
        if (vip_ret > 0) {
            if (finish_flag)
                break;
            msg = vip_msg;
            printf("Lekarz %s (PID: %d): Obsługuję pacjenta VIP %d – %s\n",
                   role_global, getpid(), msg.patient_id, msg.msg_text);
            served_count++;
            if ((strcasecmp(role_global, "poz1") == 0 || strcasecmp(role_global, "poz2") == 0)
                && served_count < capacity) {
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
                            printf("Lekarz POZ (PID: %d): Skierowałem pacjenta %d do specjalisty.\n",
                                   getpid(), msg.patient_id);
                    }
                }
            }
            continue;
        }
        
        /* Odbiór komunikatów standardowych – tryb nieblokujący */
        ssize_t ret = msgrcv(msg_queue, &msg, sizeof(msg) - sizeof(long), msg_type, IPC_NOWAIT);
        if (ret < 0) {
            if (errno == ENOMSG) {
                usleep(10000); // 10 ms
                continue;
            }
            if (errno == EINTR && finish_flag)
                break;
            perror("Błąd odbierania komunikatu");
            continue;
        }
        
        if (finish_flag)
            break;
        
        if ( (strcasecmp(role_global, "poz1") == 0) || (strcasecmp(role_global, "poz2") == 0) ) {
            printf("Lekarz POZ (PID: %d): Przyjąłem pacjenta %d – %s\n",
                   getpid(), msg.patient_id, msg.msg_text);
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
                            printf("Lekarz POZ (PID: %d): Skierowałem pacjenta %d do specjalisty.\n",
                                   getpid(), msg.patient_id);
                    }
                }
            }
        } else {
            printf("Specjalista %s (PID: %d): Przyjąłem pacjenta %d – %s\n",
                   role_global, getpid(), msg.patient_id, msg.msg_text);
            served_count++;
            if ((rand() % 100) < 10 && served_count < capacity) {
                printf("Specjalista %s (PID: %d): Kieruję pacjenta %d na badanie ambulatoryjne.\n",
                       role_global, getpid(), msg.patient_id);
                sleep(1);
                printf("Specjalista %s (PID: %d): Pacjent %d wrócił po badaniu.\n",
                       role_global, getpid(), msg.patient_id);
                served_count++;
            }
        }
    }
    
    printf("Lekarz %s (PID: %d): Kończę pracę.\n", role_global, getpid());
    while (msgrcv(msg_queue, &msg, sizeof(msg) - sizeof(long), msg_type, IPC_NOWAIT) > 0) {
        printf("Lekarz %s (PID: %d): Obsługuję pacjenta %d z kolejki po zamknięciu.\n",
               role_global, getpid(), msg.patient_id);
    }
    FILE *fp = fopen("report.txt", "a");
    if (fp) {
        fprintf(fp, "RAPORT %s (PID: %d): Obsłużono %d pacjentów.\n", role_global, getpid(), served_count);
        fclose(fp);
    }
    return 0;
}
