#define _POSIX_C_SOURCE 200809L
#include "clinic.h"   // Upewnij się, że clinic.h zawiera m.in. definicje MSG_TYPE_REGISTRATION
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define REG_MAX_QUEUE 10
#define THRESHOLD_OPEN (REG_MAX_QUEUE / 2)   // przykładowo: 5, jeśli REG_MAX_QUEUE=10
#define THRESHOLD_CLOSE (REG_MAX_QUEUE / 3)    // przykładowo: 3, jeśli REG_MAX_QUEUE=10

/* Definicja struktury węzła kolejki pacjentów */
typedef struct PatientNode {
    int patient_id;
    char msg_text[100];
    int msg_type;
    int specialist_type;
    struct PatientNode *next;
} PatientNode;

/* Statyczne zmienne kolejki */
static PatientNode* queue_head = NULL;
static PatientNode* queue_tail = NULL;
static int queue_length = 0;

/* Mutex i zmienna warunkowa do synchronizacji kolejki */
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

/* Funkcja dodająca element do kolejki */
void enqueue(int patient_id, const char* msg_text, int msg_type, int specialist_type) {
    PatientNode *node = malloc(sizeof(PatientNode));
    if (!node) { 
        perror("Błąd alokacji pamięci");
        exit(1);
    }
    node->patient_id = patient_id;
    strncpy(node->msg_text, msg_text, sizeof(node->msg_text));
    node->msg_text[sizeof(node->msg_text)-1] = '\0'; // zapewnienie null termination
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

/* Funkcja usuwająca element z kolejki */
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

/* Funkcja pomocnicza: reset kolejki */
void reset_queue() {
    while(queue_length > 0) {
        int dummy_id, dummy_type, dummy_spec;
        char dummy_text[100];
        dequeue(&dummy_id, dummy_text, &dummy_type, &dummy_spec);
    }
}

/* Test monitorowania warunków otwierania/zamykania drugiego okienka */
void test_monitor_conditions() {
    // Na początku kolejka powinna być pusta.
    reset_queue();
    assert(queue_length == 0);
    
    // Test 1: Dodajemy elementy, aby kolejka osiągnęła próg otwarcia.
    // THRESHOLD_OPEN = REG_MAX_QUEUE/2, np. 5 przy REG_MAX_QUEUE=10.
    int num_to_add = THRESHOLD_OPEN; 
    for (int i = 1; i <= num_to_add; i++) {
        enqueue(i, "Test Patient", MSG_TYPE_REGISTRATION, 0);
    }
    // Sprawdzamy, czy liczba elementów równa się num_to_add.
    assert(queue_length == num_to_add);
    printf("Monitor test: Po enqueue liczba elementów = %d (THRESHOLD_OPEN = %d)\n", queue_length, THRESHOLD_OPEN);
    
    // Test 2: Usuńmy pewną liczbę elementów, aby kolejka spadła poniżej THRESHOLD_CLOSE.
    // THRESHOLD_CLOSE = REG_MAX_QUEUE/3, np. 3 przy REG_MAX_QUEUE=10.
    // Aby kolejka spadła poniżej 3, musimy usunąć (num_to_add - THRESHOLD_CLOSE + 1) elementów.
    int num_to_remove = num_to_add - THRESHOLD_CLOSE + 1;
    for (int i = 0; i < num_to_remove; i++) {
        int pid, mtype, spec;
        char mtext[100];
        int ret = dequeue(&pid, mtext, &mtype, &spec);
        assert(ret == 1);
    }
    printf("Monitor test: Po dequeue liczba elementów = %d (THRESHOLD_CLOSE = %d)\n", queue_length, THRESHOLD_CLOSE);
    
    // Jeśli kolejka ma mniej niż THRESHOLD_CLOSE, monitor powinien zamknąć drugie okienko.
    // W naszym teście sprawdzamy tylko stan liczby elementów.
    assert(queue_length < THRESHOLD_CLOSE);
    
    printf("Test monitor conditions passed.\n");
}

#ifdef UNIT_TEST
int main(void) {
    test_monitor_conditions();
    return 0;
}
#else
int main(void) {
    return 0;
}
#endif
