// test_queue.c
#define _POSIX_C_SOURCE 200809L

#include "clinic.h"   // Upewnij się, że ten plik zawiera niezbędne definicje typów (np. MSG_TYPE_REGISTRATION, itp.)
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Testowy moduł kolejki rejestracji

#define REG_MAX_QUEUE 10
#define THRESHOLD_OPEN (REG_MAX_QUEUE / 2)
#define THRESHOLD_CLOSE (REG_MAX_QUEUE / 3)

/* Struktura węzła kolejki pacjentów */
typedef struct PatientNode {
    int patient_id;
    char msg_text[100];
    int msg_type;
    int specialist_type;
    struct PatientNode *next;
} PatientNode;

/* Globalne zmienne kolejki – deklarowane jako static, aby nie były widoczne poza tym plikiem */
static PatientNode* queue_head = NULL;
static PatientNode* queue_tail = NULL;
static int queue_length = 0;

/* Mutex i zmienna warunkowa dla kolejki */
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

/* Funkcja enqueue – dodaje element do kolejki */
void enqueue(int patient_id, const char* msg_text, int msg_type, int specialist_type) {
    PatientNode *node = malloc(sizeof(PatientNode));
    if (!node) { 
        perror("Błąd alokacji pamięci");
        exit(1);
    }
    node->patient_id = patient_id;
    strncpy(node->msg_text, msg_text, sizeof(node->msg_text));
    node->msg_text[sizeof(node->msg_text) - 1] = '\0'; // zapewnij null termination
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

/* Funkcja dequeue – usuwa element z kolejki */
int dequeue(int *patient_id, char *msg_text, int *msg_type, int *specialist_type) {
    if (!queue_head) return 0;
    PatientNode *node = queue_head;
    *patient_id = node->patient_id;
    strncpy(msg_text, node->msg_text, 100);
    msg_text[99] = '\0';
    *msg_type = node->msg_type;
    *specialist_type = node->specialist_type;
    queue_head = node->next;
    if (!queue_head) {
        queue_tail = NULL;
    }
    free(node);
    queue_length--;
    return 1;
}

/* Test funkcji enqueue/dequeue */
void test_enqueue_dequeue() {
    // Upewnij się, że kolejka jest pusta
    while(queue_length > 0) {
        int pid, type, spec;
        char text[100];
        dequeue(&pid, text, &type, &spec);
    }
    assert(queue_length == 0);

    // Dodajemy trzy elementy
    enqueue(1, "Test Patient 1", MSG_TYPE_REGISTRATION, 0);
    enqueue(2, "Test Patient 2", MSG_TYPE_REGISTRATION, 0);
    enqueue(3, "Test Patient 3", MSG_TYPE_REGISTRATION, 0);
    
    // Sprawdzamy, czy kolejka ma 3 elementy
    assert(queue_length == 3);

    int pid, msg_type, spec;
    char msg_text[100];
    int ret;

    ret = dequeue(&pid, msg_text, &msg_type, &spec);
    assert(ret == 1);
    assert(pid == 1);
    assert(strcmp(msg_text, "Test Patient 1") == 0);
    assert(queue_length == 2);

    ret = dequeue(&pid, msg_text, &msg_type, &spec);
    assert(ret == 1);
    assert(pid == 2);
    assert(queue_length == 1);

    ret = dequeue(&pid, msg_text, &msg_type, &spec);
    assert(ret == 1);
    assert(pid == 3);
    assert(queue_length == 0);

    // Próba usunięcia z pustej kolejki – powinna zwrócić 0
    ret = dequeue(&pid, msg_text, &msg_type, &spec);
    assert(ret == 0);

    printf("Test enqueue/dequeue passed.\n");
}

int main(void) {
    test_enqueue_dequeue();
    return 0;
}
