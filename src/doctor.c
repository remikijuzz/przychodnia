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

#define SIGNAL_1 1
#define CLINIC_CLOSED 2
#define CLINIC_TERMINATED 3 // Nowa przyczyna zakończenia - SIGTERM

// extern int clinic_open; // USUNIĘTO - nie używamy już globalnej clinic_open

struct message {
    long msg_type;      // Typ wiadomości – decyduje, która kolejka
    int patient_id;     // Identyfikator pacjenta
    char msg_text[100]; // Dodatkowy tekst (np. informacja o skierowaniu)
    int specialist_type; // Dodane pole do przechowywania typu specjalisty (dla skierowań)
};

volatile sig_atomic_t finish_flag = 0;
int finish_flag_reason = 0; // Dodana zmienna do przechowywania przyczyny finish_flag
char *role_global; // Dodana zmienna globalna do przechowywania roli lekarza

void sigusr1_handler(int signum) { // Obsługa sygnału 1 (zmiana handlera)
    finish_flag = 1;
    finish_flag_reason = SIGNAL_1; // Ustaw przyczynę na SIGNAL_1
    printf("Lekarz %s (PID: %d): Otrzymałem SIGUSR1 - kończę pracę po obsłużeniu pacjenta.\n", role_global, getpid()); // Użyj role_global
    fflush(stdout); // Dodane fflush
}

