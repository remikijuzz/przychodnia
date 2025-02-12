#define _POSIX_C_SOURCE 200809L
#include "clinic.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

int main(void) {
    // Tworzymy unikalny klucz – użyjemy pliku "ipc_test" (możesz go utworzyć lub wskazać inny istniejący plik)
    key_t key = ftok("ipc_test", 65);
    if (key == -1) {
        perror("ftok failed");
        exit(EXIT_FAILURE);
    }

    // Tworzymy kolejkę komunikatów
    int msgqid = msgget(key, IPC_CREAT | 0666);
    if (msgqid == -1) {
        perror("msgget failed");
        exit(EXIT_FAILURE);
    }

    // Przygotowujemy komunikat do wysłania
    struct message msg;
    msg.msg_type = 1;  // Ustalony typ komunikatu (np. 1)
    msg.patient_id = 123;  // Przykładowy identyfikator pacjenta
    strncpy(msg.msg_text, "Hello, IPC!", sizeof(msg.msg_text));
    msg.msg_text[sizeof(msg.msg_text) - 1] = '\0';  // Zapewniamy null-termination
    msg.specialist_type = 0;

    // Wysyłamy komunikat
    if (msgsnd(msgqid, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
        perror("msgsnd failed");
        msgctl(msgqid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }
    printf("Message sent: patient_id=%d, msg_text='%s'\n", msg.patient_id, msg.msg_text);

    // Odbieramy komunikat – filtrujemy według typu 1
    struct message recv_msg;
    if (msgrcv(msgqid, &recv_msg, sizeof(recv_msg) - sizeof(long), 1, 0) == -1) {
        perror("msgrcv failed");
        msgctl(msgqid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }
    printf("Message received: patient_id=%d, msg_text='%s'\n", recv_msg.patient_id, recv_msg.msg_text);

    // Sprawdzamy, czy odebrany komunikat jest taki sam jak wysłany
    assert(recv_msg.patient_id == msg.patient_id);
    assert(strcmp(recv_msg.msg_text, msg.msg_text) == 0);

    // Usuwamy kolejkę komunikatów
    if (msgctl(msgqid, IPC_RMID, NULL) == -1) {
        perror("msgctl (IPC_RMID) failed");
        exit(EXIT_FAILURE);
    }

    printf("Test IPC (msgsnd/msgrcv) passed.\n");
    return 0;
}
