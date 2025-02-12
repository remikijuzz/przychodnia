#include "clinic.h"
#include <pthread.h>

#define _POSIX_C_SOURCE 200809L
#define REG_MAX_QUEUE 10
#define THRESHOLD_OPEN (REG_MAX_QUEUE / 2)   // np. 5
#define THRESHOLD_CLOSE (REG_MAX_QUEUE / 3)    // np. 3


/* Węzeł kolejki pacjentów */
typedef struct PatientNode {
    int patient_id;
    char msg_text[100];
    int msg_type;
    int specialist_type;
    struct PatientNode *next;
} PatientNode;

static PatientNode* queue_head = NULL;
static PatientNode* queue_tail = NULL;
static int queue_length = 0;

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

int msg_queue;
volatile sig_atomic_t running = 1;

void sigterm_handler(int signum) {
    running = 0;
}

void enqueue(int patient_id, const char* msg_text, int msg_type, int specialist_type) {
    PatientNode *node = malloc(sizeof(PatientNode));
    if (!node) { perror("Błąd alokacji pamięci"); exit(1); }
    node->patient_id = patient_id;
    strncpy(node->msg_text, msg_text, sizeof(node->msg_text));
    node->msg_type = msg_type;
    node->specialist_type = specialist_type;
    node->next = NULL;
    if (!queue_tail) {
        queue_head = queue_tail = node;
    } else {
        queue_tail->next = node;
        queue_tail = node;
    }
    queue_length++;
}

int dequeue(int *patient_id, char *msg_text, int *msg_type, int *specialist_type) {
    if (!queue_head) return 0;
    PatientNode *node = queue_head;
    *patient_id = node->patient_id;
    strncpy(msg_text, node->msg_text, 100);
    *msg_type = node->msg_type;
    *specialist_type = node->specialist_type;
    queue_head = node->next;
    if (!queue_head) queue_tail = NULL;
    free(node);
    queue_length--;
    return 1;
}

void* reader_thread_func(void* arg) {
    struct message msg;
    while (running) {
        if (msgrcv(msg_queue, &msg, sizeof(msg) - sizeof(long), MSG_TYPE_REGISTRATION, IPC_NOWAIT) > 0) {
            pthread_mutex_lock(&queue_mutex);
            enqueue(msg.patient_id, msg.msg_text, msg.msg_type, msg.specialist_type);
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
        while (queue_length == 0 && running)
            pthread_cond_wait(&queue_cond, &queue_mutex);
        if (!running) {
            pthread_mutex_unlock(&queue_mutex);
            break;
        }
        int patient_id, msg_type, specialist_type;
        char msg_text[100];
        if (dequeue(&patient_id, msg_text, &msg_type, &specialist_type)) {
            pthread_mutex_unlock(&queue_mutex);
            printf("Okienko rejestracji %d: Rejestruję pacjenta %d – %s\n", window_number, patient_id, msg_text);
            sleep(1);
            
            int doctor_msg_queue;
            key_t doctor_key;
            int target_msg_type = 0;
            if (msg_type == MSG_TYPE_REFERRAL)
                target_msg_type = specialist_type;
            else if (msg_type >= MSG_TYPE_POZ && msg_type <= MSG_TYPE_MEDYCYNY_PRACY)
                target_msg_type = msg_type;
            else
                continue;
            
            if (target_msg_type == MSG_TYPE_POZ || target_msg_type == MSG_TYPE_VIP_POZ)
                doctor_key = ftok("clinic_poz", 65);
            else if (target_msg_type == MSG_TYPE_KARDIOLOG || target_msg_type == MSG_TYPE_VIP_KARDIOLOG)
                doctor_key = ftok("clinic_kardiolog", 65);
            else if (target_msg_type == MSG_TYPE_OKULISTA || target_msg_type == MSG_TYPE_VIP_OKULISTA)
                doctor_key = ftok("clinic_okulista", 65);
            else if (target_msg_type == MSG_TYPE_PEDIATRA || target_msg_type == MSG_TYPE_VIP_PEDIATRA)
                doctor_key = ftok("clinic_pediatra", 65);
            else if (target_msg_type == MSG_TYPE_MEDYCYNY_PRACY || target_msg_type == MSG_TYPE_VIP_MEDYCYNY_PRACY)
                doctor_key = ftok("clinic_medycyny_pracy", 65);
            else
                continue;
            
            doctor_msg_queue = msgget(doctor_key, 0666);
            if (doctor_msg_queue == -1) {
                perror("Błąd przy otwieraniu kolejki lekarza");
                continue;
            }
            
            struct message doctor_msg;
            doctor_msg.msg_type = target_msg_type;
            doctor_msg.patient_id = patient_id;
            if (msg_type == MSG_TYPE_REFERRAL)
                strcpy(doctor_msg.msg_text, "Skierowanie z POZ");
            else
                strcpy(doctor_msg.msg_text, "Pacjent zarejestrowany");
            
            if (msgsnd(doctor_msg_queue, &doctor_msg, sizeof(doctor_msg) - sizeof(long), 0) < 0)
                perror("Błąd przy wysyłaniu komunikatu do lekarza");
            else
                printf("Okienko %d: Pacjent %d skierowany do lekarza (typ: %d).\n", window_number, patient_id, target_msg_type);
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
            fprintf(fp, "RAPORT ZAMKNIĘCIA REJESTRACJI:\n");
            PatientNode *current = queue_head;
            while (current) {
                fprintf(fp, "Pacjent %d nie został przyjęty – %s\n", current->patient_id, current->msg_text);
                current = current->next;
            }
            fclose(fp);
        }
    }
    pthread_mutex_unlock(&queue_mutex);
}

int main(void) {
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
    
    while (queue_length > 0) {
        int dummy_id, dummy_type, dummy_spec;
        char dummy_text[100];
        dequeue(&dummy_id, dummy_text, &dummy_type, &dummy_spec);
    }
    
    printf("Rejestracja kończy działanie.\n");
    return 0;
}
