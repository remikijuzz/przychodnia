#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>

/* Jeśli nie masz w clinic.h zdefiniowanych, to definiujemy makro i strukturę */
#ifndef MSG_TYPE_REGISTRATION
#define MSG_TYPE_REGISTRATION 1
#endif

struct message {
    long msg_type;
    int patient_id;
    char msg_text[100];
    int specialist_type;  // nieużywane w teście
};

#define LOAD_NUM 10      // Liczba procesów dzieci (komunikatów) na iterację
#define ITERATIONS 5     // Liczba iteracji testu
#define TIMEOUT_SECONDS 5 // Maksymalny czas oczekiwania na odebranie wszystkich komunikatów

int main(void) {
    for (int iter = 0; iter < ITERATIONS; iter++) {
        printf("Iteracja %d\n", iter + 1);
        /* Używamy unikalnego klucza dla każdej iteracji, np. 100, 200, ... */
        key_t key = 100 * (iter + 1);
        int msgqid = msgget(key, IPC_CREAT | 0666);
        if (msgqid == -1) {
            perror("msgget failed");
            exit(EXIT_FAILURE);
        }
        
        printf("Test obciążeniowy: wysyłamy %d komunikatów.\n", LOAD_NUM);
        
        /* Tworzymy LOAD_NUM procesów dzieci – każde dziecko wysyła komunikat i kończy działanie */
        for (int i = 0; i < LOAD_NUM; i++) {
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork failed");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {
                struct message msg;
                msg.msg_type = MSG_TYPE_REGISTRATION;
                msg.patient_id = getpid();
                snprintf(msg.msg_text, sizeof(msg.msg_text), "Testowy komunikat od pacjenta %d", getpid());
                msg.specialist_type = 0;
                if (msgsnd(msgqid, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
                    perror("msgsnd failed");
                    exit(EXIT_FAILURE);
                }
                printf("Dziecko %d: wysłano komunikat.\n", getpid());
                exit(EXIT_SUCCESS);
            }
        }
        
        /* Rodzic czeka na zakończenie wszystkich procesów dzieci */
        for (int i = 0; i < LOAD_NUM; i++) {
            wait(NULL);
        }
        printf("Wszystkie procesy dzieci zakończyły działanie. Odbieram komunikaty...\n");
        
        /* Odbieramy wszystkie komunikaty – wykorzystujemy pętlę z timeoutem */
        int received = 0;
        struct message recv_msg;
        time_t start = time(NULL);
        while (received < LOAD_NUM) {
            printf("Oczekiwanie na komunikat %d z %d...\n", received + 1, LOAD_NUM);
            fflush(stdout);
            ssize_t ret = msgrcv(msgqid, &recv_msg, sizeof(recv_msg) - sizeof(long), MSG_TYPE_REGISTRATION, IPC_NOWAIT);
            if (ret == -1) {
                if (errno == ENOMSG) {
                    if (difftime(time(NULL), start) >= TIMEOUT_SECONDS) {
                        printf("Timeout: nie odebrano wszystkich komunikatów.\n");
                        break;
                    }
                    usleep(100000); // czekamy 100 ms
                    continue;
                }
                perror("msgrcv failed");
                break;
            }
            received++;
            printf("Odebrano komunikat: pacjent_id=%d, tekst='%s'\n", recv_msg.patient_id, recv_msg.msg_text);
        }
        
        printf("Iteracja %d: odebrano %d komunikatów.\n", iter + 1, received);
        
        /* Usuwamy kolejkę komunikatów */
        if (msgctl(msgqid, IPC_RMID, NULL) == -1) {
            perror("msgctl (IPC_RMID) failed");
            exit(EXIT_FAILURE);
        }
        printf("Iteracja %d: kolejka komunikatów usunięta.\n\n", iter + 1);
    }
    
    printf("Test obciążeniowy zakończony.\n");
    return 0;
}
