# TODO — dokończenie konwersji CanGeigerProbe → CanGnssNode

Checklista zmian do wykonania ręcznie w CubeMX/CubeIDE i KiCad (świadomie nietknięte przeze mnie — patrz `README.md`). Dane o pinach L76-L zweryfikowane ze **Quectel L76&L76-L Hardware Design v3.2** (2021-08-16), nie z pamięci.

## 1. `.ioc` (CubeMX) — `firmware/CanGnssNode/CanGeigerProbe.ioc`

- [x] Przenazwać plik na `CanGnssNode.ioc`; w środku zmienić `ProjectManager.ProjectName=CanGnssNode` i `ProjectManager.ProjectFileName=CanGnssNode.ioc`.
- [x] Usunąć IP: `COMP1`, `DAC1`, `TIM2` (pętla HV — nieużywana).
- [x] `ADC1`: usunąć kanał `IN10` (`HV_meas`, PA5). Zostawić `IN11` (`VIN_SENSE`, PA6) i `IN12` (`I_SENS`, PA7).
- [x] Dodać `USART2` — tryb Asynchronous, PA2=TX / PA3=RX (AF7). Baudrate startowy **9600 8N1** (domyślne ustawienie L76-L; moduł obsługuje też 4800–921600 bps, zmienialne komendą PMTK — jeśli finalnie chcesz korzystać z 1PPS + NMEA jednocześnie, moduł wymaga do tego ≥19200 bps).
- [x] PA1: GPIO input, etykieta `GPS_PPS` (opcjonalne, 1PPS z modułu — puls 100 ms, zbocze narastające; jeśli nieużywany, zostaw NC po stronie modułu).
- [x] PA5: GPIO output, etykieta `GPS_RESET` (RESET_N modułu, aktywny stan niski — output push-pull, stan domyślny wysoki/zwolniony).

### Tabela pinów MCU (co było HV/GM → co jest GPS)

| Pin | Było | Będzie |
|-----|------|--------|
| PA2 | `FBACK_CTRL` (TIM2_CH3 PWM) | `USART2_TX` → L76-L pin 3 (RXD) |
| PA3 | zarezerwowany, nieużywany | `USART2_RX` ← L76-L pin 2 (TXD) |
| PA1 | `COMP_GM_IN` (COMP1) | `GPS_PPS` ← L76-L pin 4 (1PPS), opcjonalnie |
| PA5 | `HV_meas` (ADC1_IN10) | `GPS_RESET` → L76-L pin 9 (RESET_N) |
| PA6/PA7 | `VIN_SENSE`/`I_SENS` (ADC1) | bez zmian |

CAN1 (PA11/PA12), I2C1/RTC (PB6/PB7), SWD (PA13/PA14/PB3), terminacja CAN (PA9/PA10) — bez zmian.

## 2. Schemat KiCad — nowy arkusz `gps.kicad_sch`

Pełny pinout Quectel L76-L (LCC-18, 0.65 mm rozstaw, pin 1 oznaczony na obudowie):

| Pin | Nazwa | Kierunek | Podłączenie |
|-----|-------|----------|-------------|
| 1, 10, 12 | GND | — | masa sygnałowa |
| 2 | TXD | DO | → MCU `USART2_RX` (PA3) |
| 3 | RXD | DI | ← MCU `USART2_TX` (PA2) |
| 4 | 1PPS | DO | → MCU `GPS_PPS` (PA1), opcjonalnie; jeśli nieużywane, zostaw NC |
| 5 | STANDBY | DI | pull-up wewnętrzny, edge-triggered — jeśli nieużywane, zostaw NC |
| 6 | V_BCKP | PI | zasilanie domeny RTC modułu, 1.5–4.5 V (typ. 3.3 V) — patrz sekcja VBACKUP niżej |
| 7, 15 | NC | — | nie podłączać |
| 8 | VCC | PI | zasilanie główne, 2.8–4.3 V (typ. 3.3 V), **min. wydajność prądowa 150 mA**; sieć odsprzęgająca 10 µF + 100 nF + 33 pF blisko pinu + TVS. Zasilać z istniejącej szyny +3.3V (LDO MCP1700) — **nie** bezpośrednio z przetwornicy impulsowej (LT8606), datasheet tego nie zaleca |
| 9 | RESET_N | DI | ← MCU `GPS_RESET` (PA5), aktywny stan niski |
| 11 | RF_IN | AI | tor antenowy, 50 Ω — patrz sekcja Antena niżej |
| 13 | ANTON | DO | steruje LNA/zasilaniem anteny aktywnej — zostaw NC jeśli używasz anteny pasywnej |
| 14 | VDD_RF | PO | = VCC, zasilanie anteny aktywnej/LNA zewnętrznego — zostaw NC jeśli antena pasywna |
| 16 | I2C_SDA (RESERVED na L76/L76-L(L), aktywne na L76-L) | DIO | alternatywa dla UART — nieużywane w tej wersji, zostaw NC |
| 17 | I2C_SCL | DIO | jw. — NC |
| 18 | WAKEUP | DI | wybudzenie z trybu Backup, domena RTC — zostaw NC/pull-low jeśli tryb Backup nieużywany |

