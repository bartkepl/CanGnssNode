/**
 * @file    gnss_nmea.h
 * @brief   Parser strumienia NMEA 0183 odbiornika Quectel L76-L (MTK).
 *
 *  Modul nie zalezy od HAL ani od sprzetu: przyjmuje bajty odebrane z UART,
 *  sklada linie, sprawdza sume kontrolna i dekoduje zdania RMC, GGA, GSA,
 *  GSV oraz potwierdzenia PMTK001. Wszystkie przeliczenia sa calkowitoliczbowe
 *  i daja wartosci w jednostkach rejestru wielkosci CanBusCommon:
 *    LAT/LON 1e-7 deg, wysokosc 0.01 m, predkosc 0.01 m/s, kurs 0.01 deg,
 *    DOP 0.01.
 *
 *  Migawka rozwiazania (ADR 0001) powstaje z RMC i GGA o tym samym czasie
 *  UTC oraz z pierwszego GSA po nich. Gdy GSA jest wylaczone, migawka jest
 *  publikowana przy rozpoczeciu nastepnej epoki.
 *
 *  Modul jest testowany na PC (tests/test_gnss_nmea.c), takze wejsciem
 *  blednym: zla suma, brak sumy, linia ucieta, przepelnienie bufora, znaki
 *  spoza ASCII.
 */
#ifndef GNSS_NMEA_H
#define GNSS_NMEA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Najdluzsza przyjmowana linia bez "\r\n" (NMEA: 82 znaki; zapas na PMTK). */
#define GNSS_NMEA_LINE_MAX     100u
/** Najwiecej pol w zdaniu (GSV i GSA maja ich najwiecej: 20 i 18). */
#define GNSS_NMEA_FIELDS_MAX   24u
/** Liczba rozrozniane systemow w GSV (GP, GL, GA, BD/GB, GQ/QZ, inne). */
#define GNSS_NMEA_TALKERS      6u
/** Po ilu epokach bez GSV dany system przestaje sie liczyc do stanu nieba. */
#define GNSS_NMEA_SKY_STALE    10u

/** Typ rozwiazania — kodowanie wielkosci FIX_TYPE (0x57). */
enum {
    GNSS_FIX_NONE = 0,
    GNSS_FIX_2D   = 1,
    GNSS_FIX_3D   = 2,
    GNSS_FIX_DGPS = 3,
};

/** Migawka jednej epoki rozwiazania. */
typedef struct {
    uint32_t unix_s;        /**< czas UTC epoki (z RMC), s od 1970-01-01   */
    uint16_t ms;            /**< milisekundy czasu epoki                    */
    bool     time_valid;    /**< RMC ze statusem 'A' i pelna data           */

    bool     pos_valid;     /**< pozycja obecna (RMC 'A' albo GGA jakosc>0) */
    int32_t  lat_e7;        /**< szerokosc, 1e-7 deg, + = polnoc            */
    int32_t  lon_e7;        /**< dlugosc, 1e-7 deg, + = wschod              */

    bool     alt_valid;
    int32_t  alt_msl_cm;    /**< wysokosc nad geoida, 0.01 m (GGA)          */
    int32_t  geoid_sep_cm;  /**< separacja geoidy, 0.01 m (GGA)             */
    int32_t  alt_ell_cm;    /**< wysokosc nad elipsoida = msl + separacja   */

    uint16_t speed_cms;     /**< predkosc nad ziemia, 0.01 m/s              */
    bool     course_valid;
    uint16_t course_cdeg;   /**< kurs nad ziemia, 0.01 deg, 0..35999        */

    uint8_t  sats_used;     /**< satelity uzyte (GGA)                       */
    uint8_t  gga_quality;   /**< 0 brak, 1 GPS, 2 DGPS/SBAS, 6 szacowanie   */
    uint8_t  gsa_mode;      /**< 0 nieznany, 1 brak, 2 = 2D, 3 = 3D         */
    uint16_t hdop_c;        /**< HDOP x100 (0 = brak)                       */
    uint16_t pdop_c;        /**< PDOP x100 (0 = brak)                       */
    uint16_t vdop_c;        /**< VDOP x100 (0 = brak)                       */

    uint8_t  fix_type;      /**< GNSS_FIX_* (kodowanie FIX_TYPE)            */
} gnss_fix_t;

