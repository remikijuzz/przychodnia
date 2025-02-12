## 1. Założenia projektowe kodu

Projekt symuluje działanie przychodni lekarskiej, składającej się z rejestracji, lekarzy różnych specjalności (POZ, kardiolog, okulista, pediatra, lekarz medycyny pracy), dyrektora oraz pacjentów.  Aplikacja oparta jest o wieloprocesowość, wykorzystując mechanizmy komunikacji międzyprocesowej (kolejki komunikatów, semafory) oraz synchronizacji (semafory, sygnały).  Celem projektu jest zademonstrowanie umiejętności tworzenia aplikacji wieloprocesowych w systemie Linux, wykorzystujących różne mechanizmy systemowe.

## 2. Ogólny opis kodu

Projekt składa się z następujących programów:

*   **`main` (przychodnia):** Główny program symulacji, tworzy kolejki komunikatów, semafor ograniczający liczbę pacjentów w przychodni, wątek symulacji czasu i wątek obsługi sygnału SIGUSR2 (ewakuacja). Tworzy proces rejestracji, procesy lekarzy i procesy pacjentów.  Zamyka przychodnię po upływie czasu symulacji, wysyłając sygnały SIGTERM do lekarzy i rejestracji.
*   **`registration` (rejestracja):** Program imitujący rejestrację przychodni. Posiada dwa okienka rejestracji (wątki). Odbiera pacjentów (komunikaty z kolejek) i kieruje ich do odpowiednich lekarzy (wysyłając komunikaty do kolejek lekarzy). Obsługuje pacjentów VIP i zwykłych.
*   **`doctor` (lekarz):** Program imitujący lekarza danej specjalności (rola przekazywana jako argument).  Odbiera pacjentów z dedykowanej kolejki, symuluje wizytę lekarską.  Lekarze POZ mogą kierować pacjentów do specjalistów (poprzez rejestrację). Specjaliści mogą kierować pacjentów na badania ambulatoryjne (uproszczone). Lekarze reagują na sygnały SIGUSR1 (polecenie zakończenia pracy po obsłużeniu pacjentów) od dyrektora i SIGTERM (zamknięcie przychodni) od programu głównego. Po otrzymaniu sygnału lekarze kończą przyjmowanie nowych pacjentów, ale obsługują jeszcze pacjentów z kolejki. Generują raport po zakończeniu pracy.
*   **`patient` (pacjent):** Program imitujący pacjenta. Pacjenci losowo przybywają do przychodni, rejestrują się (wysyłając komunikat do rejestracji), a następnie udają się do lekarza (POZ lub specjalisty, w zależności od skierowania lub braku). Po wizycie u lekarza pacjent opuszcza przychodnię. Pacjenci VIP mają priorytet w rejestracji i u lekarzy.
*   **`director` (dyrektor):** Program umożliwiający interakcję z symulacją.  Aktualnie dyrektor może wysłać sygnał SIGUSR1 do wszystkich lekarzy, powodując ich "ewakuację" (grzeczne zakończenie pracy po obsłużeniu pacjentów). W przyszłości można rozszerzyć funkcjonalność dyrektora.

## 3. Co udało się zrobić

*   Udało się zaimplementować **kompletną symulację przychodni**, obejmującą rejestrację, lekarzy różnych specjalności i pacjentów.
*   Zaimplementowano **wieloprocesowość** z wykorzystaniem funkcji `fork()` i `exec()`.
*   Zastosowano **kolejki komunikatów** (`msgget`, `msgsnd`, `msgrcv`, `msgctl`) do komunikacji między procesami:
    *   Pacjenci -> Rejestracja
    *   Rejestracja -> Lekarze (różnych specjalności)
    *   Lekarze POZ -> Rejestracja (skierowania)
*   Użyto **semaforów** (`semget`, `semop`, `semctl`) do ograniczenia liczby pacjentów przebywających w przychodni jednocześnie,  synchronizując dostęp do "miejsca" w przychodni.
*   Zaimplementowano **obsługę sygnałów** (`signal`, `kill`) do sterowania pracą lekarzy (SIGUSR1 - "ewakuacja" na polecenie dyrektora, SIGTERM - zamknięcie przychodni) i rejestracji (SIGTERM - zamknięcie przychodni).
*   Zaimplementowano **wątki** (`pthread_create`, `pthread_join`, `pthread_mutex_lock`, `pthread_mutex_unlock`, `pthread_cond_wait`, `pthread_cond_signal`, `pthread_cond_broadcast`) w programie głównym (wątek symulacji czasu, wątek obsługi sygnału SIGUSR2) oraz w rejestracji (okienka rejestracji - potencjalnie do rozbudowy).
*   Zaimplementowano **obsługę pacjentów VIP**, którzy mają priorytet w rejestracji i u lekarzy.
*   Lekarze POZ **kierują pacjentów do specjalistów** (z wykorzystaniem kolejek komunikatów i rejestracji).
*   Specjaliści **kierują pacjentów na badania ambulatoryjne** (uproszczona symulacja).
*   Lekarze **obsługują pacjentów z kolejek po zamknięciu przychodni**.
*   Lekarze generują **raporty** po zakończeniu pracy, zapisywane do plików `report.txt`.
*   **Sprawdzanie poprawności danych** wprowadzanych przez użytkownika (rola lekarza).
*   **Obsługa błędów** funkcji systemowych za pomocą `perror()` i `errno`.

## 4. Problemy i trudności

*   

## 5. Elementy specjalne

