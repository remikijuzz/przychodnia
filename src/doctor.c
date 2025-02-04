// doctor.c
#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>

volatile sig_atomic_t director_signal1 = 0; // Sygnał 1 od Dyrektora
volatile sig_atomic_t director_signal2 = 0; // Sygnał 2 od Dyrektora

// Obsługa SIGUSR1 – lekarz kończy normalne przyjmowanie
void handle_sigusr1(int sig) {
    director_signal1 = 1;
    printf("Lekarz: Otrzymałem SIGUSR1. Kończę przyjmowanie nowych pacjentów.\n");
}

// Obsługa SIGUSR2 – natychmiastowa ewakuacja
void handle_sigusr2(int sig) {
    director_signal2 = 1;
    printf("Lekarz: Otrzymałem SIGUSR2. Natychmiast opuszczam budynek.\n");
    exit(0);
}

struct message {
    long msg_type;      // Typ wiadomości – decyduje, z której kolejki pobiera pacjent
    int patient_id;     // Identyfikator pacjenta
    char msg_text[100]; // Dodatkowy tekst, np. informacja o skierowaniu
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <role>\n", argv[0]);
        exit(1);
    }
    char *role = argv[1];
    int capacity = 0; // limit przyjęć dla danego lekarza
    int served = 0;   // liczba obsłużonych pacjentów

    if (strcmp(role, "POZ") == 0)
        capacity = 30; // przykładowy limit dla POZ
    else if (strcmp(role, "kardiolog") == 0 ||
             strcmp(role, "okulista") == 0 ||
             strcmp(role, "pediatra") == 0 ||
             strcmp(role, "lekarz_medycyny_pracy") == 0)
        capacity = 10; // przykładowy limit dla specjalistów
    else {
        fprintf(stderr, "Nieznana rola: %s\n", role);
        exit(1);
    }

    int msg_type = 0;
    if (strcmp(role, "POZ") == 0)
        msg_type = 2; // wspólna kolejka dla POZ
    else if (strcmp(role, "kardiolog") == 0)
        msg_type = 3;
    else if (strcmp(role, "okulista") == 0)
        msg_type = 4;
    else if (strcmp(role, "pediatra") == 0)
        msg_type = 5;
    else if (strcmp(role, "lekarz_medycyny_pracy") == 0)
        msg_type = 6;

    // Ustawienie obsługi sygnałów od Dyrektora
    signal(SIGUSR1, handle_sigusr1);
    signal(SIGUSR2, handle_sigusr2);

    key_t key = ftok("clinic", 65);
    int msg_queue = msgget(key, 0666);
    if (msg_queue == -1) {
        perror("Błąd przy otwieraniu kolejki komunikatów");
        exit(1);
    }

    srand(time(NULL) ^ getpid());
    struct message msg;

    FILE *report = fopen("raport.txt", "a");
    if (!report) {
        perror("Błąd otwarcia raport.txt");
    }

    while (served < capacity) {
        // Jeśli otrzymano SIGUSR1 – przechodzimy do trybu zamknięcia
        if (director_signal1) {
            printf("Lekarz %s (PID: %d): Przechodzę do trybu zamknięcia.\n", role, getpid());
            break;
        }

        // Odbieramy komunikat (blokująco)
        if (msgrcv(msg_queue, &msg, sizeof(msg) - sizeof(long), msg_type, 0) < 0) {
            perror("Błąd przy odbiorze komunikatu");
            break;
        }

        if (strcmp(role, "POZ") == 0) {
            printf("Lekarz POZ (PID: %d): Przyjąłem pacjenta %d. Wiadomość: %s\n",
                   getpid(), msg.patient_id, msg.msg_text);
            served++;

            // Z 20% szans próbujemy skierować pacjenta do specjalisty
            int referChance = rand() % 100;
            if (referChance < 20 && served < capacity) {
                // Sprawdzamy losowo dostępność terminów (np. 70% szans)
                int specialistFree = rand() % 100;
                if (specialistFree < 70) {
                    int specialistIndex = (rand() % 4) + 3; // losujemy typ specjalisty (3-6)
                    struct message ref_msg;
                    ref_msg.msg_type = specialistIndex;
                    ref_msg.patient_id = msg.patient_id;
                    strcpy(ref_msg.msg_text, "Skierowanie z POZ");
                    if (msgsnd(msg_queue, &ref_msg, sizeof(ref_msg) - sizeof(long), 0) < 0) {
                        perror("Błąd przy wysyłaniu skierowania");
                    } else {
                        char *specRole = (specialistIndex == 3) ? "kardiolog" :
                                          (specialistIndex == 4) ? "okulista" :
                                          (specialistIndex == 5) ? "pediatra" : "lekarz_medycyny_pracy";
                        printf("Lekarz POZ (PID: %d): Skierowałem pacjenta %d do %s.\n",
                               getpid(), msg.patient_id, specRole);
                    }
                } else {
                    printf("Lekarz POZ (PID: %d): Nie mogę skierować pacjenta %d - brak wolnych terminów u specjalisty.\n",
                           getpid(), msg.patient_id);
                    if (report)
                        fprintf(report, "Lekarz POZ (PID: %d): Pacjent %d - skierowanie nie przyjęte (brak terminów).\n",
                                getpid(), msg.patient_id);
                }
            }
        } else { // Specjalista
            printf("Specjalista %s (PID: %d): Przyjąłem pacjenta %d. Wiadomość: %s\n",
                   role, getpid(), msg.patient_id, msg.msg_text);
            served++;
            // Z 10% szans kierujemy pacjenta na badanie ambulatoryjne
            int examChance = rand() % 100;
            if (examChance < 10 && served < capacity) {
                printf("Specjalista %s (PID: %d): Kieruję pacjenta %d na badanie ambulatoryjne.\n",
                       role, getpid(), msg.patient_id);
                sleep(1); // symulacja badania
                printf("Specjalista %s (PID: %d): Pacjent %d wraca po badaniu ambulatoryjnym i jest obsługiwany bez kolejki.\n",
                       role, getpid(), msg.patient_id);
            }
        }
    }

    // Tryb zamknięcia – przetwarzamy wszystkie komunikaty, które jeszcze są w kolejce
    while (msgrcv(msg_queue, &msg, sizeof(msg) - sizeof(long), msg_type, IPC_NOWAIT) > 0) {
        if (strcmp(role, "POZ") == 0) {
            printf("Lekarz POZ (PID: %d) [CLOSING]: Przyjąłem pacjenta %d bez skierowania.\n",
                   getpid(), msg.patient_id);
            if (report)
                fprintf(report, "Lekarz POZ (PID: %d) [CLOSING]: Pacjent %d przyjęty bez skierowania.\n",
                        getpid(), msg.patient_id);
            served++;
        } else {
            printf("Specjalista %s (PID: %d) [CLOSING]: Przyjąłem pacjenta %d bez badania ambulatoryjnego.\n",
                   role, getpid(), msg.patient_id);
            if (report)
                fprintf(report, "Specjalista %s (PID: %d) [CLOSING]: Pacjent %d przyjęty bez badania ambulatoryjnego.\n",
                        role, getpid(), msg.patient_id);
            served++;
        }
    }

    if (report)
        fclose(report);

    printf("Lekarz %s (PID: %d): Zakończyłem przyjmowanie pacjentów (obsłużono %d pacjentów).\n", role, getpid(), served);
    return 0;
}
