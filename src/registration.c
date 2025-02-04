// registration.c
#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>

// Definicje – przyjmujemy, że maksymalna liczba pacjentów wewnątrz budynku wynosi 10
#define MAX_PATIENTS_INSIDE 10
#define THRESHOLD_OPEN (MAX_PATIENTS_INSIDE / 2)   // np. przy 10 -> 5 pacjentów
#define THRESHOLD_CLOSE (MAX_PATIENTS_INSIDE / 3)    // np. przy 10 -> 3 pacjentów

// Struktura komunikatu – pacjent wysyła komunikat rejestracyjny z msg_type = 1
struct message {
    long msg_type;
    int patient_id;
    char msg_text[100];
};

// Struktura węzła kolejki pacjentów
typedef struct PatientNode {
    int patient_id;
    char msg_text[100];
    struct PatientNode *next;
} PatientNode;

// Globalna kolejka (FIFO) oraz liczba oczekujących
PatientNode* queue_head = NULL;
PatientNode* queue_tail = NULL;
int queue_length = 0;

// Mutex i zmienna warunkowa do synchronizacji dostępu do kolejki
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

// Identyfikator kolejki komunikatów (utworzonej przez main.c)
int msg_queue;

// Flaga sterująca działaniem procesu rejestracji (przy odebraniu SIGTERM)
volatile sig_atomic_t running = 1;

// Obsługa sygnału SIGTERM – umożliwia zakończenie pętli
void sigterm_handler(int signum) {
    running = 0;
}

// Funkcja dodająca pacjenta do kolejki
void enqueue(int patient_id, const char* msg_text) {
    PatientNode *node = malloc(sizeof(PatientNode));
    if (!node) {
        perror("Błąd alokacji pamięci");
        exit(1);
    }
    node->patient_id = patient_id;
    strncpy(node->msg_text, msg_text, sizeof(node->msg_text));
    node->next = NULL;
    if(queue_tail == NULL) {
        queue_head = queue_tail = node;
    } else {
        queue_tail->next = node;
        queue_tail = node;
    }
    queue_length++;
}

// Funkcja pobierająca pacjenta z kolejki (FIFO)
// Zwraca 1, jeśli uda się pobrać pacjenta, lub 0, gdy kolejka jest pusta.
int dequeue(int *patient_id, char *msg_text) {
    if(queue_head == NULL)
        return 0;
    PatientNode *node = queue_head;
    *patient_id = node->patient_id;
    strncpy(msg_text, node->msg_text, 100);
    queue_head = node->next;
    if(queue_head == NULL)
        queue_tail = NULL;
    free(node);
    queue_length--;
    return 1;
}

// Wątek czytający komunikaty rejestracyjne z kolejki komunikatów
void* reader_thread_func(void* arg) {
    struct message msg;
    while (running) {
        // Odbieramy komunikat typu 1 (rejestracja pacjenta)
        if (msgrcv(msg_queue, &msg, sizeof(msg) - sizeof(long), 1, IPC_NOWAIT) > 0) {
            pthread_mutex_lock(&queue_mutex);
            enqueue(msg.patient_id, msg.msg_text);
            // Budzimy wątki okien rejestracji – pojawił się nowy pacjent
            pthread_cond_signal(&queue_cond);
            pthread_mutex_unlock(&queue_mutex);
        } else {
            usleep(100000);  // jeśli brak komunikatu, czekamy 0.1 sekundy
        }
    }
    return NULL;
}

// Funkcja obsługująca jedno okienko rejestracji
// Argumentem jest numer okienka (1 lub 2)
void* registration_window_func(void* arg) {
    int window_number = *(int*)arg;
    free(arg); // już niepotrzebne
    while (running) {
        pthread_mutex_lock(&queue_mutex);
        // Czekamy, aż pojawi się pacjent w kolejce lub zakończenie działania
        while (queue_length == 0 && running) {
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }
        if (!running) {
            pthread_mutex_unlock(&queue_mutex);
            break;
        }
        int patient_id;
        char msg_text[100];
        if (dequeue(&patient_id, msg_text)) {
            pthread_mutex_unlock(&queue_mutex);
            // Symulujemy rejestrację pacjenta – np. zajmuje to 1 sekundę
            printf("Okienko rejestracji %d: Rejestruję pacjenta %d. Wiadomość: %s\n",
                   window_number, patient_id, msg_text);
            sleep(1);
        } else {
            pthread_mutex_unlock(&queue_mutex);
        }
    }
    printf("Okienko rejestracji %d: zamykam.\n", window_number);
    return NULL;
}

int main() {
    // Rejestracja obsługi SIGTERM
    signal(SIGTERM, sigterm_handler);

    // Uzyskujemy identyfikator kolejki komunikatów (utworzonej przez main.c)
    key_t key = ftok("clinic", 65);
    msg_queue = msgget(key, 0666);
    if (msg_queue == -1) {
        perror("Błąd przy otwieraniu kolejki komunikatów w rejestracji");
        exit(1);
    }

    pthread_t reader_thread, window1_thread, window2_thread;
    int window2_active = 0;  // flaga, czy drugie okienko jest aktywne

    // Tworzymy wątek czytający komunikaty rejestracyjne z kolejki komunikatów
    if (pthread_create(&reader_thread, NULL, reader_thread_func, NULL) != 0) {
        perror("Błąd przy tworzeniu wątku czytającego kolejkę komunikatów");
        exit(1);
    }

    // Tworzymy zawsze aktywne pierwsze okienko rejestracji
    int *w1 = malloc(sizeof(int));
    if (w1 == NULL) {
        perror("Błąd alokacji pamięci dla okienka 1");
        exit(1);
    }
    *w1 = 1;
    if (pthread_create(&window1_thread, NULL, registration_window_func, w1) != 0) {
        perror("Błąd przy tworzeniu okienka rejestracji 1");
        exit(1);
    }

    // Monitor – główna pętla kontroluje liczbę pacjentów oczekujących i otwiera lub zamyka drugie okienko
    while (running) {
        pthread_mutex_lock(&queue_mutex);
        int qlen = queue_length;
        pthread_mutex_unlock(&queue_mutex);

        // Jeśli w kolejce jest więcej niż THRESHOLD_OPEN pacjentów i drugie okienko nie działa...
        if (!window2_active && qlen >= THRESHOLD_OPEN) {
            int *w2 = malloc(sizeof(int));
            if (w2 == NULL) {
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
        // Jeśli drugie okienko działa, a w kolejce jest mniej niż THRESHOLD_CLOSE pacjentów...
        if (window2_active && qlen < THRESHOLD_CLOSE) {
            if (pthread_cancel(window2_thread) == 0) {
                pthread_join(window2_thread, NULL);
                window2_active = 0;
                printf("Monitor: Zamykam drugie okienko rejestracji.\n");
            }
        }
        usleep(200000);  // monitorujemy co 0.2 sekundy
    }

    // Zakończenie – anulujemy pozostałe wątki i czyścimy kolejkę
    pthread_cancel(reader_thread);
    pthread_join(reader_thread, NULL);
    pthread_cancel(window1_thread);
    pthread_join(window1_thread, NULL);
    if (window2_active) {
        pthread_cancel(window2_thread);
        pthread_join(window2_thread, NULL);
    }

    // Czyszczenie kolejki (zwalniamy pozostałe elementy)
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
