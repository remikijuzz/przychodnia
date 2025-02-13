# GAWENDA_REMIGIUSZ_151865_opis_przychodnia.docx

*Link do repozytorium GitHub: https://github.com/remikijuzz/przychodnia.git

## 1. Założenia projektowe kodu

Projekt symuluje działanie przychodni lekarskiej, która składa się z następujących elementów:
- **Rejestracja:** Pacjenci przychodzą, rejestrują się i są kierowani do lekarzy.
- **Lekarze:** Są dwóch lekarzy POZ (pierwszego kontaktu) oraz lekarze specjalistyczni: kardiolog, okulista, pediatra oraz lekarz medycyny pracy. Każdy lekarz ma określony limit przyjęć – lekarze POZ przyjmują około 60% pacjentów, natomiast specjaliści po około 10%.
- **Pacjenci:** Losowo przybywają do przychodni, rejestrują się, a następnie udają się na wizytę u lekarza. Pacjenci VIP są obsługiwani priorytetowo.
- **Dyrektor:** Umożliwia sterowanie symulacją poprzez wysyłanie sygnałów:  
  - SIGUSR1 – powoduje, że wybrany lekarz (kontrolowany przez argument, np. "okulista" lub "pediatra") kończy przyjmowanie nowych pacjentów po obsłużeniu bieżącego.
  - SIGUSR2 – ewakuacja całej przychodni (główny proces kończy pracę, wysyłane są sygnały SIGTERM do wszystkich procesów).

Aplikacja oparta jest na wieloprocesowości (fork/exec) oraz na mechanizmach komunikacji międzyprocesowej (kolejki komunikatów, semafory) i synchronizacji (mutexy, wątki, sygnały).

## 2. Ogólny opis kodu

Projekt składa się z następujących programów:

- **`main` (przychodnia – main.c):**  
  - Tworzy zasoby IPC: kolejki komunikatów (dla rejestracji oraz poszczególnych lekarzy) oraz semafor ograniczający liczbę pacjentów w przychodni.
  - Uruchamia wątki:  
    - **Time simulation:** symuluje upływ czasu (co 4 sekundy symulowany czas zwiększa się o 1 godzinę); po osiągnięciu godziny zamknięcia przychodni (Tk) ustawiana jest flaga `clinic_open = 0`.
    - **Signal handler:** obsługuje sygnał SIGUSR2 (ewakuacja) – po jego odebraniu wysyłane są sygnały SIGTERM do procesów rejestracji, lekarzy i pacjentów oraz zasoby IPC są sprzątane.
    - **Reaper threads:** Wątki zbierające zakończone procesy pacjentów i lekarzy (wywołujące waitpid(WNOHANG)) eliminują procesy zombie.
  - Fork’uje procesy rejestracji, lekarzy i pacjentów. Lekarze są uruchamiani z odpowiednimi argumentami (np. "POZ" dla rejestracji, ale w wersji z nowym podziałem – "poz1" oraz "poz2" dla lekarzy POZ).
  - Po zakończeniu symulacji, główny proces wysyła SIGTERM do rejestracji, lekarzy i pacjentów, czeka na zakończenie procesów, dołącza wątki reaper, a następnie czyści zasoby IPC.

- **`registration` (rejestracja – registration.c):**  
  - Symuluje rejestrację pacjentów.  
  - Uruchomione są dwa okienka rejestracji (realizowane jako wątki), które odbierają komunikaty od pacjentów z kolejki i kierują ich do odpowiednich lekarzy, wysyłając komunikaty do kolejki lekarzy.
  - Dynamicznie otwiera się drugie okienko, gdy liczba oczekujących pacjentów przekracza określony próg (K >= N/2) i zamyka, gdy spada poniżej innego progu (N/3).
  - Obsługuje pacjentów VIP (którym zapewniony jest priorytet).

