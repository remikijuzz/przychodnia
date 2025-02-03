#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define NUM_DOCTORS 6

<<<<<<< HEAD
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
=======
pid_t doctor_pids[NUM_DOCTORS];
pid_t registration_pid;

void spawn_doctor(int id) {
    doctor_pids[id] = fork();
    if (doctor_pids[id] == 0) {
        char buffer[10];
        sprintf(buffer, "%d", id);
        execl("./src/doctor", "doctor", buffer, NULL);
    }
>>>>>>> a8f733d (Rebuild projektu zgodnie z konsultacjami)
}

void spawn_registration() {
    registration_pid = fork();
    if (registration_pid == 0) {
        execl("./src/registration", "registration", NULL);
    }
}

<<<<<<< HEAD
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
=======
void spawn_patients() {
    for (int i = 0; i < 10; i++) {
        if (fork() == 0) {
            execl("./src/patient", "patient", NULL);
        }
        sleep(1);
    }
}

int main() {
    printf("Przychodnia otwarta\n");

    spawn_registration();
    for (int i = 0; i < NUM_DOCTORS; i++) {
        spawn_doctor(i);
    }

    spawn_patients();

    for (int i = 0; i < NUM_DOCTORS; i++) {
        waitpid(doctor_pids[i], NULL, 0);
    }
    waitpid(registration_pid, NULL, 0);

    printf("Przychodnia zamknięta\n");
>>>>>>> a8f733d (Rebuild projektu zgodnie z konsultacjami)
    return 0;
}