void sigterm_handler(int signum) { // NOWY handler dla SIGTERM - zamknięcie przychodni
    finish_flag = 1;
    finish_flag_reason = CLINIC_TERMINATED; // Ustaw przyczynę na CLINIC_TERMINATED
    printf("Lekarz %s (PID: %d): Otrzymałem SIGTERM - przychodnia zamknięta, kończę pracę po obsłużeniu pacjenta.\n", role_global, getpid()); // Użyj role_global
    fflush(stdout); // Dodane fflush
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Użycie: %s <rola>\n", argv[0]);
        exit(1);
    }

    char *role = argv[1];
    role_global = role; // Zapisz rolę w zmiennej globalnej
    int capacity = 0;
    int served_count = 0; // Dodany licznik przyjętych pacjentów
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

    int msg_queue; // Zmień nazwę lokalnej zmiennej, żeby nie kolidowała z globalną w main.c
    key_t key;

    if (strcmp(role, "POZ") == 0) {
        key = ftok("clinic_poz", 65);
    } else if (strcmp(role, "kardiolog") == 0) {
        key = ftok("clinic_kardiolog", 65);
    } else if (strcmp(role, "okulista") == 0) {
        key = ftok("clinic_okulista", 65);
    } else if (strcmp(role, "pediatra") == 0) {
        key = ftok("clinic_pediatra", 65);
    } else if (strcmp(role, "lekarz_medycyny_pracy") == 0) {
        key = ftok("clinic_medycyny_pracy", 65);
    }

    msg_queue = msgget(key, 0666);
    if (msg_queue == -1) {
        perror("Błąd przy otwieraniu kolejki komunikatów");
        exit(1);
    }

    // Zapis PID-u lekarza do pliku, przed instalacją handlerów sygnałów
    char pid_filename[50];
    snprintf(pid_filename, sizeof(pid_filename), "%s_pid.txt", role);
    FILE *pid_fp = fopen(pid_filename, "w");
    if (pid_fp == NULL) {
        perror("Błąd przy otwieraniu pliku PID");
        exit(EXIT_FAILURE);
    }
    fprintf(pid_fp, "%d\n", getpid());
    fclose(pid_fp);

    if (signal(SIGUSR1, sigusr1_handler) == SIG_ERR) {
        perror("Błąd instalacji handlera SIGUSR1");
        exit(EXIT_FAILURE);
    }
    if (signal(SIGTERM, sigterm_handler) == SIG_ERR) {
        perror("Błąd instalacji handlera SIGTERM");
        exit(EXIT_FAILURE);
    }


    srand(time(NULL) ^ getpid());
    struct message msg;

    while (!finish_flag) { // Pracuj dopóki nie ma sygnału zakończenia (clinic_open USUNIĘTO)
        if (served_count >= capacity) {
            printf("Lekarz %s (PID: %d): Limit pacjentów osiągnięty. Odmawiam przyjęć.\n", role, getpid());
            sleep(1); // Lekarz czeka na ewentualne sygnały zakończenia pracy
            continue; // Ponownie sprawdź warunek pętli (finish_flag)
        }

        struct message vip_msg;
        int vip_msg_type = 0;
        if (strcmp(role, "POZ") == 0) vip_msg_type = 8;
        else if (strcmp(role, "kardiolog") == 0) vip_msg_type = 9;
        else if (strcmp(role, "okulista") == 0) vip_msg_type = 10;
        else if (strcmp(role, "pediatra") == 0) vip_msg_type = 11;
        else if (strcmp(role, "lekarz_medycyny_pracy") == 0) vip_msg_type = 12;


        ssize_t vip_ret = msgrcv(msg_queue, &vip_msg, sizeof(vip_msg) - sizeof(long), vip_msg_type /* VIP  */, IPC_NOWAIT); // Sprawdź VIP  NIEBLOKUJĄCO
        if (vip_ret > 0) {
            msg = vip_msg; // Jeśli VIP jest, obsłuż go!
            printf("Lekarz %s (PID: %d): Obsługuję pacjenta VIP %d. Wiadomość: %s\n", role, getpid(), msg.patient_id, msg.msg_text);
            served_count++; // Inkrementuj licznik
            if (strcmp(role, "POZ") == 0) {
                int referChance = rand() % 100;
                if (referChance < 20 && served_count < capacity) {
                    int specialistIndex = (rand() % 4) + 3;
                    struct message ref_msg;
                    ref_msg.msg_type = 7; // Użyj nowego msg_type = 7 dla SKIEROWANIA do rejestracji
                    ref_msg.patient_id = msg.patient_id;
                    strcpy(ref_msg.msg_text, "Skierowanie z POZ");
                    ref_msg.specialist_type = specialistIndex; // Dodaj informację o typie specjalisty

                    key_t registration_key = ftok("clinic", 65); // Klucz do kolejki rejestracji
                    int registration_queue = msgget(registration_key, 0666);
                    if (registration_queue == -1) {
                        perror("Błąd przy otwieraniu kolejki rejestracji przy wysyłaniu skierowania");
                    } else {
                        if (msgsnd(registration_queue, &ref_msg, sizeof(ref_msg) - sizeof(long), 0) < 0) { // Wyślij do rejestracji
                            perror("Błąd przy wysyłaniu skierowania do rejestracji");
                        } else {
                            char *specRole = (specialistIndex == 3) ? "kardiolog" :
                                              (specialistIndex == 4) ? "okulista" :
                                              (specialistIndex == 5) ? "pediatra" : "lekarz medycyny pracy";
                            printf("Lekarz POZ (PID: %d): Skierowałem pacjenta VIP %d do %s (skierowanie do rejestracji).\n", getpid(), msg.patient_id, specRole);
                        }
                    }
                }
            }
            continue; // Wróć na początek pętli, żeby ZNOWU sprawdzić VIP przed zwykłymi pacjentami
        }

        // Jeśli NIE MA komunikatu VIP (vip_ret <= 0), to przejdź do odbierania ZWYKŁYCH pacjentów (BLOKUJĄCO)
        ssize_t ret = msgrcv(msg_queue, &msg, sizeof(msg) - sizeof(long), msg_type /* ZWYKŁY */, 0); // Odbierz ZWYKŁEGO pacjenta  BLOKUJĄCO
        if (ret < 0) {
            if (errno == EINTR && finish_flag)
                break;
            perror("Błąd przy odbieraniu komunikatu");
            continue;
        }

        if (strcmp(role, "POZ") == 0) {
            printf("Lekarz POZ (PID: %d): Przyjąłem pacjenta %d. Wiadomość: %s\n", role, getpid(), msg.patient_id, msg.msg_text);
            served_count++;
            int referChance = rand() % 100;
            if (referChance < 20 && served_count < capacity) {
                int specialistIndex = (rand() % 4) + 3;
                struct message ref_msg;
                ref_msg.msg_type = 7; // Użyj nowego msg_type = 7 dla SKIEROWANIA do rejestracji
                ref_msg.patient_id = msg.patient_id;
                strcpy(ref_msg.msg_text, "Skierowanie z POZ");
                ref_msg.specialist_type = specialistIndex; // Dodaj informację o typie specjalisty

                key_t registration_key = ftok("clinic", 65); // Klucz do kolejki rejestracji
                int registration_queue = msgget(registration_key, 0666);
                if (registration_queue == -1) {
                    perror("Błąd przy otwieraniu kolejki rejestracji przy wysyłaniu skierowania");
                } else {
                    if (msgsnd(registration_queue, &ref_msg, sizeof(ref_msg) - sizeof(long), 0) < 0) { // Wyślij do rejestracji
                        perror("Błąd przy wysyłaniu skierowania do rejestracji");
                    } else {
                        char *specRole = (specialistIndex == 3) ? "kardiolog" :
                                          (specialistIndex == 4) ? "okulista" :
                                          (specialistIndex == 5) ? "pediatra" : "lekarz medycyny pracy";
                        printf("Lekarz POZ (PID: %d): Skierowałem pacjenta %d do %s (skierowanie do rejestracji).\n", getpid(), msg.patient_id, specRole);
                    }
                }
            }
        } else {
            printf("Specjalista %s (PID: %d): Przyjąłem pacjenta %d. Wiadomość: %s\n", role, getpid(), msg.patient_id, msg.msg_text);
            served_count++;
            int examChance = rand() % 100;
            if (examChance < 10 && served_count < capacity) {
                printf("Specjalista %s (PID: %d): Kieruję pacjenta %d na badanie ambulatoryjne.\n", role, getpid(), msg.patient_id);
                sleep(1);
                printf("Specjalista %s (PID: %d): Pacjent %d wrócił po badaniu i jest obsługiwany bez kolejki.\n", role, getpid(), msg.patient_id);
                // **Obsługa pacjenta BEZPOŚREDNIO po badaniu - "bez kolejki" (uproszczenie)**
                printf("Specjalista %s (PID: %d): Przyjąłem pacjenta %d (po badaniu ambulatoryjnym). Wiadomość: %s\n", role, getpid(), msg.patient_id, msg.msg_text);
                served_count++; // Inkrementuj licznik SERVED dopiero tutaj, po DWA razy obsłużonym pacjencie (wizyta + badanie)
                if (served_count >= capacity) break; // Sprawdź limit ponownie po obsłużeniu pacjenta po badaniu
            }
        }
    }

    // Po wyjściu z pętli while (sygnał zakończenia), OBSŁUŻ RESZTĘ KOLEJKI PACJENTÓW

    printf("Lekarz %s (PID: %d): Otrzymałem sygnał zakończenia - kończę pracę po obsłużeniu pozostałych pacjentów z kolejki...\n", role, getpid());

    while (1) { // Druga pętla while do OBSŁUGI POZOSTAŁYCH PACJENTÓW z kolejki
        ssize_t ret = msgrcv(msg_queue, &msg, sizeof(msg) - sizeof(long), msg_type, IPC_NOWAIT); // Odbierz pacjenta NIEBLOKUJĄCO
        if (ret > 0) {
            printf("Lekarz %s (PID: %d): Przyjmuję pacjenta %d Z KOLEJKI PO ZAMKNIĘCIU. Wiadomość: %s\n", role, getpid(), msg.patient_id, msg.msg_text);
            served_count++; // Inkrementuj licznik
            // **WAŻNE: TUTAJ NIE WOLNO JUŻ KIEROWAĆ NA BADANIA ani AMBULATORYJNE, ani DO SPECJALISTÓW**
            // POMIŃ fragment kodu z referChance i examChance !!!
        } else {
            if (errno == ENOMSG || ret <= 0) { // Kolejka pusta LUB błąd (np. EINTR po sygnale)
                break; // Wyjdź z drugiej pętli while, kolejka OBSŁUŻONA lub pusta
            } else if (errno == EINTR && finish_flag) { // Obsługa EINTR i finish_flag w drugiej pętli (dodatkowe zabezpieczenie)
                break;
            } else {
                perror("Błąd przy odbieraniu komunikatów z kolejki PO ZAMKNIĘCIU");
                break; // W razie błędu przerwij obsługę kolejki po zamknięciu
            }
        }
    }


    int not_admitted = capacity - served_count; // W uproszczonej wersji not_admitted = 0, ale zostawiamy dla potencjalnych przyszłych rozszerzeń.
    FILE *fp = fopen("report.txt", "a");
    if (fp) {
        fprintf(fp, "RAPORT ZAMKNIĘCIA LEKARZA %s (PID: %d):\n", role, getpid()); // Dodany nagłówek raportu
        if (finish_flag_reason == SIGNAL_1) {
            fprintf(fp, "Lekarz %s (PID: %d): Kończę pracę na polecenie Dyrektora (sygnał 1). Przyjęto %d pacjentów.\n", role, getpid(), served_count);
        } else if (finish_flag_reason == CLINIC_TERMINATED) {
            fprintf(fp, "Lekarz %s (PID: %d): Przychodnia zamknięta (sygnał 15 - SIGTERM). Przyjęto %d pacjentów.\n", role, getpid(), served_count); // Zaktualizowany komunikat dla SIGTERM
        }
         else {
             fprintf(fp, "Lekarz %s (PID: %d): Kończę pracę. Nieprzyjętych pacjentów: %d\n", role, getpid(), not_admitted); // W uproszczonej wersji not_admitted=0
        }

        fclose(fp);
    } else {
        perror("Błąd przy otwieraniu report.txt");
    }

    printf("Lekarz %s (PID: %d): Zakończyłem przyjmowanie pacjentów (przyjęto %d pacjentów).\n", role, getpid(), served_count);
    return 0;
}