- **`doctor` (lekarz – doctor.c):**  
  - Symuluje pracę lekarza, którego specjalność jest przekazywana jako argument (np. "kardiolog", "okulista", "pediatra", "lekarz_medycyny_pracy").  
  - Dla lekarzy POZ wprowadzono rozróżnienie na dwa procesy:  
    - Procesy te są uruchamiane z argumentami "poz1" i "poz2" i zapisują swoje PID-y do plików `poz1_pid.txt` i `poz2_pid.txt`.
  - Lekarz odbiera pacjentów z dedykowanej kolejki, symuluje wizytę lekarską, a lekarze POZ dodatkowo mogą kierować pacjentów do specjalistów (poprzez wysyłanie skierowań do rejestracji).
  - Specjaliści mają możliwość skierowania pacjentów na badania ambulatoryjne (uproszczona symulacja).
  - Lekarze reagują na sygnały:
    - **SIGUSR1:** Obsługiwany przez mechanizm signalfd; po jego odebraniu lekarz kończy przyjmowanie nowych pacjentów, ale nadal obsługuje pacjentów z kolejki, a następnie generuje raport.
    - **SIGTERM:** Powoduje natychmiastowe zakończenie pracy.
  - Wątki reapujące (zbierające) procesy lekarzy w main.c usuwają zombie.

- **`patient` (pacjent – patient.c):**  
  - Symuluje pacjenta, który losowo przybywa do przychodni, rejestruje się (wysyłając komunikat do rejestracji), a następnie udaje się do lekarza (w zależności od wyboru lub skierowania).
  - Po zakończeniu wizyty pacjent opuszcza przychodnię.  
  - Pacjenci VIP są obsługiwani priorytetowo.

- **`director` (dyrektor – director.c):**  
  - Umożliwia interakcję z symulacją.  
  - Dyrektor może wysłać sygnał SIGUSR1 do konkretnego lekarza, wykorzystując nazwę pliku PID – kończąc pracę pojedynczego lekarza.  
  - Dyrektor może także wysłać SIGUSR2 do głównego procesu (main), co powoduje natychmiastową ewakuację całej przychodni.
  - W jednym wywołaniu dyrektor może wysłać oba sygnały (np. `./director -s1 pediatra -s2`), co najpierw powoduje zakończenie pracy konkretnego lekarza, a następnie ewakuację całej przychodni.

- **Testy jednostkowe i obciążeniowe:**  
    
  **testy.md:** Szczegółowy opis testów.
  
  W projekcie oprócz głównych programów znajdują się testy:
  - **test_queue.c:** Testy funkcji kolejki (enqueue/dequeue).
  - **test_queue_monitor.c:** Testy warunków dynamicznego otwierania/zamykania drugiego okienka rejestracji.
  - **test_semaphore.c:** Testy semaforów – sprawdzenie poprawności operacji sem_wait/sem_post.
  - **test_signals.c:** Testy obsługi sygnałów (SIGUSR1, SIGTERM, SIGUSR2) z weryfikacją czyszczenia zasobów.
  - **test_ipc.c:** Testy komunikacji między procesami za pomocą kolejek komunikatów (msgsnd, msgrcv).
  - **test_load.c:** Test obciążeniowy, który symuluje wysyłanie komunikatów przez procesy dzieci (pacjentów) oraz ich odbiór przez proces rodzica, w różnych iteracjach, z zastosowaniem mechanizmu czyszczenia kolejki między iteracjami.

## 3. Co udało się zrobić

- Zaimplementowano kompletną symulację przychodni, obejmującą rejestrację, lekarzy różnych specjalności i pacjentów, czyli:

• Przychodnia jest czynna w godzinach od Tp do Tk;
• W budynku przychodni w danej chwili może się znajdować co najwyżej N pacjentów
(pozostali, jeżeli są czekają przed wejściem);
• Dzieci w wieku poniżej 18 lat do przychodni przychodzą pod opieką osoby dorosłej;
• Każdy pacjent przed wizytą u lekarza musi się udać do rejestracji;
• W przychodni są 2 okienka rejestracji, zawsze działa min. 1 stanowisko;
• Jeżeli w kolejce do rejestracji stoi więcej niż K pacjentów (K>=N/2) otwiera się drugie okienko
rejestracji. Drugie okienko zamyka się jeżeli liczba pacjentów w kolejce do rejestracji jest
mniejsza niż N/3;
• Osoby uprawnione VIP
• Jeżeli zostaną wyczerpane limity przyjęć do danego lekarza, pacjenci ci nie są przyjmowani
(rejestrowani);
• Jeśli w chwili zamknięcia przychodni w kolejce do rejestracji czekali pacjenci to te osoby nie
zostaną przyjęte w tym dniu przez lekarza. Dane tych pacjentów (id - skierowanie do …. -
wystawił) powinny zostać zapisane w raporcie dziennym.

Na polecenie Dyrektora (sygnał 1) dany lekarz bada bieżącego pacjenta i kończy pracę przed
zamknięciem przychodni. Dane pacjentów (id - skierowanie do …. - wystawił), którzy nie zostali
przyjęci powinny zostać zapisane w raporcie dziennym.

