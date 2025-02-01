#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include "config.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Błąd: Brak ID lekarza\n");
        exit(EXIT_FAILURE);
    }

    int doctor_id = atoi(argv[1]);
    printf("Lekarz %d uruchomiony\n", doctor_id);

    while (1) {
        sleep(2);
        printf("Lekarz %d: Przyjmuję pacjenta\n", doctor_id);
    }

    return 0;
}
