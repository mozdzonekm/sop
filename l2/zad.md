# Sop-rune — Kodeks z Wież Bardów

## Treść

W zamku Lśniącego Diamentu księżniczka Joanna potrzebuje odtworzyć **Kodeks Runiczny**. Bardowie z różnych wież dyktują linie kodeksu przez łącza nazwane. Bardowie jednak są nieprzytomni: mówią w kawałkach, mylą numery linii, czasami wysyłają błędne znaki, a potem długo piją herbatę.

Napisz program:

```sh
sop-rune kodeks tower1 [tower2 ... tower5]
```

gdzie:

- `kodeks` — ścieżka do pliku kodeksu,
- `tower1 ... tower5` — ścieżki do łącz nazwanych, przez które dyktują bardowie.

Program ma:

1. przygotować lub zweryfikować plik kodeksu,
2. odczytać dane z podanych łącz,
3. zapisać poprawne linie do kodeksu,
4. na końcu wyczyścić runy i wypisać raport.

Zadanie musi być rozwiązane **wyłącznie funkcjami niskopoziomowymi**. Busy waiting jest zabroniony.

Wymagane jest użycie:

- `readv` do odczytu rekordów z łącz nazwanych,
- `writev` do wypisywania surowych rekordów w pierwszym etapie oraz do generowania raportu końcowego,
- `poll` do obsługi wielu łącz,
- `mmap` do odczytu i modyfikacji pliku kodeksu,
- `lseek` do sprawdzenia rozmiaru pliku,
- `stat` / `fstat` i `S_ISFIFO` do sprawdzenia, czy podana ścieżka jest łączem nazwanym.

---

## Wymagania techniczne

- Pliki i deskryptory obsługuj wyłącznie niskopoziomowo.
- Do odczytu danych z łącz używaj `readv`, a nie `read`.
- Do zapisywania surowych rekordów w etapie 1 oraz do zapisywania wierszy raportu końcowego używaj `writev`.
- Nie używaj `fopen`, `fread`, `fwrite` do odczytu/zapisu danych.
- Busy waiting jest zabroniony. Czekanie na dane realizuj przez `poll`.
- Każde łącze otwieraj z flagą `O_NONBLOCK`.
- Na końcu zwolnij zasoby: `munmap` i `close` dla wszystkich otwartych deskryptorów.

---

## Format pliku kodeksu

Plik kodeksu ma rozmiar dokładnie:

```text
2048 bajtów
```

To 32 linie po 64 bajty:

```text
63 znaki ASCII + '\n'
```

Linie są numerowane od `00` do `31`.

Pusta linia to:

```text
63 spacje + '\n'
```

---

## Format rekordu wysyłanego przez barda

Każdy rekord ma długość dokładnie:

```text
67 bajtów
```

Format rekordu:

```text
NN:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n
```

czyli:

| bajty  | zawartość |
|-------:|-----------|
| 0–1    | dwucyfrowy numer linii `NN` |
| 2      | separator `:` |
| 3–65   | dokładnie 63 bajty treści, ASCII, dopełnione spacjami z prawej, jeśli treść jest krótsza |
| 66     | znak końca rekordu `'\n'` |

Przykład poprawnego rekordu:

```text
07:Beat the egg whites stiff with a pinch of salt           \n
```

Gdzie po treści są spacje dopełniające, a na końcu `\n`.

Rekord jest poprawny, gdy:

1. pierwsze dwa bajty to cyfry,
2. liczba `NN` mieści się w zakresie `0–31`,
3. trzeci bajt to `':'`,
4. ostatni bajt to `'\n'`.

Treść może zawierać dowolne znaki ASCII, w tym token:

```text
raw~berry
```

Niepoprawne rekordy są ignorowane.

---

## Czego wymaga zadanie od użycia operacji wektorowych

Odczyt każdego 67-bajtowego rekordu z łącza nazwanego wykonaj przez `readv`, rozpraszając bajty do osobnych buforów, np.:

```c
char num[2];
char sep[1];
char body[63];
char nl[1];

struct iovec iov[4] = {
    { num,  2  },
    { sep,  1  },
    { body, 63 },
    { nl,   1  }
};
```

Pamiętaj, że `readv` może:

