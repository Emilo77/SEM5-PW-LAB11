# Laboratorium - współbieżność w systemie operacyjnym: sygnały
## Sygnały

Sygnały są prymitywną metodą komunikacji międzyprocesowej. W tym scenariuszu opiszemy sygnały zgodne ze standardem POSIX w środowisku Linux, w jednowątkowych procesach. Przesyłanie sygnałów między procesami wielowątkowymi i w innych systemach (częściowo) zgodnych z POSIX odbywa się w podobny sposób, ale występują pewne subtelności, które zostały pominięte w tym scenariuszu, np. który z wątków obsługuje nadesłany sygnał.

## Standardowe sygnały

Proces wysyła sygnał wywołaniem systemowym [kill](http://man7.org/linux/man-pages/man2/kill.2.html), wskazując docelowy proces i numer wysyłanego sygnału. Jądro systemu rejestruje informacje o sygnale i oznacza go u adresata jako nieobsłużony (*pending*). Wysyłający na tym kończy interakcję – nie czeka i nie dostaje od adresata informacji zwrotnej. Adresat sygnału przy najbliższej możliwej okazji (np. kiedy zostanie mu przydzielony kwant procesora lub kiedy wróci z wykonywania wywołania systemowego) zaczyna obsługę sygnału oznaczonego jako nieobsłużony i niezablokowany (o blokowaniu niżej).

Obsługa sygnału może polegać na:

- zignorowaniu go,
- natychmiastowym zakończeniu programu,
- wstrzymaniu (stop)/wznowieniu wykonywania programu (tylko dla niektórych numerów sygnałów),
- lub, co najważniejsze, wykonaniu wybranej przez proces funkcji.

Domyślna obsługa większości numerów sygnałów to natychmiastowe zakończenie programu (stąd też nazwa syscall'u `kill`). Najczęściej sygnał obsługuje się własną funkcją żeby pozwolić na łagodne zakończenie: np. sprzątanie tymczasowych plików, dokończenie operacji by zachować spójny stan, zakończenie podprocesów, wypisanie informacji o przerwanej operacji lub końcowych logów. Nie są to jednak jedyne zastosowania: sygnały można wysyłać by powiadomić o dowolnej sytuacji (również do synchronizacji między procesami) lub poprosić o np. wypisanie raportu.

## Numery standardowych sygnałów

Sygnał ma numer – który jest dowolną stałą zdefiniowaną w nagłówku [<signal.h>](https://man7.org/linux/man-pages/man0/signal.h.0p.html) (typu `int`, niezerową). Praktycznie zawsze odnosimy się do numeru sygnału używając nazwy stałej, np. `SIGINT` jest numerem sygnału wysyłanym przez większość konsol po wciśnięciu `Ctrl+C`. Większosć numerów sygnałów jest wysyłana przez jądro systemu w określonych sytuacjach lub ma przypisane zwyczajowe znaczenie; są one wysyłane przez procesy w podobnych sytuacjach. Częściej spotykane sygnały to:

| Nazwa | Operacja |
|-------|----------|
|`SIGINT` | Przerwanie z klawiatury `Ctrl+C`. Często obsługiwany przez program jako np. łagodne przerwanie pojedynczej operacji, bez zakończenia całego programu. Interpreter np. Pythona zamienia ten sygnał na wyjątek. |
|`SIGTERM`| Prośba o zakończenie. Często obsługiwany przez program jako łagodne zakończenie. To domyślny sygnał wysyłany przez polecenia [kill](https://man7.org/linux/man-pages/man1/kill.1.html) i [pkill](https://man7.org/linux/man-pages/man1/pgrep.1.html) w konsoli. |
|`SIGKILL`| Bezwarunkowe zakończenie – obsługi tego sygnału nie można zmienić (nie pozwala na łagodne zakończenie, więc jest wysyłany tylko w ostateczności). |
|`SIGQUIT`| Zakończenie wraz ze zrzutem pamięci. Można wywołać z klawiatury za pomocą `Ctrl + \`. |
|`SIGCHLD`| Wysyłany przez jądro do procesu-rodzica by powiadomić go gdy proces-dziecko zakończy/wstrzyma/wznowi wykonywanie. (Jeden z niewielu sygnałów którego domyślna obsługa polega na zignorowaniu). |
|`SIGTSTP`| Wstrzymanie wykonywania programu (z terminala). W konsoli wywoływany wciśnięciem `Ctrl+Z`. Program można potem wznowić poleceniem [fg](https://man7.org/linux/man-pages/man1/fg.1p.html) lub wznowić w tle poleceniem [bg](https://man7.org/linux/man-pages/man1/bg.1p.html) (a wykonywany w tle można przywrócić do terminala także poleceniem `fg`). |
|`SIGSEGV`| Błędny dostęp do pamięci ([segfault](https://en.wikipedia.org/wiki/Segmentation_fault)). Można zdefiniować własną obsługę, ale musi ona zakończyć program. |
|`SIGUSR1`, `SIGUSR2`| Sygnały bez określonego znaczenia – do własnych celów najlepiej stosować właśnie te. |

Pełną listę można znaleźć w [man 7 signal](https://man7.org/linux/man-pages/man7/signal.7.html) lub poleceniem `kill -l`. Więcej informacji o zwyczajowych użyciach także na [Wikipedii](https://en.wikipedia.org/wiki/Signal_(IPC)#POSIX_signals).

Wypróbuj w konsoli wstrzymanie programu (np. `top`) przy użyciu `Ctrl+Z`, `bg`, `fg`.

## Wysyłanie sygnałów

Do wysłania sygnału do innego procesu służy funkcja `kill` w nagłówku `<signal.h>` (wołająca `syscall` o tej samej nazwie):
```c
int kill(pid_t pid, int sig);
```
gdzie

- `pid` to identyfikator procesu do którego chcemy wysłać sygnał,
- `sig` to numer sygnału.

Funkcja zwraca `0` jeśli się powiedzie, a `-1` w przeciwnym wypadku, ustawiając `errno` na:

- `EINVAL` – jeśli numer sygnału jest niewłaściwy,
- `ESRCH` – jeśli adresat nie istnieje,
- `EPERM` – jeśli brakuje uprawnień do wysłania sygnału (można wysłać sygnał tylko do procesu tego samego użytkownika, z paroma wyjątkami).

Sygnał można też wysłać z terminala poleceniem `kill`:
```bash
kill -SIGUSR1 $PID
```
Kiedy nie znamy identyfikatora `pid` procesu można użyć `pgrep` i `pkill`.

## Zmiana obsługi sygnałów

Do zmiany obsługi sygnału służy funkcja [sigaction](https://man7.org/linux/man-pages/man2/sigaction.2.html) oraz typ [struct sigaction](https://man7.org/linux/man-pages/man2/sigaction.2.html) zdefiniowane w [<signal.h>](https://man7.org/linux/man-pages/man0/signal.h.0p.html).

(Drobna uwaga o tej kolizji nazw: w `C` nazwy typów strukturalnych trzeba wszędzie poprzedzać słowem `struct`, np. `struct sigaction`, o ile nie zarezerwujemy zwykłej nazwy używając `typedef`. W `C++` nie jest to normalnie potrzebne, ale dla kompatybilności można też dopisać `struct` przed nazwą typu, co pozwala rozróżnić funkcję `sigaction` od `struktury sigaction`).

W funkcji
```c
int sigaction(int signum, const struct sigaction* act, struct sigaction* oldact);
```
- `signum` to numer sygnału, którego obsługę chcemy zmienić lub sprawdzić;
- `act` to nowy sposób obsługi (można podać `NULL` żeby go nie zmieniać).
- `oldact` to miejsce na zachowanie dotychczasowego sposobu obsługi sygnału (można podać `NULL` by go nie zapisywać).

W strukturze struct `sigaction dokładna` definicja może zależeć od systemu i architektury, ale zawiera co najmniej następujące pola, których szczegóły omówimy później:

- `sa_handler` – prosty handler (funkcja wykonywana przy obsłudze sygnału),
- `sa_sigaction` – rozszerzony handler (przyjmuje dodatkowe argumenty),
- `sa_mask` – zbiór sygnałów dodatkowo blokowanych na czas obsługi sygnału,
- `sa_flags` – zbiór flag zmieniających różne szczegóły.

O ile pole `sa_flags` jest zwykła maską bitową (tzn. należy podać bitowy *OR* żądanych flag), o tyle pole `sa_mask` zależy od systemu i architektury (bo liczba dostępnych sygnałów istotnie się zwiększała w historii). Należy je zainicjalizować funkcją [sigemptyset](https://man7.org/linux/man-pages/man3/sigemptyset.3p.html).
```c
struct sigaction new_action;
sigemptyset(&new_action.sa_mask);
new_action.sa_flags = 0;
```        

Do pola `sa_handler` można przypisać:

- stałą `SIG_IGN` – sygnał jest wtedy ignorowany,
- stałą `SIG_DFL` – przywraca domyślną obsługę tego sygnału (opisana w tabeli w [man 7 signal](https://man7.org/linux/man-pages/man7/signal.7.html)), albo
- wskaźnik na funkcję o deklaracji `void foo(int signo);`

#### Przykład: 
Spróbuj wykonać [simple_handler.c](simple_handler.c), w którym obsługa `SIGINT` jest zmieniona na parę sekund.

## Bezpieczna obsługa sygnałów

Ponieważ funkcje obsługi sygnałów mogą być wykonywane w dowolnej chwili, nie należy przy ich pisaniu przyjmować jakichkolwiek założeń co do tego, co w chwili ich wywołania robi program. Obsługa sygnału może się zacząć w momencie kiedy program jest w trakcie inkrementacji wskaźnika na bufor (np. w `printf`) lub zmieniania listy wolnych bloków pamięci (np. w `malloc`), więc wywołanie takich funkcji w handlerze mogłoby doprowadzić do rozspójnienia.

Co gorsza główny kod programu nie będzie kontynuował aż do zakończenia handler'a, więc próba ominięcia tego problemu mutexem doprowadzi do zakleszczenia. Z tego powodu lista funkcji, które można bezpiecznie wywołać w handlerze jest stosunkowo krótka: w szczególności niebezpieczna jest wszelka dynamiczna alokacja pamięci oraz wszystkie standardowe funkcje formatowania napisów (niestety nawet `snprintf`, gdyż implementacja tworzy tymczasowe bufory). Stąd w powyższym przykładzie użyliśmy bezpośrednio wywołania systemowego [write](https://man7.org/linux/man-pages/man2/write.2.html). Pełną listę bezpiecznych funkcji ze standardu POSIX można znaleźć w [man signal-safety](https://man7.org/linux/man-pages/man7/signal-safety.7.html).

Ponadto kompilator i procesor mogą dokonywać optymalizacji, które zakładają że globalne zmienne nie zmieniają się nagle między dowolnymi dwoma instrukcjami. Dlatego standard **C11** specyfikuje, że jedyne typy zmiennych globalnych (i `thread-local`), których można bezpiecznie używać w obsłudze sygnałów to typy `atomic lock-free` (tj. takie, których wewnętrzna implementacja nie używa czekania do zapewnienia atomowości). Na większości architektur takimi typem jest `atomic_int` (zdefiniowany w [<stdatomic.h>](https://en.cppreference.com/w/c/atomic) w `C` lub [<atomic>](https://en.cppreference.com/w/cpp/header/atomic) w `C++`). Dla kompatybilności wstecznej standard określa, że bezpieczny jest także typ `volatile sig_atomic_t` (typ ten zapewnia, że odczyty jak i zapisy są niepodzielne, oraz że nie można ich pominąć w optymalizacji, natomiast w przeciwieństwie do `atomic_int` nie zapewnia niepodzielności innych operacji, np. inkrementacji).

Dlatego w większości przypadków własna obsługa sygnału sprowadza się do jednej z trzech metod:

- Przypisaniu w handlerze do globalnej zmiennej typu np. `atomic_int`. Główny kod programu okresowo sprawdza tę zmienną, tak jak robiliśmy z flagą `interrupted` w `Java`.
- Utworzeniu wątku, którego jedynym zadaniem jest czekanie na sygnały i ich obsługa (zazwyczaj nadal potrzebujemy jakoś powiadomić główny wątek o sygnale, ale różnica jest taka, że główny kod może kontynuować współbieżne wykonanie, więc dostępne są np. mutexy i wszelkie funkcje `thread-safe`).
- W programach, które w pętli czekają na zlecenie i je obsługują: można do listy rzeczy, na które nasłuchujemy (łącza *pipe*, *sockety* sieciowe, itp.) dodać sygnały (np. za pomocą [signalfd](https://man7.org/linux/man-pages/man2/signalfd.2.html)).

## Blokowanie sygnałów

Proces może określić, które numery sygnałów chce blokować. Sygnały są wtedy dalej rejestrowane przez system, jako nieobsłużone (*pending*), ale ich obsługa jest odraczana do momentu zniesienia blokady. Pozwala to np. definiować struktury danych do użycia w obsłudze sygnałów, dla których chcemy zapewnić, że w danej chwili tylko jeden fragment kodu na nich operuje.

Zbiór sygnałów (np. blokowanych czy nieobsłużonych) jest trzymany w zmiennej typu [sigset_t](https://man7.org/linux/man-pages/man3/sigfillset.3.html), na której mamy zdefiniowane operacje:

```c
int sigemptyset(sigset_t *set);
/* set staje się pustym zbiorem */

int sigfillset(sigset_t *set);
/* set staje się zbiorem wszystkich sygnałów */

int sigaddset(sigset_t *set, int signum);
/* dodaje signum do zbioru */

int sigdelset(sigset_t *set, int signum);
/* usuwa signum ze zbioru */

int sigismember(const sigset_t *set, int signum);
/* czy signum jest w zbiorze? */
```

**Dokumentacja** tych funkcji znajduje się [tutaj](https://man7.org/linux/man-pages/man3/sigfillset.3.html)

Funkcje te zwracają `-1` w przypadku błędnego numeru sygnału; w przeciwnym przypadku pierwsze cztery zawsze zwracają `0`, natomiast sigismember zwraca 1 jeśli signum jest w zbiorze i 0 jeśli nie ma.

Zbiór obecnie blokowanych przez proces sygnałów (tzw. *signal mask procesu*) można sprawdzić i zmienić funkcją [sigprocmask](http://man7.org/linux/man-pages/man2/sigprocmask.2.html#DESCRIPTION):
```c
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
```
gdzie możliwe wartości `how` to:

- `SIG_BLOCK` – dodanie set do maski
- `SIG_UNBLOCK` – usunięcie set z maski
- `SIG_SETMASK` – ustawienie maski na `set`.

Można podać `NULL` pod `set` jeśli nie chcemy nic zmieniać, a pod `oldset` jeśli nie chcemy zapisywać poprzedniej maski.

Oczekujące, nieobsłużone sygnały możemy poznać funkcją [sigpending](http://man7.org/linux/man-pages/man2/sigpending.2.html).

Najczęstszym problemem jest jednak możliwość otrzymania sygnału podczas obsługi innego sygnału. Dlatego określając obsługę sygnału w strukturze `sigaction` możemy określić w polu `sa_mask` zbiór sygnałów, które będą dodane do blokowanych na czas wykonywania sygnałów. Ponadto, na czas obsługi sygnału blokowany jest numer tego sygnału, chyba że podamy flagę `SA_NODEFER`.

Próby blokowania `SIGKILL` i `SIGSTOP` są ignorowane; pojawienie się sygnałów błędów sprzętowych (`SIGSEGV`, `SIGFPE`, `SIGILL`, `SIGBUS`) podczas ich zablokowania jest niezdefiniowane (nie można też na nie czekać).

#### Przykład
W przykładzie [signal_mask.c](signal_mask.c) sygnał `SIGINT` jest zablokowany na ok. 5 sekund. Po tym czasie odblokuje ten sygnał i poinformuje czy podczas blokady ten sygnał został przesłany.

## Oczekiwanie na sygnał

Proces (lub wątek) może czekać na wskazany zbiór sygnałów za pomocą funkcji [sigwait](https://man7.org/linux/man-pages/man3/sigwait.3.html), [sigwaitinfo](https://man7.org/linux/man-pages/man2/sigwaitinfo.2.html) (która zapisuje dodatkowe informacje o sygnale), lub [sigtimedwait](https://man7.org/linux/man-pages/man2/sigwaitinfo.2.html) (z timeout'em). Czekają na pierwszy sygnał ze wskazanego zbioru i zdejmują go ze zbioru oczekujących (jeśli przy wywołaniu oczekiwał już sygnał ze zbioru, funkcje te zwrócą natychmiast).

Jeśli chcemy w ten sposób oczekiwać na sygnały w pętli, należy pamiętać o zablokowaniu ich przed pętlą, żeby sygnały były zablokowane między dwoma wywołaniami `sigwait`/ `sigwaitinfo`/ `sigtimedwait`.

Szczegółowe informacje o sygnale są zapisywane przez `sigwaitinfo` / `sigtimedwait` do struktury typu `siginfo_t`. Jest to taka sama struktura jak otrzymana przez rozszerzony handler sygnałów, opisany niżej.

## Rozszerzony handler sygnałów

Aby przy obsłudze sygnału otrzymać więcej informacji można w strukturze [struct sigaction](https://man7.org/linux/man-pages/man2/sigaction.2.html) wśród flag podać `SA_SIGINFO`, zaś do pola sa_sigaction przypisać funkcję o deklaracji 
```c
void foo(int signo, siginfo_t* sa_info, void* sa_data);
```
gdzie
- `signo` to numer sygnału,
- `sa_info` do dodatkowa, rozbudowana, struktura [siginfo_t](http://man7.org/linux/man-pages/man2/sigaction.2.html) zawierająca pomocnicze informacje; w naszym przypadku (sygnały wysłane funkcją `kill`, a nie przez system) interesujące pola tej struktury to `pid_t`, `si_pid` oraz `uid_t`, `si_uid` – identyfikator procesu i użytkownika do którego należał proces wysyłający sygnał.
- `sa_context` to techniczny kontekst wywołania, dla nas nieinteresujący.

**Uwaga!**: należy albo przypisać pole `sa_handler`, albo podać flagę `SA_SIGINFO` oraz przypisać pole `sa_sigaction` (pola `sa_handler` i `sa_sigaction` mogą na siebie nachodzić).

#### Przykład
Przeanalizuj przykład [more_info.c](more_info.c).

## Inne flagi

W polu `sa_flags` struktury `struct sigaction` można podać różne flagi, specifykujące szczegóły sygnałów o jakimś numerze. Interesujące flagi (które w większości już poznaliśmy) to m.in.:
| nazwa | polecenie |
|-------|-----------|
| `SA_NODEFER` | nie blokuj tego sygnału podczas obsługi |
| `SA_SIGINFO` | wykorzystaj `sa_sigaction` zamiast `sa_handler` do obsługi sygnału |
| `SA_RESETHAND` | po jednorazowej obsłudze sygnału, przywróć domyślny sposób obsługi |
| `SA_RESTART` | jeśli sygnał przerwie blokujące wywołanie funkcji (np. `read`), wznów funkcję po obsłudze sygnału (**uwaga!**: dokładne zachowanie może zależeć od implementacji) |
| `SA_NOCHLDWAIT` | nie zamieniaj dzieci w `zombie` (do czasu zaczekania na nie przez rodzica; flaga ma znaczenie tylko przy zmianie obsługi sygnału `SIGCHLD`) |

Do zmiany obsługi sygnału historycznie służyła funkcja `signal`, opisana nawet w standardzie `C`. Niestety jej dokładna semantyka zależy od systemu: w zależności od wybranych flag kompilatora może działać tak jak `sigaction` z różnymi podzbiorami flag `SA_RESETHAND`, `SA_RESTART`, `SA_NODEFER`.

## Przerywanie funkcji systemowych – `EINTR`

W przypadku otrzymania obsługiwanego sygnału podczas gdy funkcja systemowa czeka (jest *blocked* w sensie czekania na `I/O` czy inne zdarzenie), mogą stać się trzy rzeczy:

- funkcja kontynuuje działanie i sygnał zostanie dopiero potem obsłużony;
- funkcja przerywa czekanie, sygnał zostaje obsłużony, po czym funkcja zwraca z błędem `EINTR` – proste, ale niewygodne;
- funkcja przerywa czekanie, sygnał zostaje obsłużony, po czym funkcja restartuje - jest automatycznie wznawiana.

Szczegóły zależą od systemu, ale pod Linuxem są następujące.

Pierwsza opcja dotyczy tylko czekania na plik (lokalny dysk) – uznaje się że nie będzie czekać długo, więc jest traktowana w tym kontekście tak jak operacje nieblokujące. Druga opcja (zakończenie błędem `EINTR`) jest domyślnie stosowana dla wszystkich innych operacji. Trzecia opcja (automatyczny restart) jest stosowana jeśli przy zmianie obsługi danego numeru sygnału użyto flagi `SA_RESTART`. Dotyczy to większości operacji (odczyt/zapis z terminala/łącza/socketu, synchronizacje mutexem/semaforem, czekanie na dziecko), natomiast kilka rodzajów operacji nigdy nie restartuje: czekanie na czas (`sleep`), czekanie na sygnał, czekanie na socket sieciowy z określonym timeoutem, oraz czekanie na listy rzeczy (`poll` / `select`).

Ponadto `EINTR` może się pojawić dla niektórych operacji w przypadku gdy podczas oczekiwania proces został sygnałem wstrzymany i następnie wznowiony. Szczegóły w [man 7 signal](https://man7.org/linux/man-pages/man7/signal.7.html).

## Standard vs Real-time

**Materiał opcjonalny**

Proces informacje o nadesłanych standardowych sygnałach przechowuje w postaci zbioru, co oznacza, że proces nie wie czy sygnał, ktory wciąż oczekuje na obsługę, został wysłany raz czy wiele razy, przez jednego czy przez wielu nadawców. Konsekwencją tego sposobu przechowywania danych jest to, że obsługa wielokrotnie nadesłanego sygnału może zostać uruchomiona tylko raz. Co więcej, kolejność obsługi tych sygnałów nie jest zdefiniowana.

Dlatego poza standardowymi sygnałami istnieją obecnie także tzw. sygnały czasu rzeczywistego (ang. *real-time signals*, albo *rt signals*). Są to sygnały o numerach w zakresie od `SIGRTMIN` do `SIGRTMAX` – nie mają poza tym przypisanych nazw ani standardowych znaczeń (są traktowane jako sygnały definiowanych przez użytkownika).

Sygnały te różnią się od poprzedników w trzech aspektach:

- każda instancja sygnału jest rejestrowana i przechowywana w kolejce (nie w zbiorze),
- wraz z sygnałem można przesłać dodatkową informację (int lub wskaźnik),
- kolejność obsługi sygnałów jest zdefiniowana: niższe numery sygnałów mają priorytet, w razie współdzielenia numeru wcześniejsza instancja sygnału ma priorytet.

Do ich wysyłania służy funkcja [sigqueue](https://man7.org/linux/man-pages/man3/sigqueue.3.html) (zamiast `kill`). Implementacja biblioteki `pthreads` wewnętrznie używa `real-time signals` (ale o numerach poza `SIGRTMIN–SIGRTMAX`).

#### Przykład
Możesz sprawdzić co zmieni się w programie [more_info.c](more_info.c) jeśli zmienisz definicję `MY_SIGNAL` na (`SIGRTMIN + 1`).

## Ćwiczenie punktowane

W przykładzie [terminator.c](terminator.c) uruchamiamy podproces w tle, po czym proces-rodzic w pętli czeka na sygnały i kończy jeśli dostanie dwa razy `SIGINT` (`Ctrl+C`) w ciągu sekundy.

Uzupełnij funkcję `parent()` aby po pierwszym `SIGINT` wysłać `SIGINT` do dziecka, a po podwójnym `SIGINT` wysłać `SIGKILL` do dziecka. Popraw kod aby proces-rodzic wychodził od razu z pętli także kiedy dziecko samo zakończy działanie (`SIGCHLD`). Nie zmieniaj `child()`.

W razie wypadków przy pracy warto pamiętać, że wciśnięcie `Ctrl + \` wysyła `SIGQUIT`.
