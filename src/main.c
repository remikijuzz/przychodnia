#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define NUM_DOCTORS 6

pid_t doctor_pids[NUM_DOCTORS];
pid_t registration_pid;

void spawn_doctor(int id) {
    doctor_pids[id] = fork();
    if (doctor_pids[id] == 0) {
        execl("./src/doctor", "doctor", NULL);
    }
}

void spawn_registration() {
    registration_pid = fork();
    if (registration_pid == 0) {
        execl("./src/registration", "registration", NULL);
    }
}

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
    return 0;
}
