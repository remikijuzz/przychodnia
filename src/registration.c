#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>

#define MAX_PATIENTS_INSIDE 10
#define THRESHOLD_OPEN (MAX_PATIENTS_INSIDE / 2)   // np. 5
#define THRESHOLD_CLOSE (MAX_PATIENTS_INSIDE / 3)    // np. 3

// Struktura komunikatu – pacjent wysyła komunikat rejestracyjny (msg_type = 1)
struct message {
    long msg_type;
    int patient_id;
    char msg_text[100];
    int specialist_type; // Dodane pole do przechowywania typu specjalisty (dla skierowań)
};

// Struktura węzła kolejki pacjentów
typedef struct PatientNode {
    int patient_id;
    char msg_text[100];
    struct PatientNode *next;
} PatientNode;

PatientNode* queue_head = NULL;
PatientNode* queue_tail = NULL;
int queue_length = 0;

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

int msg_queue;
volatile sig_atomic_t running = 1;

void sigterm_handler(int signum) {
    running = 0;
}

void enqueue(int patient_id, const char* msg_text) {
    PatientNode *node = malloc(sizeof(PatientNode));
    if (!node) {
        perror("Błąd alokacji pamięci");
        exit(1);
    }
    node->patient_id = patient_id;
    strncpy(node->msg_text, msg_text, sizeof(node->msg_text));
    node->next = NULL;
    if (queue_tail == NULL) {
        queue_head = queue_tail = node;
    } else {
        queue_tail->next = node;
        queue_tail = node;
    }
    queue_length++;
}

int dequeue(int *patient_id, char *msg_text) {
    if (queue_head == NULL)
        return 0;
    PatientNode *node = queue_head;
    *patient_id = node->patient_id;
    strncpy(msg_text, node->msg_text, 100);
    queue_head = node->next;
    if (queue_head == NULL)
        queue_tail = NULL;
    free(node);
    queue_length--;
    return 1;
}

void* reader_thread_func(void* arg) {
    struct message msg;
    while (running) {
        if (msgrcv(msg_queue, &msg, sizeof(msg) - sizeof(long), 1, IPC_NOWAIT) > 0) {
            pthread_mutex_lock(&queue_mutex);
            enqueue(msg.patient_id, msg.msg_text);
            pthread_cond_signal(&queue_cond);
            pthread_mutex_unlock(&queue_mutex);
        } else {
            usleep(100000);
        }
    }
    return NULL;
}

void* registration_window_func(void* arg) {
    int window_number = *(int*)arg;
    free(arg);
    while (running) {
        pthread_mutex_lock(&queue_mutex);
        while (queue_length == 0 && running) {
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }
        if (!running) {
            pthread_mutex_unlock(&queue_mutex);
            break;
        }
        struct message msg;
        int patient_id;
        char msg_text[100];
        if (dequeue(&patient_id, msg_text)) {
            pthread_mutex_unlock(&queue_mutex);
            printf("Okienko rejestracji %d: Rejestruję pacjenta %d. Wiadomość: %s\n",
                   window_number, patient_id, msg_text);
            sleep(1);

            int doctor_msg_queue;
            key_t doctor_key;
            int msg_type = 0; // Domyślna wartość, na wypadek błędu (choć nie powinna wystąpić)


            // Ustalenie msg_type na podstawie skierowania (msg.msg_type = 7) LUB wyboru pacjenta (msg.msg_type = 2-6)
            if (msg.msg_type == 7) {
                 msg_type = msg.specialist_type; // Typ specjalisty ze skierowania
            } else if (msg.msg_type >= 2 && msg.msg_type <= 6) {
                 msg_type = msg.msg_type; // Typ lekarza wybrany przez pacjenta przy rejestracji
            } else {
                //fprintf(stderr, "Nieznany typ komunikatu w rejestracji: %ld\n", msg.msg_type);
                continue; // Przejdź do następnego pacjenta w kolejce rejestracji
            }


            if (msg_type == 2 || msg_type == 8) { // POZ lub VIP POZ
                doctor_key = ftok("clinic_poz", 65);
            } else if (msg_type == 3 || msg_type == 9) { // kardiolog lub VIP kardiolog
                doctor_key = ftok("clinic_kardiolog", 65);
            } else if (msg_type == 4 || msg_type == 10) { // okulista lub VIP okulista
                doctor_key = ftok("clinic_okulista", 65);
            } else if (msg_type == 5 || msg_type == 11) { // pediatra lub VIP pediatra
                doctor_key = ftok("clinic_pediatra", 65);
            } else if (msg_type == 6 || msg_type == 12) { // lekarz medycyny pracy lub VIP medycyny pracy
                doctor_key = ftok("clinic_medycyny_pracy", 65);
            } else {
                fprintf(stderr, "Nieznany typ lekarza: %d\n", msg_type);
                continue; // Przejdź do następnego pacjenta w kolejce rejestracji
            }

            doctor_msg_queue = msgget(doctor_key, 0666);
            if (doctor_msg_queue == -1) {
                perror("Błąd przy otwieraniu kolejki lekarza");
                continue; // Przejdź do następnego pacjenta w kolejce rejestracji
            }

            struct message doctor_msg;
            doctor_msg.msg_type = msg_type; // Przekazujemy poprawny msg_type (2-6 lub 8-12)
            doctor_msg.patient_id = patient_id;
            strcpy(doctor_msg.msg_text, (msg.msg_type == 7) ? "Skierowanie z POZ" : "Pacjent zarejestrowany"); // Rozróżnienie komunikatu w zależności czy skierowanie czy rejestracja

            if (msgsnd(doctor_msg_queue, &doctor_msg, sizeof(doctor_msg) - sizeof(long), 0) < 0) {
                perror("Błąd przy wysyłaniu komunikatu do lekarza");
            } else {
                printf("Okienko rejestracji %d: Pacjent %d skierowany do lekarza (typ: %d).\n", window_number, patient_id, msg_type);
            }


        } else {
            pthread_mutex_unlock(&queue_mutex);
        }
    }
    printf("Okienko rejestracji %d: Zamykam.\n", window_number);
    return NULL;
}

