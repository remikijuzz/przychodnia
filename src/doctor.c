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
#include <semaphore.h>
#include <fcntl.h>

struct message {
    long msg_type;
    int patient_id;
    char msg_text[100];
};

volatile sig_atomic_t finish_flag = 0;
void sigusr1_handler(int signum) {
    finish_flag = 1;
}

int main(int argc, char *argv[]) {
    char *role = argv[1];

    // Sprawdzanie argumentów - bez zmian
    if (strcmp(role, "POZ") == 0) {
        if (argc < 12) {
            fprintf(stderr, "Użycie dla lekarza POZ: ...\n");
            exit(1);
        }
    } else {
        if (argc < 8) {
            fprintf(stderr, "Użycie dla specjalisty: ...\n");
            exit(1);
        }
    }

    printf("Doctor PID: %d, Rola: %s\n", getpid(), role);
    for (int i = 0; i < argc; i++) {
        printf("argv[%d]: %s\n", i, argv[i]);
    }

    int capacity = atoi(argv[2]);
    int msg_queue_id = atoi(argv[3]);
    const char *kardiolog_sem_name = argv[4];
    const char *okulista_sem_name = argv[5];
    const char *pediatra_sem_name = argv[6];
    const char *lekarz_pracy_sem_name = argv[7];
    int kardiolog_queue_id = atoi(argv[8]); // Kolejki specjalistów - docelowe kolejki FIFO skierowań
    int okulista_queue_id = atoi(argv[9]);
    int pediatra_queue_id = atoi(argv[10]);
    int lekarz_pracy_queue_id = atoi(argv[11]);

    int msg_queue = msg_queue_id; // Dla specjalistów msg_queue to KOLEJKA SKIEROWAŃ (FIFO), dla POZ to KOLEJKA PACJENTÓW DO POZ
    int served = 0;
    srand(time(NULL) ^ getpid());
    struct message msg;

    while (served < capacity && !finish_flag) {
        // Lekarz (POZ lub specjalista) odbiera komunikat z KOLEJKI FIFO
        ssize_t ret = msgrcv(msg_queue, &msg, sizeof(msg) - sizeof(long), 0, 0);

        if (ret < 0) {
            if (errno == EINTR && finish_flag)
                break;
            perror("Błąd przy odbieraniu komunikatu przez lekarza");
            continue;
        }

        if (strcmp(role, "POZ") == 0) {
            // Logika lekarza POZ - ZMIANY TUTAJ
            printf("Lekarz POZ (PID: %d): Przyjąłem pacjenta %d, wiadomość: %s.\n", getpid(), msg.patient_id, msg.msg_text);
            served++;
            int referChance = rand() % 100;
            if (referChance < 20 && served < capacity) {
                int specialistIndex = (rand() % 4) + 3;
                const char *specialist_sem_name = NULL;
                int specialist_queue_id_to_send = 0;
                char *specRole = NULL;

                switch (specialistIndex) {
                    case 3: specialist_sem_name = kardiolog_sem_name; specialist_queue_id_to_send = kardiolog_queue_id; specRole = "kardiolog"; break;
                    case 4: specialist_sem_name = okulista_sem_name; specialist_queue_id_to_send = okulista_queue_id; specRole = "okulista"; break;
                    case 5: specialist_sem_name = pediatra_sem_name; specialist_queue_id_to_send = pediatra_queue_id; specRole = "pediatra"; break;
                    case 6: specialist_sem_name = lekarz_pracy_sem_name; specialist_queue_id_to_send = lekarz_pracy_queue_id; specRole = "lekarz medycyny pracy"; break;
                }
                sem_t *specialist_sem = sem_open(specialist_sem_name, 0);
                if (specialist_sem == SEM_FAILED) {
                    perror("Błąd przy otwieraniu semafora specjalisty przez POZ");
                    continue;
                }

                // SPRAWDZENIE DOSTĘPNOŚCI MIEJSC U SPECJALISTY (SEMAFOR) PRZED SKIEROWANIEM
                if (sem_trywait(specialist_sem) == 0) { // sem_trywait - próbuje zająć semafor, zwraca 0 w sukcesie, -1 w błędzie lub gdy semafor jest zajęty
                    struct message ref_msg;
                    ref_msg.msg_type = 1; // typ komunikatu nie ma znaczenia w kolejkach opartych na ID
                    ref_msg.patient_id = msg.patient_id;
                    strcpy(ref_msg.msg_text, "Skierowanie z POZ");
                    // WYSYŁANIE SKIEROWANIA DO KOLEJKI FIFO SPECJALISTY (KOLEJKI KOMUNIKATÓW)
                    if (msgsnd(specialist_queue_id_to_send, &ref_msg, sizeof(ref_msg) - sizeof(long), 0) < 0) {
                        perror("Błąd przy wysyłaniu skierowania do kolejki specjalisty");
                        sem_post(specialist_sem); // ZWOLNIJ SEMAFOR, JEŚLI NIE UDAŁO SIĘ WYSŁAĆ SKIEROWANIA! (WAŻNE!)
                    } else {
                        printf("Lekarz POZ (PID: %d): Skierowano pacjenta %d do %s (kolejka FIFO ID: %d).\n", getpid(), msg.patient_id, specRole, specialist_queue_id_to_send);
                    }
                } else {
                    // BRAK WOLNYCH MIEJSC U SPECJALISTY - RAPORTOWANIE
                    fprintf(stderr, "Lekarz POZ (PID: %d): Brak wolnych miejsc u %s, nie skierowano pacjenta %d.\n", getpid(), specRole, msg.patient_id);
                    FILE *fp = fopen("report.txt", "a");
                    if (fp) {
                        fprintf(fp, "Raport Lekarz POZ: Nieudane skierowanie pacjenta %d do %s (brak miejsc w kolejce FIFO).\n", msg.patient_id, specRole);
                        fclose(fp);
                    } else {
                        perror("Błąd przy otwieraniu report.txt");
                    }
                }
                 sem_close(specialist_sem); // ZAMKNIJ SEMAFOR PO UŻYCIU (ZAWSZE!)
            }
        } else { // Logika lekarza specjalisty - BEZ ZMIAN W TYM KROKU
            // Specjalista odbiera pacjentów z KOLEJKI FIFO SKIEROWAŃ
            printf("Specjalista %s (PID: %d): Przyjmuje pacjenta %d (skierowanie: %s).\n", role, getpid(), msg.patient_id, msg.msg_text);
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