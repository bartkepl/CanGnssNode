# 0003. Wykrywanie sytuacji i tryby pracy odbiornika

**Stan:** przyjęta, progi do strojenia na nagraniach terenowych · **Data:** 2026-11-15
**Dotyczy:** warstwa IMU i GNSS firmware, parametry 0x14–0x18 i 0x20–0x25

## Kontekst

Węzeł pracuje zależnie od sytuacji stacjonarnie (maszt, postój) albo
w ruchu (przenoszenie, pojazd). Odbiornik L76-L jest dominującym odbiornikiem
energii, a jego zasilanie nie ma klucza — dostępne są tryby Full On,
Periodic (PMTK225), AlwaysLocate i Standby (PMTK161); trybu Backup nie ma.
Zdarzenia nagłe (wstrząs, upadek, przewrócenie) trwają milisekundy i nie są
wykrywalne z rozwiązania GNSS o okresie 1 s. FXOS8700CQ ma sprzętowe
detektory transient, freefall i pulse zgłaszane na linii przerwania.

## Rozważane warianty

| Wariant | Ocena |
|---|---|
| A. Stały tryb odbiornika ustawiany parametrem | zachowany jako opcja (`GNSS_POWER_POLICY` 1–3), nie jako domyślny |
| B. Stan ruchu wyłącznie z prędkości GNSS | odrzucony — nie wykrywa zdarzeń nagłych ani startu ruchu w trybie oszczędnym |
| C. Klasyfikator łączący IMU i GNSS, tryb odbiornika wynikający ze stanu | przyjęty jako domyślny (`GNSS_POWER_POLICY` = 0) |

## Decyzja

1. **Stany ruchu** (wielkość `MOTION_STATE`, zdarzenie `SITUATION_CHANGED`):

   | Stan | Wejście | Reakcja |
   |---|---|---|
   | `STATIC` | odchylenie przyspieszenia < `IMU_STATIC_MG` przez `IMU_STATIC_S` i prędkość GNSS < `MOVING_SPEED` | uśrednianie pozycji (`STATIC_AVERAGING`, `STATIC_AVG_WINDOW_S`); odbiornik w AlwaysLocate albo Periodic; kurs z kompasu |
   | `HANDLING` | ruch z akcelerometru, prędkość < `MOVING_SPEED` | odbiornik Full On 1 Hz; kurs z kompasu |
   | `MOVING` | prędkość GNSS ≥ `MOVING_SPEED` | odbiornik Full On 1 Hz; kurs nad ziemią z GNSS |
   | `UNKNOWN` | po starcie, przed rozstrzygnięciem, albo przy `EN_IMU` = 0 | odbiornik Full On |

2. **Zdarzenia nagłe** z detektorów sprzętowych FXOS8700CQ: `SHOCK`
   (transient ponad `IMU_SHOCK_MG`), `FREEFALL`, `TILT_ALARM` (zbocze
   przekroczenia `IMU_TILT_LIMIT`). Każde zdarzenie nagłe wybudza odbiornik
   z trybu oszczędnego i przenosi stan do `HANDLING`.
3. **Telemetria nie zmienia okresu** w zależności od stanu:
   `TELEMETRY_PERIOD` jest parametrem uniwersalnym, ustawianym przez
   operatora. Pilność przenoszą zdarzenia (najwyższy priorytet arbitrażu).
4. **Tryb rejestracji** surowych danych IMU i GNSS do strojenia progów
   (parametr przyszły, domyślnie wyłączony) przeznaczony jest wyłącznie dla
   stanowiska — zwiększa obciążenie magistrali.

## Konsekwencje

- Wartości domyślne progów są wstępne; ostateczne wynikają z nagrań
  w etapie E6.
- W trybie oszczędnym czas uzyskania nowego rozwiązania po wybudzeniu
  zależy od trybu odbiornika (hot start, rzędu sekund); pozycja w telemetrii
  ma wtedy `VALID` = 0 do odzyskania rozwiązania.
- Wzorzec czasu (ADR 0002) wymaga ważnego PPS; w trybach Periodic
  i AlwaysLocate impuls może zanikać, więc węzeł przechodzi w `HOLDOVER`.
  Gdy `TIME_SYNC_PERIOD` > 0, polityka automatyczna utrzymuje odbiornik
  w trybie Full On.
