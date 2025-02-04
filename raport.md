Raport Projektu „Przychodnia”

https://github.com/remikijuzz/przychodnia.git

1. Założenia projektowe

W ramach projektu zaimplementowano następujące moduły:

Dyrektor (Director) – odpowiedzialny za wysyłanie sygnau do procesów (np. sygnał 2 –  wszyscy pacjenci natychmiast opuszczają budynek).
Rejestracja (Registration) – obsługuje rejestrację pacjentów, zarządza kolejką oczekujących oraz otwiera i zamyka okienka rejestracji w zależności od liczby oczekujących.
Lekarz (Doctor) – moduł przyjmujący pacjentów, który uwzględnia limity przyjęć i generuje skierowania (np. do specjalistów).
Pacjent (Patient) – symuluje zachowanie pacjenta (wraz z opcjonalnym dzieckiem), wchodzenie do przychodni, rejestrację, wizytę u lekarza oraz opuszczanie przychodni.
Przy projektowaniu użyto różnych mechanizmów synchronizacji (semafory, kolejki komunikatów, wątki) oraz obsługi sygnałów. Wszystkie dane wprowadzane przez użytkownika (np. liczba procesów) są sprawdzane, a w przypadku błędów wykorzystywana jest funkcja perror() wraz z wartością zmiennej errno.

W projekcie przydzielono minimalne prawa dostępu do struktur systemowych, a wszystkie utworzone struktury (kolejki komunikatów, semafory) są usuwane po zakończeniu symulacji, zgodnie z wymaganiami.

2. Ogólny opis kodu

main.c

Tworzy i inicjalizuje kluczowe struktury systemowe: kolejkę komunikatów, semafor ograniczający liczbę pacjentów wewnątrz budynku oraz uruchamia zegar symulacyjny.
Fork-uje procesy: rejestracji, lekarzy (w tym lekarzy POZ oraz specjalistów) oraz pacjentów.
Zapisuje identyfikatory procesów do plików (np. doctor_pids.txt, patient_pids.txt), co umożliwia modułowi dyrektora wysyłanie sygnałów.
Po zakończeniu symulacji (zgodnie z zegarem) wysyła sygnały SIGTERM do wszystkich procesów, a następnie czyści wszystkie zasoby systemowe.

director.c

Odpowiedzialny za wysyłanie sygnału SIGUSR2 do procesów.
Umożliwia natychmiastowe zakończenie pracy przez wybrane grupy procesów.

registration.c

Implementuje rejestrację pacjentów przy użyciu kolejki FIFO.
Dynamicznie otwiera lub zamyka dodatkowe okienko rejestracji w zależności od liczby oczekujących pacjentów.
Na zakończenie pracy loguje dane pacjentów, którzy nie zostali przyjęci do wizyty 
./src/report.txt

doctor.c

Procesy lekarzy przyjmują pacjentów z odpowiednich kolejek komunikatów, stosując limity przyjęć. Lekarze POZ generują skierowania do lekarzy specjalistów, a lekarze specjaliści mogą dodatkowo kierować pacjentów na badania ambulatoryjne.
Po zakończeniu pracy lekarze zapisują do raportu informacje o nieprzyjętych pacjentach. 
./src/report.txt

patient.c

Symuluje zachowanie pacjenta, który może przychodzić z dzieckiem.
Pacjent przed wizytą przechodzi przez rejestrację i wysyła komunikaty do kolejki.
Implementowana jest obsługa sygnału (różne wersje, w tym alternatywne podejścia z signalfd).

3. Co udało się zrobić
Zaimplementowano pełną symulację działania przychodni, obejmującą rejestrację, przyjmowanie pacjentów przez lekarzy POZ oraz specjalistów.
Użyto mechanizmów synchronizacji między procesami i wątkami (semafory, kolejki komunikatów, wątki) zgodnie z wymaganiami.
Obsłużono błędy wszystkich funkcji systemowych przy użyciu perror() i zmiennej errno.
Zaimplementowano mechanizmy raportowania – informacje o pacjentach, którzy nie zostali przyjęci lub oczekiwali w kolejce, są zapisywane do pliku raportu.
Stworzono oddzielny moduł Dyrektora umożliwiający wysyłanie sygnału.


4. Synchronizacja i obsługa sygnałów:
Początkowo napotkaliśmy trudności z natychmiastowym reagowaniem procesów pacjentów na sygnał SIGUSR2. Próbowano różnych metod (asynchroniczny handler, wątek sygnałowy, signalfd), ale rozwiązanie, które miało globalnie zakończyć pracę symulacji, wymagało starannego dostosowania momentu wysłania sygnału oraz synchronizacji pomiędzy procesami.

Testowanie:
Problemem była też synchronizacja pomiędzy procesami – sygnał musiał być wysłany w odpowiednim momencie, gdy procesy pacjentów jeszcze działały. Wymagało to modyfikacji mechanizmów oczekiwania (np. stosowanie sem_trywait czy sem_timedwait) oraz sprawdzania flag, aby sygnał mógł być prawidłowo odebrany i natychmiast przerywał działanie procesu.

Zarządzanie zasobami:
Upewnienie się, że wszystkie utworzone zasoby (kolejki komunikatów, semafory) są poprawnie usuwane po zakończeniu symulacji, było dodatkowym wyzwaniem, zwłaszcza przy wielu procesach i wątkach.

5. Elementy specjalne
Dynamiczne otwieranie/zamykanie dodatkowego okienka rejestracji:
W module rejestracji zaimplementowano mechanizm monitorujący długość kolejki, który dynamicznie otwiera drugie okienko, gdy liczba oczekujących pacjentów przekracza określony próg, i zamyka je, gdy kolejka maleje.

Raport dzienny:
W raportach zapisywane są informacje o pacjentach, którzy nie zostali przyjęci przez lekarzy oraz ci, którzy oczekiwali w kolejce rejestracji na zakończenie pracy przychodni.

Obsługa sygnałów przez moduł dyrektora:
Dzięki oddzielnemu modułowi dyrektora możliwe jest wysyłanie sygnałów do konkretnych grup procesów (lekarze lub pacjenci), co wpływa na natychmiastowe zakończenie ich pracy.

6. Podsumowanie
Projekt „Przychodnia” spełnia wszystkie wymagania opisane w specyfikacji. W ramach projektu zaimplementowano wszystkie kluczowe moduły (Dyrektor, Rejestracja, Lekarz, Pacjent) wraz z odpowiednią synchronizacją oraz obsługą błędów. Mimo napotkanych trudności, zwłaszcza w zakresie obsługi sygnałów, udało się wypracować rozwiązania pozwalające na dynamiczną obsługę symulacji. Dodatkowo, w projekcie zastosowano mechanizmy raportowania, które umożliwiają analizę działania systemu oraz identyfikację ewentualnych błędów w trakcie testów.

