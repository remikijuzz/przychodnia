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

pid_t director_pid, registration_pid, patient_pid;
pid_t doctor_pids[NUM_DOCTORS];
int current_hour = CLINIC_OPEN_HOUR;

void update_time() {
    current_hour++;
    printf("\nGodzina %02d:00\n", current_hour);
}

void wait_for_processes() {
    int status;

    if (registration_pid > 0) waitpid(registration_pid, &status, 0);
    if (patient_pid > 0) waitpid(patient_pid, &status, 0);
    if (director_pid > 0) waitpid(director_pid, &status, 0);

    for (int i = 0; i < NUM_DOCTORS; i++) {
        if (doctor_pids[i] > 0) waitpid(doctor_pids[i], &status, 0);
    }
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

    // Tworzenie procesów lekarzy
    for (int i = 0; i < NUM_DOCTORS; i++) {
        doctor_pids[i] = fork();
        if (doctor_pids[i] == 0) {
            char doctor_id_str[10];
            sprintf(doctor_id_str, "%d", i);
            execl(PATH "doctor", "doctor", doctor_id_str, NULL);
            perror("Błąd uruchamiania procesu lekarza");
            exit(EXIT_FAILURE);
        }
    }

    // Tworzenie procesu generatora pacjentów
    patient_pid = fork();
    if (patient_pid == 0) {
        execl(PATH "patient", "patient", NULL);
        perror("Błąd uruchamiania generatora pacjentów");
        exit(EXIT_FAILURE);
    }

    // Tworzenie argumentów dla director
    char reg_pid_str[10], pat_pid_str[10], doctor_pids_str[NUM_DOCTORS][10];
    sprintf(reg_pid_str, "%d", registration_pid);
    sprintf(pat_pid_str, "%d", patient_pid);
    
    char *args[NUM_DOCTORS + 4];  
    args[0] = "director";
    args[1] = reg_pid_str;
    args[2] = pat_pid_str;

    for (int i = 0; i < NUM_DOCTORS; i++) {
        sprintf(doctor_pids_str[i], "%d", doctor_pids[i]);
        args[i + 3] = doctor_pids_str[i];
    }

    args[NUM_DOCTORS + 3] = NULL;  

    // Tworzenie procesu dyrektora z przekazaniem PID-ów
    director_pid = fork();
    if (director_pid == 0) {
        execv(PATH "director", args);
        perror("Błąd uruchamiania procesu dyrektora");
        exit(EXIT_FAILURE);
    }

    // Tryb ewakuacji (-d)
    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        
        while (current_hour < EVACUATION_TIME) {
        sleep(SIMULATION_TIME);
        update_time();
    }
        kill(director_pid, SIGUSR2);
        wait_for_processes();
        exit(0);
    }

    // Tryb normalny (przychodnia działa do 20:00)
    printf("Rozpoczynamy pracę placówki o godzinie %02d:00\n", CLINIC_OPEN_HOUR);

    while (current_hour < CLINIC_CLOSE_HOUR) {
        sleep(SIMULATION_TIME);
        update_time();
    }

    // Normalne zamknięcie
    kill(director_pid, SIGUSR1);
    wait_for_processes();

    printf("Przychodnia zamknięta.\n");
    return 0;
}
