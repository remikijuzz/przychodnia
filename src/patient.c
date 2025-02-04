#include "structs.h"
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define REGISTRATION_QUEUE_KEY 1234

int main() {
    Patient p = {rand() % 1000, rand() % 90, rand() % 5 == 0, rand() % 6, false, -1};

    int msgid = msgget(REGISTRATION_QUEUE_KEY, 0666);
    if (msgid == -1) {
        perror("Błąd uzyskania dostępu do kolejki rejestracji");
        exit(1);
    }

    Message msg = {1, p};
    if (msgsnd(msgid, &msg, sizeof(Patient), 0) == -1) {
        perror("Błąd wysyłania do rejestracji");
        exit(1);
    }

    printf("Pacjent %d zgłosił się do rejestracji.\n", p.id);
    return 0;
}