/** Stan nieba z GSV, sumowany po systemach. */
typedef struct {
    uint8_t in_view;        /**< satelity widoczne                          */
    uint8_t tracked;        /**< satelity sledzone (SNR > 0)                */
    uint8_t cn0_max;        /**< najwyzszy C/N0, dB-Hz                      */
    uint8_t cn0_avg;        /**< sredni C/N0 satelitow sledzonych, dB-Hz    */
} gnss_sky_t;

/** Liczniki diagnostyczne strumienia. */
typedef struct {
    uint32_t lines_ok;      /**< linie z poprawna suma, rozpoznane          */
    uint32_t lines_ignored; /**< poprawna suma, zdanie nieobslugiwane       */
    uint32_t err_checksum;  /**< zla suma kontrolna                         */
    uint32_t err_format;    /**< brak sumy, zle pole, znak spoza ASCII      */
    uint32_t err_overflow;  /**< linia dluzsza niz GNSS_NMEA_LINE_MAX       */
    uint32_t fixes;         /**< opublikowane migawki                       */
} gnss_nmea_counters_t;

/** Akumulator GSV jednego systemu. */
typedef struct {
    uint8_t  expect_next;   /**< numer oczekiwanego zdania serii, 0 = brak  */
    uint8_t  total;
    uint8_t  in_view;
    uint8_t  tracked, cn0_max;
    uint16_t cn0_sum;
    /* ostatnia kompletna seria */
    bool     have;
    uint32_t epoch;         /**< nr epoki, w ktorej seria sie zakonczyla    */
    uint8_t  c_in_view, c_tracked, c_cn0_max;
    uint16_t c_cn0_sum;
} gnss_nmea_gsv_t;

/** Stan parsera — przydzielany przez wywolujacego, bez alokacji. */
typedef struct {
    char     line[GNSS_NMEA_LINE_MAX + 1u];
    uint16_t len;
    bool     in_line;       /**< od '$' do konca linii                      */
    bool     discard;       /**< biezaca linia odrzucona (przepelnienie)    */

    gnss_fix_t work;        /**< epoka w budowie                            */
    uint32_t work_tod_ms;   /**< czas dnia epoki w budowie                  */
    uint8_t  work_have;     /**< bity GNSS_NMEA_HAVE_*                      */

    gnss_fix_t fix;         /**< ostatnia opublikowana migawka              */
    bool     fix_new;
    uint32_t epoch;         /**< licznik opublikowanych epok                */

    gnss_nmea_gsv_t gsv[GNSS_NMEA_TALKERS];
    gnss_sky_t sky;

    uint16_t ack_cmd;       /**< ostatnie PMTK001: komenda                  */
    uint8_t  ack_flag;      /**< 0 bledna, 1 nieobslugiwana, 2 nieudana, 3 OK */
    bool     ack_new;
    bool     startup_seen;  /**< PMTK010 — modul wystartowal                */

    gnss_nmea_counters_t cnt;
} gnss_nmea_t;

void   gnss_nmea_init(gnss_nmea_t *p);

/** Podaje bajty odebrane z UART (dowolnie podzielone). */
void   gnss_nmea_feed(gnss_nmea_t *p, const uint8_t *data, size_t n);

/** Przetwarza jedna pelna linie (bez "\r\n"). Zwraca true, gdy rozpoznana. */
bool   gnss_nmea_process_line(gnss_nmea_t *p, const char *line, size_t len);

/** Pobiera nowa migawke; false, gdy od ostatniego pobrania nie bylo nowej. */
bool   gnss_nmea_take_fix(gnss_nmea_t *p, gnss_fix_t *out);

/** Pobiera nowe potwierdzenie PMTK001. */
bool   gnss_nmea_take_ack(gnss_nmea_t *p, uint16_t *cmd, uint8_t *flag);

/**
 * Sklada zdanie do wyslania: "$" + body + "*HH\r\n" + '\0'.
 * @return dlugosc bez '\0'; 0, gdy bufor za maly albo body zawiera '$'/'*'.
 */
size_t gnss_nmea_build(char *out, size_t cap, const char *body);

/** Dni od 1970-01-01 dla daty kalendarza gregorianskiego. */
int32_t gnss_nmea_days_from_civil(int32_t y, uint32_t m, uint32_t d);

#ifdef __cplusplus
}
#endif

#endif /* GNSS_NMEA_H */
