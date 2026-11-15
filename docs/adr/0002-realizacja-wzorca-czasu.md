# 0002. Realizacja wzorca czasu

**Stan:** przyjęta (poziom 1) · provisional (poziom 2) · **Data:** 2026-11-15
**Dotyczy:** firmware aplikacji, konfiguracja TIM2 i CAN1

## Kontekst

Węzeł deklaruje zdolność `TIME_SOURCE` (ADR 0007 CanBusCommon). Wymagania
standardu zebrane są w [`WYMAGANIA_CZAS_2.1.md`](../WYMAGANIA_CZAS_2.1.md):
`SET_TIME_SYNC` nadawany tuż po zboczu PPS z jakością `FIX`/`PPS`/`HOLDOVER`,
milczenie bez wzorca, holdover, reguła wyboru źródła (poziom 1) oraz para
`TIME_SYNC` / `TIME_FOLLOW_UP` ze znacznikiem początku ramki (poziom 2).

Zasoby płytki: impuls PPS odbiornika na PA1 (TIM2_CH2), zdania NMEA po
USART2 (9600 bit/s), RTC MCP79410 z rozdzielczością 1 s, bxCAN z licznikiem
TTCM o rozdzielczości czasu bitu (2 µs przy 500 kbit/s). Rdzeń taktowany
z MSI 4 MHz w trybie PLL względem LSE (bypass z wyjścia MFP układu MCP79410).

## Rozważane warianty

| Wariant | Ocena |
|---|---|
| A. Czas z NMEA, bez PPS | odrzucony — opóźnienie zdania względem początku sekundy wynosi setki ms i zależy od długości zdań |
| B. PPS przez EXTI i znacznik z SysTick | odrzucony — rozdzielczość 1 ms, jitter obsługi przerwania |
| C. Przechwycenie PPS sprzętowo przez TIM2 (32 bit, 4 MHz) | przyjęty |

## Decyzja

1. **Podstawa czasu:** TIM2 liczy swobodnie z PCLK (4 MHz, 250 ns), CH2
   przechwytuje zbocze narastające PPS. Liczba taktów między kolejnymi
   zboczami jest bieżącą miarą częstotliwości zegara MCU (filtrowaną),
   używaną do przeliczeń w obrębie sekundy.
2. **Ważność wzorca:** `FIX` — ważne rozwiązanie czasu z RMC (status `A`)
   i rozwiązanie pozycji; `PPS` — dodatkowo co najmniej N kolejnych zboczy
   w odstępie 1 s ± tolerancja zgodnej z pomiarem częstotliwości.
   Przyporządkowanie zdania NMEA do zbocza (sekunda zbocza poprzedzającego
   czy następnego) ustalane jest pomiarem w E1 i zapisywane w uzupełnieniu
   tej decyzji.
3. **Poziom 1:** w obsłudze przechwycenia PPS (priorytet 0) zlecane jest
   nadanie `SET_TIME_SYNC` (`ARG` = bieżący `NODE_ID`, `MS` = czas od zbocza
   do zlecenia), jeśli upłynął `TIME_SYNC_PERIOD`. Po utracie wzorca węzeł
   nadaje z bitem `HOLDOVER` przez `TIME_HOLDOVER` sekund, licząc sekundy
   z podstawy TIM2, a następnie milknie. Reguła wyboru źródła i pola statusu
   według `docs/czas.md` CanBusCommon.
4. **RTC własny:** MCP79410 ustawiany na zboczu PPS przy ważnym wzorcu,
   ponownie co godzinę; trim oscylatora (rejestr OSCTRIM) wyznaczany
   z dryfu RTC względem PPS. RTC nie jest wzorcem dla magistrali.
5. **Poziom 2:** CAN1 w trybie TTCM (`TimeTriggeredMode` = ENABLE,
   `TransmitGlobalTime` = DISABLE — ramki na magistrali bez zmian).
   Znacznik SOF ramki `TIME_SYNC` (`TDTxR.TIME`) wiązany z TIM2
   w przerwaniu zakończenia nadania (priorytet 0): takt TIM2 minus długość
   ramki w bitach z bitami wypychania, z filtrem minimum na wielu ramkach.
   `TIME_FOLLOW_UP` nadawany nie później niż 100 ms po `TIME_SYNC`.
   Okres pary: parametr własny `PRECISE_SYNC_PERIOD` (0 = wyłączony).
6. **Priorytety przerwań:** CAN1_TX, CAN1_RX0 i TIM2 — 0; obsługa NMEA
   i I2C poza ścieżką czasową (priorytety 2–3).

## Konsekwencje

- Dokładność poziomu 1 ograniczona jest do milisekund (czas od zbocza do
  zlecenia nadania i arbitraż), poziomu 2 — do pojedynczych czasów bitu,
  pod warunkiem potwierdzenia pomiarem (cel: p99 ≤ 50 µs).
- Nadawanie `SET_TIME_SYNC` jest możliwe dopiero po zaimplementowaniu
  warstwy wspólnej (E2) i GNSS (E3); do tego czasu węzeł milczy jako
  źródło, co jest zachowaniem zgodnym ze standardem.
- Wynik pomiaru poziomu 2 przekazywany jest do CanBusCommon jako
  uzupełnienie ADR 0007.
