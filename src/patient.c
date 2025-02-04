#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h>

struct message {
    long msg_type;
    int patient_id;
    char msg_text[100];
};

int main() {
    key_t key = ftok("clinic", 65);
    int msg_queue = msgget(key, 0666);
    struct message msg;

    msg.msg_type = 1;
    msg.patient_id = getpid();
    snprintf(msg.msg_text, sizeof(msg.msg_text), "Pacjent %d wchodzi do rejestracji", msg.patient_id);
    msgsnd(msg_queue, &msg, sizeof(msg) - sizeof(long), 0);
    printf("Pacjent %d: Zarejestrowano.\n", msg.patient_id);
    
    sleep(1); // Dajemy czas na przetworzenie rejestracji

    msg.msg_type = 2;
    msgsnd(msg_queue, &msg, sizeof(msg) - sizeof(long), 0);
    
    return 0;
}
