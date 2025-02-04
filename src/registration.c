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

    while (1) {
        if (msgrcv(msg_queue, &msg, sizeof(msg) - sizeof(long), 1, IPC_NOWAIT) > 0) {
            printf("Rejestracja: Pacjent %d zarejestrowany.\n", msg.patient_id);
        }
        usleep(100000);  // Odpoczynek CPU
    }

    return 0;
}
