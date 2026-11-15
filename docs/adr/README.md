# Rejestr decyzji (ADR)

Decyzje projektowe dotyczące wyłącznie tego repozytorium. Decyzje obejmujące
kilka repozytoriów rodziny zapisywane są w `common/docs/adr` (CanBusCommon).

Format: nagłówek z numerem, **Stan / Data / Dotyczy**, Kontekst, Rozważane
warianty, Decyzja, Konsekwencje.

| Nr | Tytuł | Stan |
|---|---|---|
| [0001](0001-spojna-migawka-pozycji.md) | Spójna migawka pozycji | przyjęta |
| [0002](0002-realizacja-wzorca-czasu.md) | Realizacja wzorca czasu | przyjęta (poziom 1) · provisional (poziom 2) |
| [0003](0003-sytuacje-i-tryby-gnss.md) | Wykrywanie sytuacji i tryby pracy odbiornika | przyjęta, progi do strojenia |
| [0004](0004-imu-przerwania-i-zasilanie.md) | IMU: przerwania, zasilanie, orientacja montażu | przyjęta, rola klucza do potwierdzenia pomiarem |

Decyzje warstwy wspólnej, z których węzeł korzysta wprost:
ADR 0006 CanBusCommon (bootloader wyłącznie CAN), ADR 0007 CanBusCommon
(wspólna oś czasu i węzeł wzorcowy czasu) i ADR 0008 CanBusCommon
(automatyczna retransmisja CAN obowiązkowa).
