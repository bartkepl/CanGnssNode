/**
 * @file    bl_board.h
 * @brief   Konfiguracja bootloadera CanGnssNode.
 *
 *  Jedyny plik bootloadera zyjacy w repozytorium urzadzenia — pozostale
 *  pochodza z podmodulu common/bootloader/ (szablon: bl_board.h.template).
 *
 *  Kod bootloadera jest pin-agnostyczny: makra pinow (CAN_R_CTRL_Pin,
 *  CAN_S_Pin) pochodza z main.h generowanego przez CubeMX. Opis pinow nizej
 *  sluzy diagnostyce bez zagladania do .ioc.
 */
#ifndef BL_BOARD_H
#define BL_BOARD_H

/* ---------------------------------------------------------------- wezel ---- */
/*  Adres, na ktorym bootloader odpowiada. WKOMPILOWANY w obraz i NIEZALEZNY od
 *  NODE_ID aplikacji — narzedzie flashujace musi znac oba.
 *  Wartosc bierze sie z registry/device_types.yaml (pole bootloader_node);
 *  walidator sprawdza zgodnosc.
 *
 *  UWAGA: adres bootloadera musi byc UNIKALNY na magistrali. Dwa bootloadery
 *  o tym samym adresie odpowiadalyby jednoczesnie na te sama komende. */
#define BL_NODE_ID       0x20u

/*  DEVICE_TYPE z registry/device_types.yaml. Zwracany w odpowiedzi HELLO
 *  (bajty [6:7]) od wersji BL 1.1 — pozwala narzedziu flashujacemu upewnic
 *  sie, ze nie wgrywa obrazu MPCC do stacji pogodowej. */
#define BL_DEVICE_TYPE   0x0003u

/* ----------------------------------------------------------- wersja wire --- */
/*  0 = zgodnosc bit-w-bit z bootloaderem zalanego MPSWP (HELLO zwraca zera w
 *      bajtach [3] i [6:7]).
 *  1 = wypelnia te wolne bajty node_id i device_type. Pola v1.0 bez zmian,
 *      wiec stary host dziala dalej.
 *  Nowe wezly: zostaw 1. Odbudowa bootloadera dla zalanego MPSWP jest
 *  niemozliwa, wiec tam pytanie nie powstaje. */
#define BL_VER_MINOR     1u

/* ------------------------------------------------------------------ piny --- */
/*  Dokumentacja, nie konfiguracja — faktyczne przypisanie robi CubeMX (.ioc).
 *
 *    CAN_R_CTRL   terminacja 120 Ohm (TS5A21366); HIGH = zalaczona
 *    CAN_S        TJA1051 silent;                  LOW  = praca normalna
 *
 *  Bootloader nie uzywa zadnego USART (ADR 0006) — jedynym transportem jest
 *  CAN, a droga odzysku poza CAN jest SWD. Projekt bootloadera nie musi
 *  konfigurowac w CubeMX zadnego USART.
 *
 *  Przyklady z istniejacych projektow:
 *    MPSWP : CAN_R_CTRL = PB1, CAN_S = PA8
 *    MPCC  : CAN_R_CTRL = PB5, CAN_S = PB1
 *
 *  CanGnssNode: CAN_R_CTRL = PA10 (terminacja split przez PhotoMOS GAQY212GS,
 *  HIGH = zalaczona), CAN_S = PA9 (ATA6561 STBY, LOW = praca normalna).
 *  PA5 (GPS_RST) i PB4 (IMU_EN) zostaja w stanie po resecie: odbiornik GNSS
 *  pracuje dalej, IMU pozostaje wylaczone przez R41.
 *
 *  UWAGA: bootloader zalacza terminacje BEZWARUNKOWO na czas swojej pracy
 *  (bl_core.c / enable_bus). Dla wezla w SRODKU magistrali oznacza to trzecie
 *  zakonczenie na czas programowania. Przy dlugiej magistrali warto to
 *  sprawdzic oscyloskopem przed pierwszym zdalnym flashowaniem. */
#define BL_BOARD_NAME    "CanGnssNode"

#endif /* BL_BOARD_H */