**Uwaga poziomów logicznych:** domena I/O modułu L76-L (odmiana bez `(L)`) to **2.8 V**, `VIN_IO` max **3.1 V absolute maximum**. STM32 sterujący z 3.3V logiki (`USART2_TX`→RXD modułu) technicznie przekracza tę wartość maksymalną. Najprościej: rezystor szeregowy ~1–2.2 kΩ na linii MCU→RXD (obniża stromość zbocza i ogranicza prąd, popularne rozwiązanie w praktyce) albo dzielnik rezystorowy/level-shifter, jeśli zależy Ci na pełnej zgodności ze specyfikacją. Linia moduł→MCU (TXD→RX) nie jest problemem (MCU akceptuje niższy poziom VIH).

**VBACKUP:** V_BCKP modułu (1.5–4.5 V, typ. 3.3 V) pokrywa się z istniejącym ogniwem `BT1` (MS621FE-FL11E), które dziś zasila tylko MCP79410. Podłącz V_BCKP do tej samej szyny **przez diodę szeregową** (jak już jest zrobione dla MCP79410 — sprawdź istniejący obwód w `mcu.kicad_sch`), żeby oba odbiorniki nie zwierały się wzajemnie przy różnym poborze.

**Antena (`RF_IN`, pin 11):** przy odległości anteny od modułu **< 1 m** (nasz przypadek — insert na tej samej rurze) datasheet rekomenduje **antenę pasywną** — prościej, bez obwodu zasilania. Referencyjny π-matching: `RF_IN — C1 100pF (DC-block) — L1 47nH — antena`, plus TVS na linii. Jeśli jednak wybierzesz antenę aktywną (np. przez dłuższy pigtail koncentryczny do patcha w insercie), zasil ją z `VDD_RF` przez `R2 10R` + `C4/C5` odsprzęgające + TVS (pełny schemat: rozdz. 5.2.3 datasheetu). Decyzja aktywna/pasywna i dokładne umiejscowienie (na płytce vs. w insercie) zależy od finalnego layoutu dwustronnego PCB — do ustalenia w KiCad.

Podczas montażu/lutowania pinu `RF_IN`: najpierw podłącz GND, potem RF_IN; używaj lutownicy z grotem ESD-safe (moduł jest czuły na ESD na tym pinie).

### Zmiany w pozostałych arkuszach
- [x] Usunąć `hv.kicad_sch` (flyback, dzielnik HV, detektor G-M — referencje D1-D4, Q1, R6-R14, C11-C19, F3, `ANODE1`, `CATHODE1`).
- [x] `mcu.kicad_sch`: zamienić hierarchiczne piny `COMP_GM`/`HV_meas`/`FBACK_CTRL` na `USART2_TX`/`USART2_RX`/`GPS_PPS`/`GPS_RESET`, poprowadzić do pinów z tabeli wyżej.
- [x] Root sheet `gnss_node.kicad_sch`: podmienić arkusz HV na nowy `gps.kicad_sch` w siatce połączeń hierarchicznych.
- [x] `pwr.kicad_sch`, `can_bus.kicad_sch`, `prot.kicad_sch` — bez zmian funkcjonalnych.
- [x] Po otwarciu w KiCad: **File → Save Project As** (albo zwykły zapis), żeby scalić już przenazwane pliki (`gnss_node.*`) z wewnętrznymi referencjami projektu w arkuszach podrzędnych (dziś nadal wskazują `"geiger_probe"` w metadanych instancji — kosmetyczne, KiCad samo to poprawi przy zapisie).

## 3. PCB — `gnss_node.kicad_pcb`

- [x] Usunąć footprinty: `D1`-`D4`, `Q1`, `R6`-`R14`, `C11`-`C19`, `F3`, `ANODE1`, `CATHODE1` (cała sekcja HV/G-M).
- [ ] Przeprojektować na **dwustronny** układ: elektronika cyfrowa/zasilanie na jednej stronie, cała strona RF (moduł L76-L + tor antenowy + matching) na drugiej — zgodnie z Twoją decyzją o zmniejszeniu płytki.
- [ ] Dodać footprint L76-L (LCC-18, 10.1×9.7 mm, rozstaw pinów 0.65 mm — użyj/zrób w `pcb/lib_local/` wzorem istniejących `INA186A4IDCKR`/`CDM1209-05A-MP-F12-67`) + footprint toru antenowego (U.FL `Connector_Coaxial` jeśli pigtail, albo bezpośrednio patch/chip antenna jeśli mieści się na stronie RF).
- [ ] Usunąć netclass `HV_lines` i jej pattern `/HV/HV+*` w `gnss_node.kicad_pro` (sekcja `net_settings`).
- [ ] Zaktualizować obrys płytki pod nowy, mniejszy kształt dwustronny.

## 5. Sanity check na koniec

`grep -ri "geiger\|CanGeigerProbe\|sonda G-M\|flyback"` w całym repo poza tym plikiem (gdzie nazwa starego projektu musi paść dla kontekstu) — zero trafień.
