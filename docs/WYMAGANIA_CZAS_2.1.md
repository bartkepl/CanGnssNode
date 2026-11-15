# Wzorzec czasu — wymagania standardu 2.1

**Źródło:** CanBusCommon rewizja 2.1 —
[ADR 0007](https://github.com/bartkepl/CanBusCommon/blob/main/docs/adr/0007-wspolna-os-czasu.md),
[`docs/czas.md`](https://github.com/bartkepl/CanBusCommon/blob/main/docs/czas.md),
[ADR 0006](https://github.com/bartkepl/CanBusCommon/blob/main/docs/adr/0006-bootloader-wylacznie-can.md).
**Data:** 2026-11-15 · **Charakter:** materiał wejściowy do `docs/PLAN.md`
(etap E0); przy rozbieżności rozstrzyga `common/protocol/common.yaml`.

CanGnssNode jest pierwszym węzłem wzorcowym czasu na magistrali: rozgłasza
czas UTC z GNSS, zdyscyplinowany impulsem PPS, bez pośrednictwa hosta.

## 1. Deklaracja w `protocol/device.yaml`

| Element | Wartość | Uwagi |
|---|---|---|
| `capabilities` | `TIME_SOURCE`, `TIME_SYNC`, `RTC`, `GNSS` (+ pozostałe wg implementacji) | `TIME_SYNC_PRECISE` **nie** — węzeł jest nadawcą, nie odbiorcą poziomu 2 |
| `universal_params_implemented` | `TIME_SYNC_PERIOD`, `TIME_HOLDOVER` | obowiązkowe przy `TIME_SOURCE` — walidator to wymusza |
| parametr własny | np. `PRECISE_SYNC_PERIOD` (u16, ms, 0 = wyłączony, domyślnie 1000) | poziom 2 jest provisional; nastawa 0 wyłącza go bez zmiany firmware |
| komenda własna | `GET_GNSS_TIME` (0x80+) | zostaje — źródło diagnostyczne dla hosta |

## 2. Poziom 1 — normatywny

Wymagania standardu (`docs/czas.md#wzorzec-czasu`):

1. `SET_TIME_SYNC` co `TIME_SYNC_PERIOD` sekund (domyślnie 10; 0 = wzorzec
   wyłączony): `REQUEST`, `NODE = 0xFF`, `OBJ = 0x15`, **`ARG` = własny
   adres węzła** (`MPC_NODE_ID` bieżący, nie adres bootloadera).
2. Ładunek 7 B (`MPC_TIME_SYNC_LEN`): `UNIX` u32, `MS` u16, `QUALITY` u8.
   `QUALITY`: `FIX` gdy rozwiązanie czasu jest ważne, `PPS` gdy czas jest
   zdyscyplinowany impulsem, `HOLDOVER` po utracie wzorca.
3. Nadawanie **tuż po zboczu PPS**, `UNIX` = sekunda, którą to zbocze
   rozpoczyna, `MS` = czas od zbocza do zlecenia nadania (zwykle 0–2 ms).
   Węzły MPSWP i MPCC (rewizja 2.0) biorą tylko sekundy — ustawią zegar na
   początku właściwej sekundy.
4. **Bez wzorca od startu — milczenie.** RTC MCP79410 bez dyscypliny nie
   jest wzorcem.
5. Utrata wzorca: nadawanie z `HOLDOVER` przez `TIME_HOLDOVER` sekund
   (domyślnie 600), potem milczenie aż do odzyskania fixa.
6. Reguła wyboru źródła: słysząc przez dwa własne okresy `SET_TIME_SYNC`
   od źródła lepszego (wyższa jakość, przy równej — niższy adres), węzeł
   milknie. Ramki hosta (`ARG` 0xFE albo 0x00, `QUALITY` 0) przegrywają
   z węzłem mającym `FIX` — węzeł ich nie uwzględnia przy wyborze.
7. Węzeł bez fixa **może** przyjmować `SET_TIME_SYNC` od innego źródła, żeby
   utrzymać własny RTC, ale nie nadaje wtedy jako źródło.
8. Status (`GET_STATUS_EXT`, 13 B): `TIME_SOURCE` = źródło własnego zegara —
   dla wzorca z fixem własny adres, `TIME_AGE_S` od ostatniej dyscypliny
   PPS; `HEARTBEAT.FLAGS.TIME_SYNCED` ustawiony przy ważnym wzorcu.
   **Pola wypełniane jawnie** — nie zera z bufora (`CMD-016` testera).

## 3. Poziom 2 — provisional

Numery i układy zarezerwowane, do zatwierdzenia pomiarem:

1. Co `PRECISE_SYNC_PERIOD` ms (przy ważnym `PPS`): zdarzenie `TIME_SYNC`
   (`EVENT`, `NODE` = węzeł, `OBJ` = `SYSTEM`, `ARG` = 0x20), ładunek
   `SEQ` u8 + `QUALITY` u8.
2. Po nadaniu: odczyt znacznika SOF tej ramki (licznik TTCM, `TDTxR.TIME`),
   przeliczenie na UTC, zdarzenie `TIME_FOLLOW_UP` (`ARG` = 0x21): `SEQ`,
   `UNIX` u32, `USEC` u24 (0–999 999) — **nie później niż 100 ms** po
   `TIME_SYNC`.
3. `SEQ` rośnie o 1 na parę i jest wspólny dla obu ramek pary.

## 4. Realizacja — wskazówki do etapu E0/E2

| Element | Rozwiązanie | Do potwierdzenia |
|---|---|---|
| **PPS — poprawka `.ioc`** | PA1 (`GPS_PPS`) jest dziś ustawiony jako **`GPIO_Output`** — musi być wejściem. Docelowo przechwytywanie timera: PA1 = TIM2_CH2 (AF1), Input Capture, zbocze narastające | przypisanie AF w CubeMX; polaryzacja i szerokość impulsu PPS L76-L w dokumentacji modułu |
| Podstawa czasu | TIM2 (32-bit) swobodnie liczący; przechwycenie zbocza PPS, pomiar liczby ticków na sekundę (ciągła kalibracja zegara MCU) | częstotliwość taktowania TIM2 w konfiguracji zegarów |
| Sekunda UTC | NMEA z L76-L po USART2 (RMC/ZDA, 9600 8N1). **Przyporządkowanie zdania do zbocza PPS** (czy zdanie opisuje sekundę zbocza poprzedzającego, czy następnego) ustalić pomiarem — od tego zależy `UNIX` = sekunda albo sekunda + 1 | pomiar przy bring-upie: analizator na TX L76-L i PPS |
| Ważność | fix z RMC (`A`) i typ rozwiązania; `PPS` dopiero po kilku kolejnych zboczach w odstępie 1 s ± tolerancja | — |
| Znacznik SOF (poziom 2) | bxCAN w trybie TTCM (`hcan1.Init.TimeTriggeredMode = ENABLE`): licznik w czasie bitu (2 µs przy 500 kbit/s), przechwycenie SOF przy nadaniu. Bieżącej wartości licznika nie da się odczytać — powiązanie z TIM2 w przerwaniu „nadanie zakończone”: tick TIM2 minus długość ramki w bitach z bitami wypychania, z filtrem minimum na wielu ramkach | wpływ TTCM na pozostałą obsługę CAN (TTCM nie zmienia ramek na magistrali, dopóki `TGT` = 0) |
| Priorytety przerwań | CAN TX/RX i TIM2 CC najwyższe; USART2 (NMEA) i I2C niżej | — |
| RTC własny | MCP79410 ustawiany na zboczu PPS, przy ważnym fixie co godzinę | czy zapis rejestru sekund zeruje dzielnik sekundy |
| Bootloader | wspólne źródła tylko CAN (ADR 0006): `bl_core_run(&hcan1)`, bez USART w projekcie BL, **bez zaślepki** `bl_uart_none.c`; `BL_NODE_ID 0x20`, `BL_DEVICE_TYPE 0x0003` | — |

## 5. Kryteria odbioru

| Kryterium | Narzędzie |
|---|---|
| `SYN-001`, `SYN-002`, `SYN-004`, `SYN-005`, `CMD-010`, `CMD-016` — PASS | `canconf run --node 0x20 --observe 30` przy `TIME_SYNC_PERIOD` ≤ 10 s |
| MPSWP i MPCC po synchronizacji od węzła: `GET_TIME` zgodny z czasem hosta (NTP) do 1 s | rejestrator / GUI |
| Poziom 2: odchyłka odbiorcy testowego od PPS ≤ 50 µs (p99) | analizator logiczny: PPS węzła i impuls odbiorcy w obliczonej pełnej sekundzie |
| Bez fixa: brak `SET_TIME_SYNC` z adresem węzła | `canconf` (SYN-001 FAIL byłby tu oczekiwany — kontrolować ręcznie) albo rejestrator |

Wynik pomiaru poziomu 2 trafia do CanBusCommon jako uzupełnienie ADR 0007
i zdejmuje status provisional.