Na polecenie Dyrektora (sygnał 2) wszyscy pacjenci natychmiast opuszczają budynek.
Napisz procedury Dyrektor, Rejestracja, Lekarz i Pacjent symulujące działanie przychodni.

- Zastosowano wieloprocesowość (fork/exec) oraz komunikację międzyprocesową (kolejki komunikatów, semafory).
- Obsługa sygnałów SIGUSR1, SIGUSR2 i SIGTERM umożliwia zakończenie pracy poszczególnych komponentów (lekarzy, rejestracji, pacjentów).
- Wprowadzone zostały mechanizmy reapujące (wątki zbierające procesy lekarzy i pacjentów) eliminujące zombie.
- Testy jednostkowe i obciążeniowe potwierdzają poprawność działania poszczególnych mechanizmów.

## 4. Problemy i trudności

- **Osobisty "workflow"** Brak ustrukturyzowanej formy, implementacja modułów, naprawy błędów na bieżąco (najczęściej po prezentacji), zamiast zaplanowanej wcześniej strategii stworzenia projektu od początku do końca.
- **Procesy zombie:** Początkowo pojawiały się procesy zombie, które rozwiązałem poprzez wątki reapujące oraz handler SIGCHLD.
- **Limity systemowe dla kolejek komunikatów:** Testy obciążeniowe wykazały, że przy bardzo dużej liczbie komunikatów kolejka może osiągać swoje limity, co wymagało modyfikacji testów (iteracyjnych) lub zwiększenia limitów systemowych.
- **Obsługa sygnałów:** Implementacja sygnału SIGUSR1 dla pojedynczych lekarzy (wybieranych przez dyrektora) wymagała rozróżnienia procesów POZ (wprowadzono konwencję "pediatra" i "okulista") oraz odpowiedniej modyfikacji modułu director.
- **Dynamiczne okienka rejestracji:** Synchronizacja i dynamiczne otwieranie drugiego okienka rejestracji wymagały precyzyjnego ustawienia warunków, aby system działał stabilnie.
- **Testy obciążeniowe:** Implementacja testu obciążeniowego pokazała, że przy bardzo dużej liczbie komunikatów systemowe limity IPC mogą wpływać na działanie, co wymagało iteracyjnego opróżniania kolejki.

## 5. Zauważone problemy z testami

- Testy funkcjonalne wykazały, że symulacja działa poprawnie – pacjenci są obsługiwani, lekarze kończą pracę, raporty są generowane.
- Testy obciążeniowe pokazały ograniczenia systemowe kolejki komunikatów – przy bardzo dużej liczbie komunikatów kolejka może osiągać limit (co zostało rozwiązane poprzez iteracyjne opróżnianie kolejki).
- Testy sygnałów potwierdziły, że procesy poprawnie reagują na SIGUSR1 (selektywne wyłączanie lekarzy) i SIGUSR2 (ewakuacja całej przychodni).