- wczytać cały rekord,
- wczytać tylko część rekordu,
- wczytać tylko część pierwszego bufora,
- zwrócić `0` przy EOF,
- zwrócić `-1` z `EAGAIN` / `EWOULDBLOCK`, jeśli dane jeszcze nie są dostępne.

Dlatego każde łącze musi mieć własny stan niedokończonego rekordu.

Zapis raportu końcowego wykonaj przez `writev`. Każdy wiersz raportu możesz skomponować jako osobne fragmenty, np.:

```c
char number[3];      // np. "07"
char prefix[3] = { ':', ' ', '\0' };
char content[64];    // treść bez spacji dopełniających
char newline[1] = { '\n' };

struct iovec out[4] = {
    { number,  2 },
    { prefix,  2 },
    { content, len },
    { newline, 1 }
};

writev(STDOUT_FILENO, out, 4);
```

---

## Etapy

### Etap 1: Inicjalizacja kodeksu i surowy odczyt z jednej wieży

Jeśli plik kodeksu nie istnieje:

1. utwórz go funkcją `writev`,
2. zapisz 32 puste linie,
3. każda linia ma być zapisana jako co najmniej dwa fragmenty:
   - 63 spacje,
   - `'\n'`.

Jeśli plik kodeksu istnieje:

1. otwórz go,
2. sprawdź przez `lseek`, czy ma dokładnie `2048` bajtów,
3. jeśli ma inny rozmiar, wypisz na stderr:

```text
To nie jest kodeks!
```

i zakończ działanie.

Następnie sprawdź łącze `tower1`:

- jeśli plik nie istnieje,
- albo nie jest łączem nazwanym,

wypisz na stderr:

```text
Nie ma barda na wieży X
```

gdzie `X` to podana ścieżka, i zakończ działanie.

Po sprawdzeniu otwórz łącze do odczytu z flagą `O_NONBLOCK`.

Czytaj z łącza rekordy aż do EOF.

W tym etapie:

- nie waliduj treści,
- nie zapisuj do kodeksu,
- wypisz każdy rekord bez zmian na stdout.

Odczyt rekordu wykonaj przez `readv`, a wypisanie rekordu na stdout wykonaj przez `writev`.

---

### Etap 2: Walidacja i zapis linii do kodeksu

Zamiast wypisywać surowe rekordy, program ma je teraz interpretować.

Dla każdego odczytanego rekordu:

1. sprawdź, czy jest poprawny,
2. jeśli jest poprawny:
   - zapisz jego treść do linii o numerze `NN` w pliku kodeksu,
   - do linii trafia tylko 63 bajty treści,
   - nie trafia separator `NN:` ani znak `'\n'`,
   - wypisz na stdout:

     ```text
     Linia NN zapisana
     ```

     gdzie `NN` ma zawsze dwie cyfry, np. `07`,

3. jeśli rekord jest niepoprawny:
   - zignoruj go,
   - wypisz na stderr:

     ```text
     Bard się zagadał: X
     ```

     gdzie `X` to ścieżka łącza, z którego przyszedł rekord.

Do zapisywania linii do pliku kodeksu użyj pamięci zmmapowanej przez `mmap`.

Przed odczytem z łącz otwórz plik kodeksu z flagą `O_RDWR` i zmapuj go z `MAP_SHARED` oraz `PROT_READ | PROT_WRITE`.

---

### Etap 3: Obsługa wielu wież przez `poll`

Program otwiera wszystkie podane łącza `tower1 ... tower5`.

Przed rozpoczęciem obsługi sprawdź wszystkie łącza:

- jeśli któreś nie istnieje,
- albo nie jest łączem nazwanym,

wypisz na stderr:

```text
Nie ma barda na wieży X
```

i zakończ działanie.

Następnie program czeka na dane z wszystkich aktywnych łącz naraz, używając funkcji `poll`.

Każde łącze musi mieć osobny stan odczytu, ponieważ kawałki rekordów z różnych łącz mogą przychodzić naprzemiennie.

Po wykryciu EOF na jakimś łączu:

1. zamknij jego deskryptor,
2. wypisz na stdout:

   ```text
   Bard z wieży X skończył
   ```

3. usuń to łącze z monitorowania przez `poll`.

