# 0001. Spójna migawka pozycji

**Stan:** przyjęta · **Data:** 2026-11-15
**Dotyczy:** `protocol/device.yaml`, warstwa GNSS firmware

## Kontekst

Telemetria standardu przenosi jedną wielkość na ramkę (`seq`, `flags`,
`value` i32, `qual`). Szerokość i długość geograficzna, wysokość, prędkość
i kurs trafiają więc do hosta w osobnych ramkach. Odbiorca nie ma w nich
informacji, czy wartości pochodzą z tego samego rozwiązania: przy okresie
telemetrii zbliżonym do okresu rozwiązania (1 s) szerokość z epoki N może
zostać zestawiona z długością z epoki N+1. W ruchu z prędkością 30 m/s
daje to błąd pozycji rzędu dziesiątek metrów.

Wcześniejszy szkic protokołu (README, „ramki CAN v1”) zakładał własne ramki
`POSITION` z parą lat/lon. Rozwiązanie takie omija rejestr wielkości, więc
host generyczny (CanSensorLoggerPi) nie zdekodowałby pozycji.

Wysokość: rejestr definiuje `ALTITUDE` jako wysokość nad elipsoidą WGS84,
odbiornik L76-L podaje w GGA wysokość nad geoidą i separację geoidy.

## Rozważane warianty

| Wariant | Ocena |
|---|---|
| A. Wyłącznie ramki własne z parami wartości | odrzucony — pozycja nieczytelna dla hosta generycznego |
| B. Wyłącznie telemetria rejestrowa | odrzucony — brak gwarancji spójności epoki |
| C. Telemetria rejestrowa nadawana jedną serią z jednej migawki + komenda własna ze spójną migawką | przyjęty |

## Decyzja

1. Kanały 0x50–0x57 nadawane są w jednym cyklu telemetrii **z jednej
   migawki rozwiązania**, zatrzaśniętej przed nadaniem pierwszego kanału
   serii. Rozwiązanie nieważne (brak fixa, `SATELLITES` < `GNSS_MIN_SATS`,
   `HDOP` > `GNSS_MAX_HDOP`) nadawane jest z `VALID = 0`.
2. Komenda `READ_FIX` (0x80, STREAM, 32 B) zwraca całą migawkę: czas UTC
   rozwiązania, pozycję, wysokość nad elipsoidą i nad geoidą, prędkość,
   kurs, liczbę satelitów, typ rozwiązania, HDOP, wiek migawki i flagi
   (`VALID`, `AVERAGED`, `PPS`). Jest to źródło właściwe dla hostów, które
   potrzebują pozycji spójnej.
3. `ALTITUDE` = wysokość nad geoidą + separacja geoidy (pole 11 GGA),
   zgodnie z rejestrem. Wysokość nad geoidą dostępna jest wyłącznie
   w `READ_FIX`.
4. Typ rozwiązania kodowany jak wielkość `FIX_TYPE`: brak → 0; GSA tryb 2 →
   1; tryb 3 → 2; GGA jakość 2 (SBAS/DGPS) przy trybie 2 lub 3 → 3.

## Konsekwencje

- Rejestrator generyczny zapisuje pozycję bez wiedzy o typie urządzenia;
  korelację kanałów jednej serii umożliwia wspólny cykl i licznik `seq`.
- Host potrzebujący pewnej spójności (CanSensorHub, mapa) korzysta
  z `READ_FIX`.
- Szkic „ramki CAN v1” w README traci ważność; opis protokołu pochodzi
  wyłącznie z `device.yaml`.