void log_remaining_patients() {
    pthread_mutex_lock(&queue_mutex);
    if (queue_length > 0) {
        FILE *fp = fopen("report.txt", "a");
        if (fp) {
            fprintf(fp, "RAPORT ZAMKNIĘCIA REJESTRACJI:\n"); // Dodany nagłówek raportu
            PatientNode *current = queue_head;
            while (current) {
                fprintf(fp, "Pacjent %d nie został przyjęty (Przychodnia zamknięta - kolejka rejestracji). Wiadomość: %s\n", // Dodana przyczyna
                        current->patient_id, current->msg_text);
                current = current->next;
            }
            fclose(fp);
        } else {
            perror("Błąd przy otwieraniu report.txt");
        }
    }
    pthread_mutex_unlock(&queue_mutex);
}

int main() {
    signal(SIGTERM, sigterm_handler);
    
    key_t key = ftok("clinic", 65);
    msg_queue = msgget(key, 0666);
    if (msg_queue == -1) {
        perror("Błąd przy otwieraniu kolejki komunikatów w rejestracji");
        exit(1);
    }
    
    pthread_t reader_thread, window1_thread, window2_thread;
    int window2_active = 0;
    
    if (pthread_create(&reader_thread, NULL, reader_thread_func, NULL) != 0) {
        perror("Błąd przy tworzeniu wątku czytającego komunikaty");
        exit(1);
    }
    
    int *w1 = malloc(sizeof(int));
    if (!w1) {
        perror("Błąd alokacji pamięci dla okienka 1");
        exit(1);
    }
    *w1 = 1;
    if (pthread_create(&window1_thread, NULL, registration_window_func, w1) != 0) {
        perror("Błąd przy tworzeniu okienka rejestracji 1");
        exit(1);
    }
    
    while (running) {
        pthread_mutex_lock(&queue_mutex);
        int qlen = queue_length;
        pthread_mutex_unlock(&queue_mutex);
        
        if (!window2_active && qlen >= THRESHOLD_OPEN) {
            int *w2 = malloc(sizeof(int));
            if (!w2) {
                perror("Błąd alokacji pamięci dla okienka 2");
                exit(1);
            }
            *w2 = 2;
            if (pthread_create(&window2_thread, NULL, registration_window_func, w2) == 0) {
                window2_active = 1;
                printf("Monitor: Otwieram drugie okienko rejestracji.\n");
            } else {
                perror("Błąd przy tworzeniu okienka rejestracji 2");
                free(w2);
            }
        }
        if (window2_active && qlen < THRESHOLD_CLOSE) {
            if (pthread_cancel(window2_thread) == 0) {
                pthread_join(window2_thread, NULL);
                window2_active = 0;
                printf("Monitor: Zamykam drugie okienko rejestracji.\n");
            }
        }
        usleep(200000);
    }
    
    pthread_cancel(reader_thread);
    pthread_join(reader_thread, NULL);
    pthread_cancel(window1_thread);
    pthread_join(window1_thread, NULL);
    if (window2_active) {
        pthread_cancel(window2_thread);
        pthread_join(window2_thread, NULL);
    }
    
    log_remaining_patients();
    
    pthread_mutex_lock(&queue_mutex);
    while (queue_length > 0) {
        int dummy_id;
        char dummy_text[100];
        dequeue(&dummy_id, dummy_text);
    }
    pthread_mutex_unlock(&queue_mutex);
    
    printf("Rejestracja kończy działanie.\n");
    return 0;
}