Jeśli przez `3` sekundy na żadnym aktywnym łączu nie pojawią się dane, wypisz na stdout:

```text
Bardowie dolewają herbaty...
```

i dalej czekaj.

Gdy wszystkie łącza zostaną zamknięte, przejdź do etapu 4.

---

### Etap 4: Czystka runów i raport końcowy

Gdy wszystkie łącza zostaną zamknięte:

1. w pamięci kodeksu zamień każde wystąpienie:

   ```text
   raw~berry
   ```

   na:

   ```text
   raspberry
   ```

   Token i zastępnik mają tę samą długość, więc nie musisz przesuwać reszty linii.

2. wypisz na stdout linię:

   ```text
   Wyczyszczone runy: N
   ```

   gdzie `N` to liczba zamienionych wystąpień.

3. wypisz na stdout każdy niepusty krok w formacie:

   ```text
   NN: treść
   ```

   gdzie:
   - `NN` ma zawsze dwie cyfry,
   - `treść` jest wypisana bez spacji dopełniających z końca linii,
   - linie wypisuj w kolejności numerów: od `00` do `31`,
   - linie, które składają się wyłącznie z samych spacji, są uznawane za puste i nie są wypisywane.

4. na końcu wypisz:
   - jeśli istnieje co najmniej jedna pusta linia:

     ```text
     Brakuje K linii, kodeks sie rozpadnie!
     ```

     gdzie `K` to liczba pustych linii,

   - jeśli żadna linia nie jest pusta:

     ```text
     Kodeks gotowy!
     ```

Cały raport końcowy zapisz przez `writev`.

Na końcu programu:

1. zwolnij mapowanie przez `munmap`,
2. zamknij wszystkie otwarte deskryptory.

---

## Jak pokazać działanie

### Przykład 1: normalne działanie

Terminal 1:

```sh
$ ./bard.sh tower1 10
```

Terminal 2:

```sh
$ ./bard.sh tower2 10
```

Terminal 3:

```sh
$ ./sop-rune kodeks.txt tower1 tower2
```

Po zakończeniu:

```sh
$ cat kodeks.txt
$ grep -c '~' kodeks.txt
```

Po poprawnym działaniu `grep -c '~' kodeks.txt` powinien wypisać `0`, o ile nie pojawiły się inne tilde w treści.

---

### Przykład 2: brak barda

Terminal:

```sh
$ ./sop-rune kodeks.txt tower_missing
```

Oczekiwany błąd:

```text
Nie ma barda na wieży tower_missing
```

---

### Przykład 3: plik nie jest kodeksem

Uruchom na pliku, który ma inny rozmiar niż `2048` bajtów:

```sh
$ ./sop-rune wrong.txt tower1
```

Oczekiwany błąd:

```text
To nie jest kodeks!
```

---

### Przykład 4: komunikat o pauzie

Uruchom jednego barda:

```sh
$ ./bard.sh tower1 20
```

Następnie:

```sh
$ ./sop-rune kodeks.txt tower1
```

Dzięki losowym pauzom w skrypcie można zobaczyć komunikat:

```text
Bardowie dolewają herbaty...
```

## Wymagane materiały

Do rozwiązania zadania przydadzą się:

```text
man 3p readv
man 3p writev
man 3p poll
man 3p open
man 3p close
man 3p lseek
man 3p fstat
man 3p mmap
man 3p munmap
man 7 fifo
man 0p sys_uio.h
man 0p poll.h
```

---

## Kryteria odbioru

Program powinien:

- akceptować od 1 do 5 wież,
- tworzyć brakujący kodeks przez `writev`,
- sprawdzać istniejący kodeks przez `lseek`,
- sprawdzać łącza przez `stat` / `fstat` i `S_ISFIFO`,
- odczytywać rekordy przez `readv`,
- obsługiwać częściowe odczyty,
- obsługiwać kilka łącz jednocześnie przez `poll`,
- wypisywać komunikat o pauzie co 3 sekundy,
- poprawnie kończyć się po EOF na wszystkich łączach,
- zapamiętywać poprawne linie w zmapowanym pliku,
- zamieniać `raw~berry` na `raspberry`,
- wypisywać raport przez `writev`,
- nie robić busy waitingu,
- zwolnić wszystkie zasoby.