*   **Pacjenci VIP:**  Implementacja priorytetowej obsługi pacjentów VIP w rejestracji i u lekarzy.
*   **Skierowania od lekarza POZ:**  Mechanizm kierowania pacjentów od lekarza POZ do specjalistów, z wykorzystaniem kolejek komunikatów i rejestracji.
*   **Obsługa pacjentów po zamknięciu przychodni:** Lekarze kontynuują przyjmowanie pacjentów, którzy byli już w kolejce w momencie zamknięcia przychodni.
*   **Symulacja czasu:**  Uproszczona symulacja upływu czasu w wątku `time_simulation` w `main.c`.
*   **Ewakuacja przychodni na sygnał dyrektora (SIGUSR1):**  Możliwość "grzecznego" zakończenia pracy lekarzy na polecenie dyrektora.
*   **Grzeczne zamykanie przychodni (SIGTERM):**  Program główny wysyła sygnał SIGTERM do wszystkich procesów (rejestracji, lekarzy, pacjentów) na zakończenie symulacji, umożliwiając im poprawne zakończenie pracy i posprzątanie zasobów.

## 6. Zauważone problemy z testami

*   Testy funkcjonalne symulacji ogólnie przebiegają poprawnie.  Symulacja uruchamia się, pacjenci są obsługiwani, lekarze kończą pracę, raporty są generowane.
*   Otrzeżenie **"Nieznany typ komunikatu w rejestracji: 0"** w `registration.c`, aby upewnić się, czy nie kryje się za nim błąd.
*   Testy **obciążeniowe** (np. zwiększenie liczby pacjentów), przechodzą poprawnie.

## 7. Linki do istotnych fragmentów kodu

*   **a. Tworzenie i obsługa plików:**
    *   `fopen`, `fprintf`, `fclose` (raporty lekarzy - `doctor.c`, funkcja `main` - zapis PIDów do plików):
*   **b. Tworzenie procesów:**
    *   `fork` (tworzenie procesów rejestracji, lekarzy, pacjentów w `main.c`): 
    *   `exec` (`execl` - uruchamianie programów rejestracji, lekarzy, pacjentów w `main.c`): 
    *   `exit` (zakończenie procesów potomnych w `registration.c`, `doctor.c`, `patient.c`, `main.c`): 
    *   `wait` (`waitpid` - czekanie na zakończenie procesów potomnych w `main.c`): 
*   **c. Tworzenie i obsługa wątków:**
    *   `pthread_create` (tworzenie wątków symulacji czasu i obsługi sygnałów w `main.c`, okienek rejestracji w `registration.c` - potencjalnie do rozbudowy): 
    *   `pthread_join` (czekanie na zakończenie wątków w `main.c`): `[LINK_DO_FUNKCJI_PTHREAD_JOIN_W_MAIN.C]`
    *   `pthread_mutex_lock`, `pthread_mutex_unlock` (ochrona dostępu do czasu symulacji - mutex `time_mutex` w `main.c`): 
*   **d. Obsługa sygnałów:**
    *   `kill` (wysyłanie sygnałów SIGUSR1 i SIGTERM do lekarzy, rejestracji i pacjentów w `director.c` i `main.c`): 
    *   `signal` (instalacja handlerów sygnałów SIGUSR1 i SIGTERM w `doctor.c`, SIGUSR2 w `main.c`): 
    *   `sigaction` (alternatywnie do `signal`, jeśli użyto bardziej zaawansowanej obsługi sygnałów): 
*   **e. Synchronizacja procesów(wątków):**
    *   `ftok` (generowanie kluczy dla semaforów i kolejek komunikatów w `main.c`, `doctor.c`, `registration.c`, `patient.c`, `director.c`): 

    *   `semop` (operacje na semaforze - `sem_wait` (opuszczanie) i `sem_post` (podnoszenie) semafora w `registration.c` i `patient.c`): `[LINK_DO_FUNKCJI_SEMOP_SEM_WAIT_W_REGISTRATION.C]` i 
    *   `semctl` (operacje kontrolne na semaforze - `sem_close`, `sem_unlink` w `main.c`): 
*   **f. Łącza nazwane i nienazwane:**
    *   *Projekt nie wykorzystuje łącz nazwanych (FIFO) ani nienazwanych (pipe) bezpośrednio do komunikacji między procesami.*  Komunikacja oparta jest o kolejki komunikatów.
*   **g. Segmenty pamięci dzielonej:**
    *   *Projekt nie wykorzystuje segmentów pamięci dzielonej.* (Jeśli jednak gdzieś użyto segmentów pamięci dzielonej, proszę dodać linki i opis).
*   **h. Kolejki komunikatów:**
    *   `msgget` (tworzenie kolejek komunikatów - kolejki dla rejestracji, lekarzy, ogólna kolejka przychodni w `main.c`, `doctor.c`, `registration.c`, `patient.c`, `director.c`):
    *   `msgsnd` (wysyłanie komunikatów do kolejek w `registration.c`, `doctor.c`, `patient.c`, `director.c`): 
    *   `msgrcv` (odbieranie komunikatów z kolejek w `registration.c`, `doctor.c`, `patient.c`): 
    *   `msgctl` (operacje kontrolne na kolejkach komunikatów - usuwanie kolejek w `main.c`): 
*   **i. Gniazda (socket(), bind(), listen(), accept(), connect()):**
    *   *Projekt nie wykorzystuje gniazd (socketów).* (Jeśli jednak gdzieś użyto gniazd, proszę dodać linki i opis).

---
