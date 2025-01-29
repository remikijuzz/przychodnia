Celem projektu jest symulacja działania przychodni z lekarzami POZ i specjalistami, dynamicznym zarządzaniem kolejkami oraz obsługą pacjentów VIP i dzieci z opiekunami.

- Przychodnia działa w ustalonych godzinach (Tp do Tk).

- Dynamiczne zarządzanie kolejkami w rejestracji.

- Lekarze POZ kierują pacjentów do specjalistów lub obsługują ich na miejscu.

- Specjaliści mogą kierować pacjentów na badania ambulatoryjne.

- Pacjenci VIP mają priorytet w rejestracji i obsłudze.

- Dzieci rejestrowane wyłącznie z opiekunem.

Użyte struktury i algorytmy:

- Struktury Doctor i Patient do zarządzania lekarzami i pacjentami.

- Kolejki cykliczne do obsługi pacjentów.

- Mechanizm dynamicznego zarządzania okienkami rejestracji.

Wyniki:

- Lekarze POZ obsługują 60% pacjentów, reszta trafia do specjalistów.

- Specjaliści kierują 10% pacjentów na badania ambulatoryjne.

- Raport dzienny zawiera szczegóły dotyczące obsługi pacjentów.

Funkcje systemowe:

- Synchronizacja: pthread_mutex_lock(), pthread_cond_signal().

- Obsługa wątków: pthread_create(), pthread_join().

- Obsługa sygnałów: signal().

