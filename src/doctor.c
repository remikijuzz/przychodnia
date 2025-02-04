// doctor.c
#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

// Definicja struktury komunikatu (używana zarówno przez pacjentów, jak i lekarzy)
struct message {
    long msg_type;      // Typ wiadomości – decyduje, która kolejka
    int patient_id;     // Identyfikator pacjenta
    char msg_text[100]; // Dodatkowy tekst (np. informacja o skierowaniu)
};

int main(int argc, char *argv[]) {
    
    char *role = argv[1];
    int capacity = 0; // maksymalna liczba pacjentów, których lekarz przyjmie
    int served = 0;   // licznik przyjętych pacjentów

    // Ustalamy pojemność w zależności od roli
    if (strcmp(role, "POZ") == 0) {
        capacity = 30; // przykładowo – każdy lekarz POZ przyjmuje 30 pacjentów
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

    // Ustalamy typ wiadomości, na której lekarz będzie odbierał pacjentów:
    int msg_type = 0;
    if (strcmp(role, "POZ") == 0) {
        msg_type = 2; // wspólna kolejka dla POZ
    } else if (strcmp(role, "kardiolog") == 0) {
        msg_type = 3;
    } else if (strcmp(role, "okulista") == 0) {
        msg_type = 4;
    } else if (strcmp(role, "pediatra") == 0) {
        msg_type = 5;
    } else if (strcmp(role, "lekarz_medycyny_pracy") == 0) {
        msg_type = 6;
    }

    // Otwarcie kolejki komunikatów (utworzonej przez main.c)
    key_t key = ftok("clinic", 65);
    int msg_queue = msgget(key, 0666);
    if (msg_queue == -1) {
        perror("Błąd przy otwieraniu kolejki komunikatów");
        exit(1);
    }

    srand(time(NULL) ^ getpid());

    struct message msg;

    // Główna pętla – lekarz przyjmuje pacjentów aż osiągnie swoją dzienną pojemność
    while (served < capacity) {
        // Odbieramy komunikat (blokująco – czekamy aż pojawi się pacjent)
        if (msgrcv(msg_queue, &msg, sizeof(msg) - sizeof(long), msg_type, 0) < 0) {
            perror("Błąd przy odbiorze komunikatu");
            break;
        }

        // Jeśli lekarz POZ
        if (strcmp(role, "POZ") == 0) {
            printf("Lekarz POZ (PID: %d): Przyjąłem pacjenta %d. Wiadomość: %s\n", getpid(), msg.patient_id, msg.msg_text);
            served++;

            // Jeśli to nie jest ostatni pacjent (symulujemy, że przed zamknięciem nie kierujemy na dodatkowe badania)
            int referChance = rand() % 100;
            if (referChance < 20 && served < capacity) {  // 20% szans na skierowanie
                // Wybieramy losowo jednego ze specjalistów – typy wiadomości: 3 (kardiolog), 4 (okulista), 5 (pediatra), 6 (lekarz_medycyny_pracy)
                int specialistIndex = (rand() % 4) + 3;  // losowa wartość z zakresu 3-6
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
                    printf("Lekarz POZ (PID: %d): Skierowałem pacjenta %d do %s.\n", getpid(), msg.patient_id, specRole);
                }
            }
        }
        // Jeśli lekarz jest specjalistą
        else {
            printf("Specjalista %s (PID: %d): Przyjąłem pacjenta %d. Wiadomość: %s\n", role, getpid(), msg.patient_id, msg.msg_text);
            served++;
            // Z prawdopodobieństwem 10% kierujemy pacjenta na badanie ambulatoryjne,
            // po czym symulujemy jego powrót i obsługę bez kolejki.
            int examChance = rand() % 100;
            if (examChance < 10 && served < capacity) {
                printf("Specjalista %s (PID: %d): Kieruję pacjenta %d na badanie ambulatoryjne.\n", role, getpid(), msg.patient_id);
                sleep(1); // symulacja czasu badania
                printf("Specjalista %s (PID: %d): Pacjent %d wraca po badaniu ambulatoryjnym i jest obsługiwany bez kolejki.\n", role, getpid(), msg.patient_id);
            }
        }
    }

    printf("Lekarz %s (PID: %d): Zakończyłem przyjmowanie pacjentów (obsłużono %d pacjentów).\n", role, getpid(), served);
    return 0;
}
