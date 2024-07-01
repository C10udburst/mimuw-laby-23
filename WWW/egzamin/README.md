Informacje ogólne
======

1. Zadania programistyczne są dwa, za pierwsze można uzyskać 10 punktów, za drugie 8 punktów, łącznie z 2 punktami za test, maksymalna liczba punktów do zdobycia wynosi 20.
2. W każdym z zadań jest precyzyjnie napisane jakie pliki należy oddać.
3. Proszę w każdym z oddawanych plików wpisać w komentarzu swoje imię i nazwisko.

Zadanie 1: Serwer FastAPI
=====

Cel
---

W tym zadaniu stworzysz serwer FastAPI (`zad1.py`), który będzie komunikował się z innym serwerem (`zad1-server.py`) symulującym prawdziwą stronę internetową. Dzięki temu przetestujemy Twoje umiejętności pracy z FastAPI bez konieczności korzystania z zewnętrznych zasobów. Po uruchomieniu Twojego serwera na porcie 8001, będziesz mógł używać przeglądarki do interakcji z nim.

Jedynym plikiem, który należy oddać w tym zadaniu, to `zad1.py`

Wymagania
---

Twoim celem jest uzupełnienie kodu w pliku `zad1.py`, który zawiera początkową strukturę serwera FastAPI. Serwer powinien udostępniać następujące endpointy:

1. **Pobieranie liczby zdań - 2 pkt:** Pobiera stronę o podanym seedzie z serwera symulującego stronę internetową i zwraca liczbę zdań na tej stronie. Uwaga: zdania zawsze zaczynają się od wielkiej litery i kończą się kropką.
2. **Pobieranie liczby odnośników - 3 pkt:** Pobiera stronę o podanym seedzie i zwraca liczbę odnośników (linków) na niej. W tym punkcie ważne jest prawidłowe obsłużenie potencjalnych błędów, takich jak brak strony o danym seedzie albo błąd połączenia z serwerem. W pozostałych punktach nie musisz się martwić o obsługę błędów.
3. **Pobieranie stron z linkami (głębokość 1) - 2 pkt:** Pobiera strony o podanych seedach, a także wszystkie strony, do których można przejść z nich jednym kliknięciem. Zwraca listę wszystkich pobranych stron (wraz z ich seedami) oraz liczbę zdań na każdej z nich.
4. **Pobieranie stron z linkami (głębokość k) - 3pkt:** Pobiera strony o podanych seedach oraz wszystkie strony osiągalne z nich poprzez maksymalnie k kliknięć (k ≤ 3), eliminując duplikaty. Zwraca unikalną listę pobranych stron (wraz z seedami) i liczbę zdań na każdej stronie. Limit czasu wykonania: 60 sekund.

Uwagi:
---

1. **Modele Pydantic:** Dla każdego z punktów 2-4 przygotuj odpowiednie modele danych w Pydantic, które będą służyć do walidacji danych wejściowych i wyjściowych. W kodzie znajdziesz przykłady, jak to zrobić.
2. **Dokumentacja:** Do każdego z endpointów dodaj krótką dokumentację (1-2 zdania), która będzie dostępna w automatycznie generowanej dokumentacji FastAPI (pod adresem /docs).
3. **Komunikacja między `zad1-server.py` a `zad1.py`:** Komunikacja między serwerem symulującym stronę a oddawanym serwerem FastAPI powinna odbywać się za pomocą protokołu HTTP, w szczególności nie należy ani importować `zad1-server.py` ani przekopiowywać z niego funkcji do `zad1.py`.
4. **Przykładowe testy:** W pliku `zad1-tests.sh` znajdują się przykładowe testy a w pliku `zad1-tests-results.txt` ich wyniki.

Zadanie 2: Klient WebSocket w TypeScript
======

Cel
---

Celem zadania jest stworzenie klienta WebSocket w języku TypeScript, który będzie komunikować się z serwerem Python (`zad2-server.py`) dostarczającym aktualne ceny. Klient powinien wyświetlać otrzymaną cenę na stronie internetowej (`zad2.html`) bez konieczności jej modyfikacji.

Oddać należy `zad2.ts`, `zad2.js` i `README-zad2.md`

Wymagania
----

1. **Stworzenie klienta WebSocket - 2 pkt:** Zaimplementuj klienta w pliku zad2.ts, który nawiąże połączenie z serwerem WebSocket i będzie reagował na zmiany cen wyświetlane przez serwer.
2. **Wyświetlanie ceny - 2 pkt:** Wyświetl otrzymaną od serwera cenę w odpowiednim miejscu na stronie zad2.html. Na stronie powinna być wyświetlona tylko jedna, aktualna cena.
3. **Obsługa błędów - 2 pkt:** Zapewnij obsługę błędów, takich jak utrata połączenia z serwerem czy nieprawidłowe dane. Wyświetl stosowny komunikat w przypadku wystąpienia błędu i daj użytkownikowi możliwość
ponownego połączenia się do serwera bez przeładowywania całej strony.
4. **Użycie TypeScript - 1 pkt:** Wykorzystaj TypeScript do implementacji klienta, dbając o poprawność typów i czytelność kodu.
5. **Skompilowanie do JavaScript - 1 pkt:** Skompiluj plik zad2.ts do pliku zad2.js, który będzie uruchamiany przez przeglądarkę i opisz w README-zad2.md jak to zrobiłeś.

Dokumentacja: Stwórz plik `README-zad2.md` z instrukcją kompilacji pliku `zad2.ts`.

Kryteria oceny
---

1. **Poprawność działania:** Klient prawidłowo łączy się z serwerem, odbiera ceny i wyświetla je na stronie.
2. **Obsługa błędów:** Klient radzi sobie z błędami, takimi jak utrata połączenia czy nieprawidłowe dane.
3. **Jakość kodu TypeScript:** Kod jest czytelny, dobrze ustrukturyzowany i wykorzystuje zalety TypeScript.
4. **Dokumentacja:** Plik README zawiera jasną instrukcję kompilacji.
