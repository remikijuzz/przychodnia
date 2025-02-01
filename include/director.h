#ifndef DIRECTOR_H
#define DIRECTOR_H

#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

// Funkcja wysyłająca sygnał 1 do wybranego lekarza
void send_signal_to_doctor(pid_t doctor_pid);

// Funkcja wysyłająca sygnał 2 do wszystkich pacjentów
void send_signal_to_all_patients(pid_t *patient_pids, int num_patients);

#endif // DIRECTOR_H