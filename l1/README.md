# L1: Magazyn Składników Alchemicznych i Receptur

Witaj w Średniowiecznej Gildii Alchemików! Jako nowy pomocnik mistrza masz za zadanie uporządkować archiwum zawierające opisy składników, księgi oraz fiolki z odczynnikami. Całe laboratorium zostało odwzorowane w postaci hierarchicznej struktury katalogów i plików w systemie Linux.

Twoim zadaniem jest napisanie programu w języku C wykorzystującego wysokopoziomowe i niskopoziomowe funkcje API POSIX do zarządzania tym systemem plików. Pamiętaj o poprawnym zwalnianiu zasobów (zamknięcie katalogów, zwolnienie pamięci), rzetelnym obsłużeniu błędów funkcji systemowych (w tym weryfikacji zmiennej errno przy wywołaniach readdir) oraz zachowaniu dobrych praktyk programistycznych.

## Etapy zadania

### Etap 1: Parsowanie metadanych składnika

Na tym etapie program przyjmuje jako pierwszy argument ścieżkę do pliku tekstowego reprezentującego metadane pojedynczego składnika. Plik ten zapisany jest w prostym formacie – w każdej linii występuje dokładnie jedna para w formacie klucz:wartość (możesz założyć brak nadmiarowych dwukropków w linii, jednak inne nieprawidłowości należy odpowiednio obsłużyć).

Odczytaj zawartość pliku, a następnie wypisz na standardowe wyjście wartości pól w następującej kolejności:

* name
* type
* rarity

W przypadku braku któregos z pól wypisz dla niego komunikat missing!.

Przykład pliku z metadadanymi:

```text
origin:Dolina Mgieł
name:Korzeń Mandragory
rarity:legendary
type:botanical
incipit:Rare root harvested during full moon
```

Oczekiwane wyjście:

```text
name: Korzeń Mandragory
type: botanical
rarity: legendary
```

(Podpowiedź: Biblioteka standardowa oferuje przydatne funkcje, takie jak getline, strchr czy strcmp. Najlepiej umieścić logikę parsowania w osobnej funkcji, gdyż przyda się ona w kolejnych etapach).

### Etap 2: Skanowanie magazynu i spłaszczanie struktury

Magazyn alchemika odzwierciedla fizyczną strukturę komnat, skrzyń i półek w postaci zagnieżdżonych katalogów.

W tym etapie zmodyfikuj program tak, aby nie pobierał już argumentów z linii poleceń. Po uruchomieniu program ma rekurencyjnie przeszukać katalog główny o nazwie warehouse.
W katalogu roboczym programu stwórz nowy katalog o nazwie alchemist_index (jeśli katalog już istnieje, program powinien zwrócić błąd). Wewnątrz niego utwórz podkatalog by-name.

Umieść w katalogu by-name dowiązania symboliczne (względne) do wszystkich zwykłych plików (reprezentujących składniki) znajdujących się w całym drzewie katalogu warehouse, zachowując ich oryginalne nazwy. Jeżeli nazwa jakiegoś pliku się powtarza w różnych podkatalogach magazynu, program powinien zgłosić błąd.

### Etap 3: Tworzenie indeksów wg właściwości

W katalogu alchemist_index (utworzonym w Etapie 2) utwórz dwa dodatkowe indeksy obok by-name:

by-rarity: Tworzy strukturę podfolderów postaci alchemist_index/by-rarity/<rarity>, gdzie <rarity> to wartość pola rarity odczytanego z metadanych danego składnika. Wewnątrz odpowiedniego podfolderu umieść dowiązanie symboliczne do pliku składnika, którego nazwą jest wartość pola name z metadanych (jeśli tytuł/nazwa jest dłuższa niż 64 bajty, należy ją przyciąć do tej długości). Jeżeli pole rarity nie występuje w metadanych, plik należy pominąć.

by-type: Działa analogicznie do powyższego, tworząc strukturę alchemist_index/by-type/<type> na podstawie pola type, a dowiązania noszą nazwy będące wartością pola name (przyciętą w razie potrzeby do 64 bajtów).

### Etap 4: Weryfikacja stanu ekwipunku z rejestrem binarnym

Ostatnią funkcjonalnością programu jest automatyczna weryfikacja, czy w magazynie nie brakuje żadnych składników ani czy ich rozmiary się zgadzają.

Gdy program zostanie uruchomiony z dodatkowym argumentem będącym ścieżką do pliku bazy danych (np. database_inventory), należy wczytać jego zawartość i porównać z faktycznym stanem magazynu (możesz w tym celu wykorzystać stworzony wcześniej indeks by-title/by-name).

Format bazy danych:
Baza składa się z pewnej liczby wpisów o stałym rozmiarze 68 bajtów każdy:

* 4 bajty: rozmiar pliku składnika (unsigned int),

* 64 bajty: nazwa składnika (name z metadanych; w przypadku, gdy nazwa jest krótsza niż 64 bajty, reszta pola jest wypełniona zerami).

Przejrzyj zawartość magazynu i dla każdego wpisu z bazy zweryfikuj stan:

Jeśli dany składnik nie istnieje w magazynie, wypisz: Ingredient "<name>" is missing

Jeśli plik istnieje, ale jego rzeczywisty rozmiar w systemie plików nie zgadza się z rozmiarem zapisanym w bazie, wypisz: Ingredient "<name>" size mismatch (<expected size> vs <actual size>)

Do odczytu bazy danych wykorzystaj wektorowe we/wy (readv), natomiast do analizy systemu plików użyj funkcji takich jak opendir, readdir, closedir, stat, oraz operacji na dowiązaniach symbolicznych poznanych na laboratoriach L1.