1. **Tworzenie zasobów IPC i semaforów w main.c:**  
   Główny proces inicjalizuje kolejki komunikatów (dla rejestracji oraz poszczególnych lekarzy) oraz semafor kontrolujący liczbę pacjentów w przychodni.  
   [Zobacz fragment kodu – main.c (linie 35-55)](https://github.com/remikijuzz/przychodnia/blob/main/src/main.c#L35-L55)

2. **Wątek symulacji czasu (time_simulation) w main.c:**  
   Wątek symuluje upływ czasu, zwiększając go co określony interwał. Po osiągnięciu godziny zamknięcia (Tk) ustawiana jest flaga `clinic_open = 0`.  
   [Zobacz fragment kodu – main.c (linie 65-80)](https://github.com/remikijuzz/przychodnia/blob/main/src/main.c#L65-L80)

3. **Obsługa sygnału SIGUSR2 przez główny proces w main.c:**  
   Główny proces reaguje na sygnał SIGUSR2, kończąc pracę wszystkich procesów podrzędnych (rejestracji, lekarzy i pacjentów) oraz zwalniając zasoby IPC.  
   [Zobacz fragment kodu – main.c (linie 90-110)](https://github.com/remikijuzz/przychodnia/blob/main/src/main.c#L90-L110)

4. **Dynamiczne okienka rejestracji w registration.c:**  
   Drugie stanowisko rejestracji jest otwierane, gdy kolejka pacjentów przekracza próg K>=N/2, i zamykane, gdy spada poniżej N/3.  
   [Zobacz fragment kodu – registration.c (linie 85-130)](https://github.com/remikijuzz/przychodnia/blob/main/src/registration.c#L85-L130)

5. **Obsługa priorytetowych pacjentów VIP w registration.c:**  
   Pacjenci VIP są rejestrowani z priorytetem, co umożliwia im szybsze dostanie się do lekarza.  
   [Zobacz fragment kodu – registration.c (linie 140-170)](https://github.com/remikijuzz/przychodnia/blob/main/src/registration.c#L140-L170)

6. **Przydzielanie pacjentów do lekarzy POZ i specjalistów w doctor.c:**  
   Lekarze POZ obsługują około 60% pacjentów i kierują część z nich (20%) do specjalistów, którzy mają indywidualne kolejki.  
   [Zobacz fragment kodu – doctor.c (linie 100-140)](https://github.com/remikijuzz/przychodnia/blob/main/src/doctor.c#L100-L140)

7. **Obsługa skierowań na badania specjalistyczne w doctor.c:**  
   Lekarze specjaliści mogą kierować pacjentów na dodatkowe badania ambulatoryjne, po których pacjent wraca do specjalisty i wchodzi bez kolejki.  
   [Zobacz fragment kodu – doctor.c (linie 150-180)](https://github.com/remikijuzz/przychodnia/blob/main/src/doctor.c#L150-L180)

8. **Obsługa pacjentów – rejestracja i wybór lekarza w patient.c:**  
   Pacjent rejestruje się, po czym czeka na swoją kolej do lekarza, zgodnie z ustalonymi zasadami (kolejki wspólne i indywidualne, pacjenci VIP).  
   [Zobacz fragment kodu – patient.c (linie 50-90)](https://github.com/remikijuzz/przychodnia/blob/main/src/patient.c#L50-L90)

9. **Dyrektor – sterowanie systemem przez sygnały w director.c:**  
   Dyrektor może wysyłać sygnały SIGUSR1 do lekarzy, zmuszając ich do zakończenia pracy, oraz SIGUSR2 do zamknięcia całej przychodni.  
   [Zobacz fragment kodu – director.c (linie 30-70)](https://github.com/remikijuzz/przychodnia/blob/main/src/director.c#L30-L70)

## 8. Testy jednostkowe i obciążeniowe

- **Testy funkcji kolejki:**  
  Testy jednostkowe (plik `test_queue.c`) potwierdziły poprawność operacji na kolejce pacjentów (enqueue/dequeue).

- **Testy semaforów:**  
  Testy w pliku `test_semaphore.c` weryfikują, że semafor ograniczający liczbę pacjentów działa prawidłowo i każda operacja sem_wait jest sparowana z sem_post.

- **Testy komunikacji IPC:**  
  Testy w pliku `test_ipc.c` potwierdzają poprawność wysyłania i odbierania komunikatów między procesami (msgsnd/msgrcv).

- **Test obciążeniowy:**  
  Test obciążeniowy (plik `test_load.c`) symuluje wysyłanie komunikatów przez procesy dzieci i ich odbiór przez proces rodzica w kilku iteracjach. W każdej iteracji kolejka jest tworzona, wykorzystywana do wysyłania komunikatów, a następnie usuwana. Test wykazał, że przy małym obciążeniu (LOAD_NUM = 10) system działa poprawnie, a przy większych liczbach (po odpowiedniej iteracji) należy uwzględnić ograniczenia systemowe kolejki IPC.

- **Testy sygnałów:**  
  Testy (plik `test_signals.c`) sprawdzają, czy:
  - SIGUSR1 jest poprawnie odbierany przez lekarzy (np. "okulista", "pediatra") i powoduje zakończenie pracy tego konkretnego procesu.
  - SIGTERM jest obsługiwany przez wszystkie procesy (lekarzy, rejestrację, pacjentów) przy zamykaniu przychodni.
  - SIGUSR2 powoduje ewakuację całej przychodni (proces główny kończy działanie, a wszystkie zasoby są czyszczone).

- **Testy reapujące:**  
  W głównym procesie (main.c) uruchomiono wątki zbierające (patient_waiter_thread i doctor_waiter_thread), które cyklicznie wywołują waitpid(WNOHANG) i usuwają zakończone procesy, eliminując zombie.

*Link do repozytorium GitHub: https://github.com/remikijuzz/przychodnia.git
---
