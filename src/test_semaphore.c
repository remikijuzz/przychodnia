#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

/*
  Test jednostkowy dla semaforów:
  - Tworzy semafor o początkowej wartości (np. 5)
  - Sprawdza, czy sem_getvalue zwraca poprawną wartość
  - Wywołuje sem_wait() kolejno, aż semafor zostanie wyczerpany
  - Używa sem_trywait(), aby upewnić się, że gdy semafor jest wyczerpany, zwraca EAGAIN
  - Następnie wywołuje sem_post() tyle razy, aby przywrócić wartość semafora
*/

int main(void) {
    const char *sem_name = "/test_sem";
    int init_val = 5; // Początkowa wartość semafora
    sem_t *sem = sem_open(sem_name, O_CREAT | O_EXCL, 0666, init_val);
    if (sem == SEM_FAILED) {
         perror("sem_open failed");
         exit(1);
    }

    int sval = 0;
    if (sem_getvalue(sem, &sval) == -1) {
         perror("sem_getvalue failed");
         exit(1);
    }
    printf("Initial semaphore value: %d\n", sval);
    assert(sval == init_val);

    // Wywołujemy sem_wait() init_val razy
    for (int i = 0; i < init_val; i++) {
         if (sem_wait(sem) == -1) {
              perror("sem_wait failed");
              exit(1);
         }
         if (sem_getvalue(sem, &sval) == -1) {
              perror("sem_getvalue failed");
              exit(1);
         }
         printf("Semaphore value after sem_wait #%d: %d\n", i+1, sval);
         assert(sval == init_val - (i+1));
    }

    // Po wyczerpaniu semafora, sem_trywait() powinien zwrócić -1 i ustawić errno na EAGAIN
    if (sem_trywait(sem) != -1) {
         fprintf(stderr, "sem_trywait succeeded when it should block\n");
         exit(1);
    } else {
         if (errno == EAGAIN) {
              printf("sem_trywait correctly returned EAGAIN when semaphore is exhausted\n");
         } else {
              perror("sem_trywait failed with unexpected error");
              exit(1);
         }
    }

    // Wywołujemy sem_post() init_val razy, aby przywrócić początkową wartość
    for (int i = 0; i < init_val; i++) {
         if (sem_post(sem) == -1) {
              perror("sem_post failed");
              exit(1);
         }
         if (sem_getvalue(sem, &sval) == -1) {
              perror("sem_getvalue failed");
              exit(1);
         }
         printf("Semaphore value after sem_post #%d: %d\n", i+1, sval);
         assert(sval == i+1);
    }

    // Sprzątamy: zamykamy i usuwamy semafor
    if (sem_close(sem) == -1) {
         perror("sem_close failed");
         exit(1);
    }
    if (sem_unlink(sem_name) == -1) {
         perror("sem_unlink failed");
         exit(1);
    }
    printf("Test semaphore passed.\n");
    return 0;
}
