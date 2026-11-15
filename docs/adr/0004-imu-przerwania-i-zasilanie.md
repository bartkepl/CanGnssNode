# 0004. IMU: przerwania, zasilanie, orientacja montażu

**Stan:** przyjęta; rola klucza zasilania do potwierdzenia pomiarem w E1 ·
**Data:** 2026-11-15
**Dotyczy:** FXOS8700CQ (U5), Q5/Q6, R38/R39, PB4/PB5, parametr `CAL_MOUNT_ORIENT`

## Kontekst

- Wyjścia INT1 i INT2 FXOS8700CQ połączone są przez rezystory R38 i R39
  (10 kΩ) z jednym wejściem MCU (PB5). W konfiguracji domyślnej układu oba
  wyjścia są push-pull: gdy jedno zgłasza przerwanie (stan niski), a drugie
  trzyma stan nieaktywny (wysoki), na PB5 powstaje połowa napięcia zasilania
  — poziom nierozstrzygalny dla wejścia cyfrowego.
- Zasilanie IMU przełączane jest kluczem Q5/Q6 (`IMU_EN`, PB4, R41 utrzymuje
  stan wyłączony). Linie SDA/SCL magistrali I2C, wspólnej z MCP79410, są
  podciągnięte do +3,3 V niezależnie od klucza. Przy wyłączonym kluczu
  układ może być zasilany wstecznie przez diody zabezpieczające wejść I2C,
  co obciąża magistralę i może zakłócać komunikację z RTC.
- Płytka stoi w rurze pionowo; osie układu scalonego nie pokrywają się
  z osiami węzła. Rejestr wielkości (0x60–0x65) definiuje osie w układzie
  węzła.

## Decyzja

1. **Przerwania:** oba wyjścia INT konfigurowane jako open-drain
   (`CTRL_REG3.PP_OD` = 1), aktywne stanem niskim; PB5 z podciągnięciem
   w MCU (pull-up), EXTI na zboczu opadającym. Źródło przerwania odczytywane
   z rejestru statusu układu.
2. **Klucz zasilania** służy do odzysku układu (wyłączenie i ponowne
   włączenie zasilania po usterce), nie do oszczędzania energii. W pracy
   normalnej IMU pozostaje zasilone, a w czasie bezczynności przechodzi
   w tryb standby układu (rzędu µA). Jeżeli pomiar w E1 wykaże, że przy
   wyłączonym kluczu magistrala I2C działa poprawnie, decyzja zostanie
   rozszerzona o wyłączanie przy `EN_IMU` = 0.
3. **Orientacja montażu** (`CAL_MOUNT_ORIENT`, 0–23): `v_węzeł = R · v_czujnik`,
   gdzie `R = Rz(b · 90°) · F_a`, kod = `4·a + b`, `b` ∈ 0..3, a `F_a`
   wskazuje oś czujnika przechodzącą w oś +Z węzła:

   | a | Oś czujnika → +Z węzła | F_a |
   |---|---|---|
   | 0 | +Z | I |
   | 1 | −Z | Rx(180°) |
   | 2 | +X | Ry(−90°) |
   | 3 | −X | Ry(+90°) |
   | 4 | +Y | Rx(+90°) |
   | 5 | −Y | Rx(−90°) |

   Przeliczenie wykonywane jest dla akcelerometru i magnetometru przed
   korekcją hard-iron; offsety `CAL_MAG_OFF_*` wyrażone są w osiach węzła.
   Właściwy kod dla montażu w rurze wyznaczany jest w E5.

## Konsekwencje

- Bez kroku 1 każde zgłoszenie przerwania przy aktywnym drugim wyjściu
  byłoby nierozpoznawalne; konfiguracja open-drain jest warunkiem użycia
  detektorów sprzętowych (ADR 0003).
- Wariant LSM303AGR (pola DNP) ma inny układ wyjść przerwań i wymaga
  osobnego przeglądu przed dopisaniem do `device.yaml`.
