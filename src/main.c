#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include "config.h"

#define PATH "./src/"
#define EVACUATION_TIME 10  // Czas działania przed ewakuacją
#define SIMULATION_TIME 5   // Czas między zmianami godzin w symulacji

pid_t registration_pid, patient_pid, doctor_pid;
int current_hour = CLINIC_OPEN_HOUR;

void update_time() {
    current_hour++;
    printf("\nGodzina %02d:00\n", current_hour);
}

void wait_for_processes() {
    int status;
    if (registration_pid > 0) waitpid(registration_pid, &status, 0);
    if (patient_pid > 0) waitpid(patient_pid, &status, 0);
    if (doctor_pid > 0) waitpid(doctor_pid, &status, 0);
}

int main(int argc, char *argv[]) {
    printf("Przychodnia otwarta od %d:00 do %d:00\n", CLINIC_OPEN_HOUR, CLINIC_CLOSE_HOUR);

    // Tworzenie procesu rejestracji
    registration_pid = fork();
    if (registration_pid == 0) {
        execl(PATH "registration", "registration", NULL);
        perror("Błąd uruchamiania procesu rejestracji");
        exit(EXIT_FAILURE);
    }

    // Tworzenie procesu lekarzy
    doctor_pid = fork();
    if (doctor_pid == 0) {
        execl(PATH "doctor", "doctor", NULL);
        perror("Błąd uruchamiania procesu lekarzy");
        exit(EXIT_FAILURE);
    }

    // Tworzenie procesu generatora pacjentów
    patient_pid = fork();
    if (patient_pid == 0) {
        execl(PATH "patient", "patient", NULL);
        perror("Błąd uruchamiania generatora pacjentów");
        exit(EXIT_FAILURE);
    }

    // **Tryb ewakuacji (-d)**
    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        while (current_hour < EVACUATION_TIME) {
            sleep(SIMULATION_TIME);
            update_time();
        }
        printf("Dyrektor: Zarządzam ewakuację!\n");
        kill(registration_pid, SIGUSR2);
        kill(patient_pid, SIGUSR2);
        kill(doctor_pid, SIGUSR2);
        wait_for_processes();
        printf("Dyrektor: Wszyscy opuścili budynek.\n");
        exit(0);
    }

    // **Tryb normalny**
    printf("Rozpoczynamy pracę placówki o godzinie %02d:00\n", CLINIC_OPEN_HOUR);

    while (current_hour < CLINIC_CLOSE_HOUR) {
        sleep(SIMULATION_TIME);
        update_time();
    }

    // **Zamknięcie przychodni**
    printf("Dyrektor: Jest godzina %02d:00 – zamykamy przychodnię.\n", CLINIC_CLOSE_HOUR);
    kill(registration_pid, SIGUSR1);
    kill(patient_pid, SIGUSR1);
    kill(doctor_pid, SIGUSR1);

    wait_for_processes();

    printf("Dyrektor: Przychodnia została zamknięta – wszyscy pacjenci i lekarze opuścili budynek.\n");
    return 0;
}
