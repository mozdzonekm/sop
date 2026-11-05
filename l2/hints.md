## Wskazówki

### 1. `readv` a częściowe odczyty

`readv` może zwrócić mniej bajtów, niż chciałeś. Nie oznacza to błędu.

Przy odczycie rekordu możesz trzymać coś w rodzaju:

```c
struct iovec iov[4];
size_t consumed = 0;
```

Gdzie `consumed` to liczba bajtów zebranych z bieżącego rekordu.

Po każdym `readv`:

1. dodaj zwróconą liczbę bajtów do `consumed`,
2. popraw pola `iov_base` i `iov_len`,
3. gdy `consumed == 67`, przetwórz rekord i zresetuj stan.

Przykład idei:

```c
while (state->consumed < 67) {
    ssize_t r = readv(fd, state->iov, state->iovcnt);

    if (r == 0) {
        // EOF
        break;
    }

    if (r < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // brak danych, czekaj na kolejny poll
            return WAIT;
        }
        // prawdziwy błąd
        break;
    }

    state->consumed += r;
    consume_iov(state->iov, &state->iovcnt, r);
}
```

Funkcja `consume_iov` powinna przesunąć wskaźniki i zmniejszyć długości buforów o ilość faktycznie odczytanych bajtów.

---

### 2. Każde łącze ma własny stan

Przy wielu łączach nie możesz mieć jednego wspólnego bufora dla wszystkich.

Możesz mieć tablicę:

```c
struct TowerState {
    int fd;
    const char *path;
    size_t consumed;
    int iovcnt;
    struct iovec iov[4];
};
```

Po każdym odczycie aktualizuj tylko stan danego łącza.

---

### 3. Łącza nazwane otwieraj jako nieblokujące

Otwieranie FIFO w trybie blokującym może sprawić, że program zacznie czekać na barda w nieoczekiwanych miejscach.

Używaj:

```c
O_RDONLY | O_NONBLOCK
```

Dzięki temu:

- `open` nie będzie długo czekał,
- `readv` może zwrócić od razu `EAGAIN`,
- realne czekanie realizuje `poll`.

---

### 4. EOF przy FIFO

Gdy bard zakończy działanie i zamknie łącze do pisania, odczyt z FIFO może zwrócić:

```c
0
```

To EOF.

Jeśli w tym momencie miałeś niedokończony rekord, po prostu go odrzuć i zamknij deskryptor.

---

### 5. Zmiana tokena w zmapowanym pliku

Token:

```text
raw~berry
```

zostaje zamieniony na:

```text
raspberry
```

Oba ciągi mają długość `9`.

Dlatego możesz przeszukiwać każdą linię w pamięci:

```c
for (int line = 0; line < 32; line++) {
    char *row = map + line * 64;

    for (int pos = 0; pos + 9 <= 63; pos++) {
        if (strncmp(row + pos, "raw~berry", 9) == 0) {
            memcpy(row + pos, "raspberry", 9);
            n++;
            pos += 8;
        }
    }
}
```

Po zamianie pamiętaj, że linia nadal ma 63 znaki plus `'\n'`.

---

### 6. Usuwanie spacji dopełniających przy raporcie

Linia w kodeksu ma 63 znaki treści.

Przed wypisaniem do raportu usuń końcowe spacje:

```c
int len = 63;

while (len > 0 && row[len - 1] == ' ') {
    len--;
}
```

Jeśli `len == 0`, linia jest pusta.

---

### 7. Raport przez `writev`

Dla każdej niepuste linii użyj `writev`, np. z fragmentami:

```c
char num[3];
char colon_space[3] = { ':', ' ', '\0' };
char newline[1] = { '\n' };

struct iovec out[4] = {
    { num,         2 },
    { colon_space, 2 },
    { row,         len },
    { newline,     1 }
};

writev(STDOUT_FILENO, out, 4);
```