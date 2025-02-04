#define _POSIX_C_SOURCE 200809L
//doctor.c
#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

struct message {
    long msg_type;      // Typ wiadomości – decyduje, która kolejka
    int patient_id;     // Identyfikator pacjenta
    char msg_text[100]; // Dodatkowy tekst (np. informacja o skierowaniu)
};

volatile sig_atomic_t finish_flag = 0;
void sigusr1_handler(int signum) {
    finish_flag = 1;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Użycie: %s <rola>\n", argv[0]);
        exit(1);
    }
    
    char *role = argv[1];
    int capacity = 0;
    int served = 0;
    if (strcmp(role, "POZ") == 0) {
        capacity = 30;
    } else if (strcmp(role, "kardiolog") == 0) {
        capacity = 10;
    } else if (strcmp(role, "okulista") == 0) {
        capacity = 10;
    } else if (strcmp(role, "pediatra") == 0) {
        capacity = 10;
    } else if (strcmp(role, "lekarz_medycyny_pracy") == 0) {
        capacity = 10;
    } else {
        fprintf(stderr, "Nieznana rola: %s\n", role);
        exit(1);
    }
    
    int msg_type = 0;
    if (strcmp(role, "POZ") == 0) {
        msg_type = 2;
    } else if (strcmp(role, "kardiolog") == 0) {
        msg_type = 3;
    } else if (strcmp(role, "okulista") == 0) {
        msg_type = 4;
    } else if (strcmp(role, "pediatra") == 0) {
        msg_type = 5;
    } else if (strcmp(role, "lekarz_medycyny_pracy") == 0) {
        msg_type = 6;
    }
    
    key_t key = ftok("clinic", 65);
    int msg_queue = msgget(key, 0666);
    if (msg_queue == -1) {
        perror("Błąd przy otwieraniu kolejki komunikatów");
        exit(1);
    }
    
    struct sigaction sa;
    sa.sa_handler = sigusr1_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("Błąd przy instalowaniu handlera SIGUSR1");
        exit(1);
    }
    
    srand(time(NULL) ^ getpid());
    struct message msg;
    
    while (served < capacity && !finish_flag) {
        ssize_t ret = msgrcv(msg_queue, &msg, sizeof(msg) - sizeof(long), msg_type, 0);
        if (ret < 0) {
            if (errno == EINTR && finish_flag)
                break;
            perror("Błąd przy odbieraniu komunikatu");
            continue;
        }
        
        if (strcmp(role, "POZ") == 0) {
            printf("Lekarz POZ (PID: %d): Przyjąłem pacjenta %d. Wiadomość: %s\n", getpid(), msg.patient_id, msg.msg_text);
            served++;
            int referChance = rand() % 100;
            if (referChance < 20 && served < capacity) {
                int specialistIndex = (rand() % 4) + 3;
                struct message ref_msg;
                ref_msg.msg_type = specialistIndex;
                ref_msg.patient_id = msg.patient_id;
                strcpy(ref_msg.msg_text, "Skierowanie z POZ");
                if (msgsnd(msg_queue, &ref_msg, sizeof(ref_msg) - sizeof(long), 0) < 0) {
                    perror("Błąd przy wysyłaniu skierowania");
                } else {
                    char *specRole = (specialistIndex == 3) ? "kardiolog" :
                                      (specialistIndex == 4) ? "okulista" :
                                      (specialistIndex == 5) ? "pediatra" : "lekarz medycyny pracy";
                    printf("Lekarz POZ (PID: %d): Skierowałem pacjenta %d do %s.\n", getpid(), msg.patient_id, specRole);
                }
            }
        } else {
            printf("Specjalista %s (PID: %d): Przyjąłem pacjenta %d. Wiadomość: %s\n", role, getpid(), msg.patient_id, msg.msg_text);
            served++;
            int examChance = rand() % 100;
            if (examChance < 10 && served < capacity) {
                printf("Specjalista %s (PID: %d): Kieruję pacjenta %d na badanie ambulatoryjne.\n", role, getpid(), msg.patient_id);
                sleep(1);
                printf("Specjalista %s (PID: %d): Pacjent %d wrócił po badaniu i jest obsługiwany bez kolejki.\n", role, getpid(), msg.patient_id);
            }
        }
    }
    
    int not_admitted = capacity - served;
    FILE *fp = fopen("report.txt", "a");
    if (fp) {
        fprintf(fp, "Lekarz %s (PID: %d): Kończę pracę. Nieprzyjętych pacjentów: %d\n", role, getpid(), not_admitted);
        fclose(fp);
    } else {
        perror("Błąd przy otwieraniu report.txt");
    }
    
    printf("Lekarz %s (PID: %d): Zakończyłem przyjmowanie pacjentów (przyjęto %d pacjentów).\n", role, getpid(), served);
    return 0;
}
