# Plan realizacji oprogramowania

**Stan na: 2026-11-15** · Dotyczy: firmware aplikacji i bootloadera węzła,
integracja z magistralą CanBusCommon (profil 2, rewizja 2.1).

Dokument określa zakres, założenia i kolejność prac nad oprogramowaniem
węzła. Decyzje projektowe zapisywane są jako ADR w
[`docs/adr/`](adr/README.md). Wymagania standardu dotyczące wzorca czasu
zebrane są w [`WYMAGANIA_CZAS_2.1.md`](WYMAGANIA_CZAS_2.1.md); przy
rozbieżności rozstrzyga `common/protocol/common.yaml`.

## 1. Zakres

Węzeł jest zgodny z profilem 2 standardu
[CanBusCommon](https://github.com/bartkepl/CanBusCommon) od pierwszej
wersji:

- podmoduł `common/`, opis urządzenia [`protocol/device.yaml`](../protocol/device.yaml),
  kod protokołu generowany (`common/protocol/generate.py`), zgodność
  sprawdzana walidatorem ekosystemu;
- wspólny bootloader CAN w wersji wire 1.1, wyłącznie z transportem CAN
  (ADR 0006 CanBusCommon);
- **wzorzec czasu magistrali** (`TIME_SOURCE`, ADR 0007 CanBusCommon):
  czas UTC z GNSS zdyscyplinowany impulsem PPS;
- baza firmware wspólna z węzłem CanGeigerProbe i oparta na sprawdzonym
  zestawie modułów MPCC (scheduler kooperacyjny, warstwa CAN z oczekiwaniem
  na miejsce w kolejce, NVM we flashu, bezpieczeństwo i autotest, IWDG,
  MCP79410, tor ADC z filtrem IQM i przeliczeniem ratiometrycznym względem
  VREFINT) — plan bazy: [CanGeigerProbe `docs/PLAN.md`](https://github.com/bartkepl/CanGeigerProbe/blob/main/docs/PLAN.md);
- warstwa specyficzna: odbiornik GNSS, IMU, wykrywanie sytuacji;
- odbiór warstwy wspólnej testerem zgodności `canconf`.

Numery z rejestrów wspólnych:

| Element | Wartość |
|---|---|
| `DEVICE_TYPE` | `0x0003` |
| Zakres adresów | `0x20`–`0x2F`, domyślny `0x20`; bootloader `0x20` |
| Czujniki | `GNSS` 0x20, `FXOS8700CQ` 0x21, `ORIENTATION` 0x23, `SUPPLY_MON` 0x11, `MCU` 0x0C |
| Wielkości nawigacyjne | 0x50–0x57 (`LATITUDE` … `FIX_TYPE`) |
| Orientacja i ruch | 0x60–0x69 (`ACCEL_*`, `MAG_*`, `PITCH`, `ROLL`, `HEADING_MAG`, `MOTION_STATE`) |
| Zasilanie | `SUPPLY_VOLTAGE` 0x12, `SUPPLY_CURRENT` 0x13 |
| Zdolności specyficzne | `GNSS` (bit 17), `TIME_SOURCE` (bit 20) |

## 2. Założenia

| Założenie | Konsekwencja |
|---|---|
| Węzeł pracuje na wspólnej magistrali z MPSWP i MPCC | Stały nasłuch, transceiver w trybie normalnym, bez Stop 2 — jak w planie bazy |
| Praca stacjonarna albo mobilna, zależnie od sytuacji | Stan ruchu rozpoznawany z IMU i GNSS; od niego zależy tryb odbiornika i uśrednianie pozycji ([ADR 0003](adr/0003-sytuacje-i-tryby-gnss.md)) |
| IMU służy do wykrywania sytuacji, jako kompas i do zdarzeń nagłych | Funkcje sprzętowe FXOS8700CQ (transient, freefall) na przerwaniu, niezależnie od GNSS |
| Antena pasywna ATIGG1540-1 na IPX, bez strojenia | Sieć dopasowania C11/C12 pozostaje DNP; C/N0 z `GET_SKY` służy ocenie położenia anteny w insercie |
| Magistrala 500 kbit/s | Jak pozostałe węzły |
| Jeden egzemplarz | Ewidencja przy odbiorze |

## 3. Mapa sprzętowa MCU

Przypisanie pinów odczytane z płytki (`pcb/gnss_node.kicad_pcb`, układ U1).

| Pin | Sygnał | Funkcja |
|---|---|---|
| PA1 | `GPS_PPS` | TIM2_CH2, przechwytywanie zbocza narastającego |
| PA2 / PA3 | `GPS_RX` / `GPS_TX` | USART2 ↔ L76-L (I/O modułu 2,8 V, dzielnik R47/R49) |
| PA5 | `GPS_RESET` | wyjście; stan wysoki = reset modułu przez Q4 |
| PA6 | `VIN_SENSE` | ADC1 IN11 — dzielnik 1 MΩ / 100 kΩ |
| PA7 | `I_SENS` | ADC1 IN12 — INA186A4 (G = 200, shunt 0,5 Ω) |
| PB0 | `EN_I_SENS` | wyjście — włączenie INA186 |
| PA9 | `CAN_STBY` | wyjście — ATA6561 STBY, stan niski = tryb normalny |
| PA10 | `CAN_R_CTRL` | wyjście — terminacja split (PhotoMOS), stan wysoki = załączona |
| PA11 / PA12 | `CAN_RX` / `CAN_TX` | bxCAN |
| PA15 | — | wejście połączone z `CAN_RX` przez 10 Ω (rezerwa) |
| PB4 | `IMU_EN` | wyjście — klucz zasilania IMU (Q5/Q6), R41 trzyma wyłączone |
| PB5 | `IMU_INT` | EXTI5 — INT1 i INT2 FXOS8700CQ przez R38/R39 |
| PB6 / PB7 | I2C1 | MCP79410 (0x6F, EEPROM 0x57), FXOS8700CQ (0x1F) |
| PC14 | `CLK` | MFP MCP79410 (32,768 kHz) — LSE w trybie bypass, podciągnięcie R18 |
| PA13 / PA14 / PB3 | SWD + SWO | Tag-Connect |
| PA0, PA4, PA8, PB1, PC15 | — | niepodłączone |

Właściwości sprzętu istotne dla firmware:

- **L76-L** ma zasilanie VCC bez klucza, a piny `STANDBY` i `FORCE_ON` są
  niepodłączone: dostępne są tryby Full On, Periodic, AlwaysLocate i Standby
  (PMTK161, wybudzenie bajtem na UART); trybu Backup nie ma. Moduł nie jest
  resetowany przy resecie MCU, więc rozwiązanie przeżywa aktualizację
  aplikacji — start aplikacji nie resetuje odbiornika.
- **BT1** (MS621FE) podtrzymuje jednocześnie V_BCKP odbiornika (efemerydy,
  szybki start) i VBAT układu MCP79410.
- **FXOS8700CQ**: wyjścia INT1 i INT2 są połączone przez rezystory na jedno
  wejście, więc oba pracują jako open-drain z podciąganiem w MCU
  ([ADR 0004](adr/0004-imu-przerwania-i-zasilanie.md)).

## 4. Opis urządzenia

Pełny opis: [`protocol/device.yaml`](../protocol/device.yaml). Zarys:

- **Telemetria (16 kanałów):** nawigacja 0x50–0x57 z jednej migawki
  rozwiązania ([ADR 0001](adr/0001-spojna-migawka-pozycji.md)), `PITCH`,
  `ROLL`, `HEADING_MAG`, `MOTION_STATE`, `SUPPLY_VOLTAGE`,
  `SUPPLY_CURRENT`, `MCU_TEMP`, `VDDA`. Surowe `ACCEL_*` i `MAG_*` przez
  `READ_SENSOR`.
- **Komendy własne:** `READ_FIX` (spójna migawka, także wysokość n.p.m.),
  `GET_GNSS_TIME`, `GET_SKY` (C/N0, DOP), `GNSS_RESTART` (bajt
  zabezpieczający 0xC5), `MAG_CAL`.
- **Zdarzenia własne:** `FIX_ACQUIRED`, `FIX_LOST`, `SITUATION_CHANGED`,
  `SHOCK`, `FREEFALL`, `TILT_ALARM`. Awaria czujnika — uniwersalne
  `SENSOR_FAULT` z identyfikatorem z rejestru.
- **Parametry własne:** aktywność GNSS i IMU, konstelacje, SBAS, polityka
  trybu odbiornika, progi ważności rozwiązania, uśrednianie w spoczynku,
  `PRECISE_SYNC_PERIOD`, progi IMU i stanu ruchu, deklinacja; kalibracyjne
  (`CAL_LOCK`): offsety hard-iron, orientacja montażu, tory VIN i prądu.

Wymagania warstwy wspólnej (`common/docs/integracja.md` §6) obowiązują jak
w planie bazy.

## 5. Wzorzec czasu

Wymagania: [`WYMAGANIA_CZAS_2.1.md`](WYMAGANIA_CZAS_2.1.md); realizacja:
[ADR 0002](adr/0002-realizacja-wzorca-czasu.md).

- **Poziom 1 (normatywny):** `SET_TIME_SYNC` z własnym adresem w `ARG`,
  7 B z `QUALITY`, nadawany tuż po zboczu PPS co `TIME_SYNC_PERIOD`;
  holdover przez `TIME_HOLDOVER`; milczenie bez wzorca; reguła wyboru
  źródła; pola `TIME_SOURCE` i `TIME_AGE_S` w `GET_STATUS_EXT`.
- **Poziom 2 (provisional):** para `TIME_SYNC` / `TIME_FOLLOW_UP` ze
  znacznikiem SOF z licznika TTCM, co `PRECISE_SYNC_PERIOD`. Wynik pomiaru
  odchyłki uzupełnia ADR 0007 CanBusCommon.
- Podstawa czasu: TIM2 (32 bit, 4 MHz) z przechwyceniem PPS i ciągłym
  pomiarem liczby taktów na sekundę. RTC MCP79410 ustawiany na zboczu PPS.

## 6. Strategia poboru prądu

Środki bazy (MSI 4 MHz, Range 2, `WFI`, ADC skanami, INA186 na czas pomiaru,
R18 do sprawdzenia) obowiązują bez zmian. Dominującym odbiornikiem jest L76-L
(kilkanaście–kilkadziesiąt mA w akwizycji i śledzeniu), dlatego zasadniczą
oszczędność daje polityka trybu odbiornika zależna od stanu ruchu
([ADR 0003](adr/0003-sytuacje-i-tryby-gnss.md)). Wyjścia MCU pracują bez
wewnętrznych podciągnięć (ok. 80 µA na pin w stanie wysokim); stany po
resecie zapewniają rezystory na płytce.

## 7. Etapy

| Etap | Zakres | Kryterium zakończenia |
|---|---|---|
| **E0** Specyfikacja | podmoduł `common/`; `device.yaml`, generacja i walidacja; projekty CubeIDE aplikacji i bootloadera; ten plan i ADR-y; parser NMEA z testami uruchamianymi na PC | walidator bez błędów, `generate.py --check` aktualne, testy parsera zaliczone |
| **E1** Uruchomienie sprzętu | jak w planie bazy, a ponadto: komunikacja z L76-L, PPS, `WHO_AM_I` FXOS8700CQ, zasilanie wsteczne IMU z linii I2C przy `IMU_EN` = 0, przyporządkowanie zdania NMEA do zbocza PPS | ramki na magistrali, aplikacja wgrana przez bootloader CAN |
| **E2** Warstwa wspólna | moduły bazy | `canconf` poziom `full`: 0 FAIL; obraz `app-v0.1` w `releases/` |
| **E3** GNSS | konfiguracja PMTK, telemetria nawigacyjna, `READ_FIX`, `GET_SKY`, zdarzenia rozwiązania, `SENSOR_FAULT` przy braku NMEA, pomiar TTFF | rozwiązanie w obudowie z insertem; CanSensorLoggerPi rejestruje bez zmian w rejestratorze |
| **E4** Wzorzec czasu, poziom 1 | podstawa czasu TIM2, dyscyplina RTC, `SET_TIME_SYNC`, holdover, wybór źródła, status | `canconf`: SYN-001, SYN-002, SYN-004, SYN-005, CMD-010, CMD-016 PASS; MPSWP i MPCC zgodne z czasem hosta do 1 s |
| **E4b** Wzorzec czasu, poziom 2 | TTCM, `TIME_SYNC` + `TIME_FOLLOW_UP`, pomiar analizatorem | odchyłka odbiorcy od PPS ≤ 50 µs (p99) |
| **E5** IMU | odczyty, orientacja montażu, kompas z kompensacją pochylenia, `MAG_CAL` | kurs magnetyczny zgodny z kursem GNSS w ruchu (po deklinacji) |
| **E6** Sytuacje i energia | klasyfikator stanu ruchu, funkcje sprzętowe FXOS, polityka trybów L76-L, tryb rejestracji surowych danych | zdarzenia potwierdzone próbami, budżet prądu zmierzony |
| **E7** Hosty | moduł w CanSensorHub (mapa, niebo, IMU, narzędzie kalibracji magnetometru), dedykowany widok w CanSensorLoggerPi | — |
| **E8** Odbiór | praca ciągła 24 h, testy terenowe (dokładność, TTFF), wersja 1.0, wpis egzemplarza w `node_addresses.yaml` | — |

Każdy obraz wgrany na sprzęt trafia przed wgraniem do `releases/` wraz
z `MANIFEST.txt`, według zasad wspólnych dla rodziny węzłów.

## 8. Konfiguracja CubeMX — stan

### 8.1 Aplikacja — `firmware/CanGnssNode/CanGnssNode.ioc`

| Blok | Ustawienie |
|---|---|
| RCC | MSI 4 MHz (Range 6), AHB/APB /1; Voltage Scale 2; LSE, HSI, PLL wyłączone (LSE bypass i tryb PLL MSI włącza kod) |
| CAN1 | Prescaler 1, BS1 6 TQ, BS2 1 TQ, SJW 1 → 500 kbit/s, 87,5 %; Transmit FIFO Priority, Time Triggered Mode, Automatic Retransmission: ENABLE; Automatic Bus-Off: DISABLE |
| TIM2 | wewnętrzny, prescaler 0, okres 0xFFFFFFFF; CH2 input capture, zbocze narastające, filtr 4 |
| USART2 | 9600 8N1, overrun wyłączony; RX DMA1 Ch6 circular, TX DMA1 Ch7 normal |
| I2C1 | 100 kHz (Timing 0x00100D14); RX DMA2 Ch6, TX DMA2 Ch7 |
| ADC1 | IN11, IN12, VREFINT, TEMP (640,5 / 247,5 / 640,5 / 640,5 cyklu); scan, bez trybu ciągłego, wyzwalanie programowe; oversampling 16×, przesunięcie 2; DMA1 Ch1 normal |
| GPIO | PA5, PA9, PA10, PB0, PB4 — wyjścia Low bez podciągnięć; PB5 EXTI falling z pull-up; PA15 wejście |
| NVIC | 0: CAN1_TX, CAN1_RX0, TIM2 · 1: CAN1_SCE · 2: USART2, DMA1 Ch6/Ch7, EXTI9_5 · 3: I2C1 EV/ER, DMA2 Ch6/Ch7, DMA1 Ch1 · 15: SysTick |

Poza CubeMX: linker `FLASH ORIGIN 0x08004000, LENGTH 0x3B800`, sekcja
`.noinit (NOLOAD)`; `SCB->VTOR = 0x08004000` w `USER CODE 1`.

### 8.2 Bootloader — `firmware/CanGnssNode_BL/CanGnssNode_BL.ioc`

| Blok | Ustawienie |
|---|---|
| RCC | MSI 4 MHz |
| CAN1 | jak w aplikacji, Time Triggered Mode: DISABLE; bez NVIC (odpytywanie FIFO) |
| GPIO | PA9 `CAN_S`, PA10 `CAN_R_CTRL` — wyjścia Low bez podciągnięć |
| Pozostałe | bez USART, ADC, TIM; PA5 i PB4 w stanie po resecie |

Poza CubeMX: folder połączony `Core/Common` → `common/bootloader`,
[`bl_board.h`](../firmware/CanGnssNode_BL/Core/Inc/bl_board.h)
(`BL_NODE_ID 0x20`, `BL_DEVICE_TYPE 0x0003`, `BL_VER_MINOR 1`),
`bl_core_run(&hcan1)` w `USER CODE 2`, linker `FLASH LENGTH = 16K`.

## 9. Zagadnienia otwarte

- **Przyporządkowanie zdania NMEA do zbocza PPS** — rozstrzyga pomiar w E1;
  od niego zależy, czy `UNIX` w `SET_TIME_SYNC` to sekunda zbocza, czy
  następna.
- **Zasilanie wsteczne IMU** z linii I2C przy wyłączonym kluczu — pomiar
  w E1; wynik określa rolę klucza ([ADR 0004](adr/0004-imu-przerwania-i-zasilanie.md)).
- **Zakres INA186** — przy wejściu 7 V i pełnym poborze odbiornika tor
  prądu zbliża się do nasycenia (ok. 30 mA); sprawdzenie w E1.
- **Podciągnięcie R18** wyjścia MFP — jak w planie bazy.
- **Automatyczna retransmisja CAN** — węzeł pracuje z włączoną; MPSWP i MPCC
  z wyłączoną. Ujednolicenie należy do decyzji na poziomie rodziny węzłów.
