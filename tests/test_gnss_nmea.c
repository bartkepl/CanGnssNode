/*
 * Testy parsera NMEA (firmware/CanGnssNode/Core/Src/gnss_nmea.c) na PC.
 *
 *   make -C tests          (Linux / WSL, gcc)
 *
 * Kazdy przypadek negatywny sprawdza, ze blad zostal wykryty Z WLASCIWEGO
 * POWODU (wlasciwy licznik) i ze nie zostawil po sobie danych — wzorem testu
 * negatywnego walidatora CanBusCommon.
 */
#include "gnss_nmea.h"

#include <stdio.h>
#include <string.h>

static int g_fail, g_pass;

#define CHECK(cond) do { \
    if (cond) { g_pass++; } \
    else { g_fail++; printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } \
} while (0)

#define CHECK_EQ(a, b) do { \
    long long _a = (long long)(a), _b = (long long)(b); \
    if (_a == _b) { g_pass++; } \
    else { g_fail++; printf("  FAIL %s:%d: %s == %lld, oczekiwano %lld\n", \
                            __FILE__, __LINE__, #a, _a, _b); } \
} while (0)

static void feed_str(gnss_nmea_t *p, const char *s)
{
    gnss_nmea_feed(p, (const uint8_t *)s, strlen(s));
}

/* Sklada poprawne zdanie z tresci i podaje je parserowi. */
static void feed_body(gnss_nmea_t *p, const char *body)
{
    char buf[160];
    size_t n = gnss_nmea_build(buf, sizeof buf, body);
    CHECK(n > 0u);
    gnss_nmea_feed(p, (const uint8_t *)buf, n);
}

/* --------------------------------------------------------------------- */

static void t_checksum_reference(void)
{
    /* Zdania wzorcowe z opisu NMEA 0183 — sumy znane niezaleznie. */
    gnss_nmea_t p;
    gnss_nmea_init(&p);
    feed_str(&p, "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A\r\n");
    feed_str(&p, "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n");
    feed_str(&p, "$GPGSA,A,3,04,05,,09,12,,,24,,,,,2.5,1.3,2.1*39\r\n");
    feed_str(&p, "$GPGSV,2,1,08,01,40,083,46,02,17,308,41,12,07,344,39,14,22,228,45*75\r\n");
    CHECK_EQ(p.cnt.err_checksum, 0);
    CHECK_EQ(p.cnt.err_format, 0);
    CHECK_EQ(p.cnt.lines_ok, 4);

    char out[32];
    size_t n = gnss_nmea_build(out, sizeof out, "PMTK314,0,1,0,1,1,5,0,0,0,0,0,0,0,0,0,0,0,0,0");
    CHECK_EQ(n, 0);          /* bufor za maly */
    char big[96];
    n = gnss_nmea_build(big, sizeof big, "PMTK000");
    CHECK(strcmp(big, "$PMTK000*32\r\n") == 0);   /* wzorzec z dokumentacji MTK */
    CHECK_EQ(n, 13);
    CHECK_EQ(gnss_nmea_build(big, sizeof big, "PMTK*000"), 0);
}

static void t_epoch_reference(void)
{
    gnss_nmea_t p;
    gnss_fix_t f;
    gnss_nmea_init(&p);
    feed_str(&p, "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A\r\n");
    feed_str(&p, "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n");
    CHECK(!gnss_nmea_take_fix(&p, &f));          /* czeka na GSA */
    feed_str(&p, "$GPGSA,A,3,04,05,,09,12,,,24,,,,,2.5,1.3,2.1*39\r\n");
    CHECK(gnss_nmea_take_fix(&p, &f));
    CHECK(!gnss_nmea_take_fix(&p, &f));          /* pobranie zeruje znacznik */

    CHECK(f.time_valid);
    CHECK_EQ(f.unix_s, 764426119);               /* 1994-03-23 12:35:19 UTC */
    CHECK(f.pos_valid);
    CHECK_EQ(f.lat_e7, 481173000);               /* 48 deg 07.038' */
    CHECK_EQ(f.lon_e7, 115166667);               /* 11 deg 31.000' */
    CHECK_EQ(f.speed_cms, 1152);                 /* 22.4 kn */
    CHECK(f.course_valid);
    CHECK_EQ(f.course_cdeg, 8440);
    CHECK(f.alt_valid);
    CHECK_EQ(f.alt_msl_cm, 54540);
    CHECK_EQ(f.geoid_sep_cm, 4690);
    CHECK_EQ(f.alt_ell_cm, 59230);               /* elipsoida = geoida + separacja */
    CHECK_EQ(f.sats_used, 8);
    CHECK_EQ(f.hdop_c, 90);                      /* z GGA */
    CHECK_EQ(f.pdop_c, 250);
    CHECK_EQ(f.vdop_c, 210);
    CHECK_EQ(f.gsa_mode, 3);
    CHECK_EQ(f.fix_type, GNSS_FIX_3D);
    CHECK_EQ(p.cnt.fixes, 1);
}

/* Epoka L76-L w trybie wielosystemowym (GN), z ujemnymi wspolrzednymi. */
static void t_epoch_gn_south_west(void)
{
    gnss_nmea_t p;
    gnss_fix_t f;
    gnss_nmea_init(&p);
    feed_body(&p, "GNRMC,140531.000,A,3352.1234,S,15112.5678,W,0.05,,151126,,,A");
    feed_body(&p, "GNVTG,,T,,M,0.05,N,0.09,K,A");                /* ignorowane */
    feed_body(&p, "GNGGA,140531.000,3352.1234,S,15112.5678,W,2,11,0.82,-12.30,M,-30.15,M,,");
    feed_body(&p, "GPGSA,A,2,01,02,03,,,,,,,,,,1.50,0.82,1.25");
    feed_body(&p, "GLGSA,A,2,65,66,,,,,,,,,,,1.50,0.82,1.25");    /* drugi GSA epoki */
    CHECK(gnss_nmea_take_fix(&p, &f));
    CHECK_EQ(p.cnt.fixes, 1);
    CHECK_EQ(p.cnt.lines_ignored, 1);
    CHECK_EQ(f.unix_s, 1794751531);              /* 2026-11-15 14:05:31 UTC */
    CHECK_EQ(f.ms, 0);
    CHECK_EQ(f.lat_e7, -338687233);              /* 33 + 52.1234/60 */
    CHECK_EQ(f.lon_e7, -1512094633);             /* 151 + 12.5678/60 */
    CHECK_EQ(f.speed_cms, 3);
    CHECK(!f.course_valid);                      /* kurs pusty */
    CHECK_EQ(f.alt_msl_cm, -1230);
    CHECK_EQ(f.alt_ell_cm, -4245);
    CHECK_EQ(f.gga_quality, 2);
    CHECK_EQ(f.fix_type, GNSS_FIX_DGPS);         /* SBAS ma pierwszenstwo przed 2D */
    CHECK_EQ(f.gsa_mode, 2);
}

/* Odbiornik bez rozwiazania: czas z RTC modulu (1980), status V. */
static void t_no_fix(void)
{
    gnss_nmea_t p;
    gnss_fix_t f;
    gnss_nmea_init(&p);
    feed_body(&p, "GPRMC,000012.800,V,,,,,0.00,0.00,060180,,,N");
    feed_body(&p, "GPGGA,000012.800,,,,,0,0,,,M,,M,,");
    feed_body(&p, "GPGSA,A,1,,,,,,,,,,,,,,,");
    CHECK(gnss_nmea_take_fix(&p, &f));
    CHECK(!f.time_valid);
    CHECK(!f.pos_valid);
    CHECK(!f.alt_valid);
    CHECK(!f.course_valid);
    CHECK_EQ(f.fix_type, GNSS_FIX_NONE);
    CHECK_EQ(f.ms, 800);
    CHECK_EQ(p.cnt.err_format, 0);
}

/* GSA wylaczone: publikacja przy rozpoczeciu nastepnej epoki. */
static void t_epoch_without_gsa(void)
{
    gnss_nmea_t p;
    gnss_fix_t f;
    gnss_nmea_init(&p);
    feed_body(&p, "GPRMC,101010.000,A,5013.0000,N,01900.0000,E,0.00,0.00,010125,,,A");
    feed_body(&p, "GPGGA,101010.000,5013.0000,N,01900.0000,E,1,6,1.20,250.00,M,40.00,M,,");
    CHECK(!gnss_nmea_take_fix(&p, &f));
    feed_body(&p, "GPRMC,101011.000,A,5013.0000,N,01900.0000,E,0.00,0.00,010125,,,A");
    CHECK(gnss_nmea_take_fix(&p, &f));
    CHECK_EQ(f.lat_e7, 502166667);
    CHECK_EQ(f.fix_type, GNSS_FIX_3D);           /* tryb nieznany, jakosc 1 */
    CHECK_EQ(f.gsa_mode, 0);
    /* niekompletna epoka (sam RMC) nie jest publikowana */
    feed_body(&p, "GPRMC,101012.000,A,5013.0000,N,01900.0000,E,0.00,0.00,010125,,,A");
    CHECK(!gnss_nmea_take_fix(&p, &f));
}

static void t_dates(void)
{
    CHECK_EQ(gnss_nmea_days_from_civil(1970, 1, 1), 0);
    CHECK_EQ(gnss_nmea_days_from_civil(2000, 2, 29) * 86400LL + 86399, 951868799);
    CHECK_EQ(gnss_nmea_days_from_civil(2099, 12, 31) * 86400LL, 4102358400LL);
}

static void t_gsv_sky(void)
{
    gnss_nmea_t p;
    gnss_nmea_init(&p);
    feed_str(&p, "$GPGSV,2,1,08,01,40,083,46,02,17,308,41,12,07,344,39,14,22,228,45*75\r\n");
    CHECK_EQ(p.sky.tracked, 0);                  /* seria niekompletna */
    feed_body(&p, "GPGSV,2,2,08,15,10,100,,16,20,200,30,17,30,300,,18,40,010,20");
    CHECK_EQ(p.sky.in_view, 8);
    CHECK_EQ(p.sky.tracked, 6);
    CHECK_EQ(p.sky.cn0_max, 46);
    CHECK_EQ(p.sky.cn0_avg, 37);                 /* (46+41+39+45+30+20)/6 = 36.8 */
    feed_body(&p, "GLGSV,1,1,03,65,50,050,33,66,10,100,,67,20,200,28,1");  /* z polem sygnalu */
    CHECK_EQ(p.sky.in_view, 11);
    CHECK_EQ(p.sky.tracked, 8);
    CHECK_EQ(p.sky.cn0_avg, 35);                 /* (221+61)/8 = 35.25 */

    /* zgubiony srodek serii: seria porzucona, stan poprzedni zostaje */
    feed_body(&p, "GPGSV,3,1,09,01,40,083,10,02,17,308,10,12,07,344,10,14,22,228,10");
    feed_body(&p, "GPGSV,3,3,09,20,10,100,10");
    CHECK_EQ(p.sky.in_view, 11);
    CHECK_EQ(p.sky.cn0_max, 46);
}

static void t_pmtk(void)
{
    gnss_nmea_t p;
    uint16_t cmd; uint8_t flag;
    gnss_nmea_init(&p);
    feed_body(&p, "PMTK010,001");
    CHECK(p.startup_seen);
    feed_body(&p, "PMTK001,314,3");
    CHECK(gnss_nmea_take_ack(&p, &cmd, &flag));
    CHECK_EQ(cmd, 314);
    CHECK_EQ(flag, 3);
    CHECK(!gnss_nmea_take_ack(&p, &cmd, &flag));
    feed_body(&p, "PMTK011,MTKGPS");             /* nieobslugiwane, poprawne */
    CHECK_EQ(p.cnt.lines_ignored, 1);
}

/* ----------------------------------------------------------- negatywne -- */

static void t_bad_checksum(void)
{
    gnss_nmea_t p;
    gnss_fix_t f;
    gnss_nmea_init(&p);
    feed_str(&p, "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6B\r\n");
    CHECK_EQ(p.cnt.err_checksum, 1);
    CHECK_EQ(p.cnt.err_format, 0);
    CHECK_EQ(p.cnt.lines_ok, 0);
    CHECK(!gnss_nmea_take_fix(&p, &f));
    CHECK_EQ(p.work_have, 0);                    /* nic nie trafilo do epoki */
}

static void t_missing_checksum(void)
{
    gnss_nmea_t p;
    gnss_nmea_init(&p);
    feed_str(&p, "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,\r\n");
    feed_str(&p, "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*4\r\n");
    feed_str(&p, "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*4G\r\n");
    CHECK_EQ(p.cnt.err_format, 3);
    CHECK_EQ(p.cnt.err_checksum, 0);
    CHECK_EQ(p.work_have, 0);
}

static void t_truncated_and_resync(void)
{
    gnss_nmea_t p;
    gnss_nmea_init(&p);
    /* zdanie przerwane nowym '$' — liczone jako blad formatu, nastepne przyjete */
    feed_str(&p, "$GPRMC,123519,A,4807.0");
    feed_str(&p, "$GPGSA,A,3,04,05,,09,12,,,24,,,,,2.5,1.3,2.1*39\r\n");
    CHECK_EQ(p.cnt.err_format, 1);
    CHECK_EQ(p.cnt.lines_ok, 1);
    /* smieci przed '$' sa pomijane bez bledu */
    static const char junk[] = "\xFF\x00" "garbage$GPGSA,A,3,04,05,,09,12,,,24,,,,,2.5,1.3,2.1*39\r\n";
    gnss_nmea_feed(&p, (const uint8_t *)junk, sizeof junk - 1u);   /* z bajtem 0x00 */
    CHECK_EQ(p.cnt.err_format, 1);
    CHECK_EQ(p.cnt.lines_ok, 2);
    /* podzial w dowolnym miejscu, bajt po bajcie */
    const char *s = "$GPGSA,A,3,04,05,,09,12,,,24,,,,,2.5,1.3,2.1*39\r\n";
    for (size_t i = 0; s[i]; i++) gnss_nmea_feed(&p, (const uint8_t *)&s[i], 1);
    CHECK_EQ(p.cnt.lines_ok, 3);
}

static void t_overflow(void)
{
    gnss_nmea_t p;
    gnss_nmea_init(&p);
    char big[300];
    big[0] = '$';
    memset(big + 1, 'A', 250);
    strcpy(big + 251, "*00\r\n");
    feed_str(&p, big);
    CHECK_EQ(p.cnt.err_overflow, 1);
    CHECK_EQ(p.cnt.err_format, 0);
    CHECK_EQ(p.len, 0);
    feed_str(&p, "$GPGSA,A,3,04,05,,09,12,,,24,,,,,2.5,1.3,2.1*39\r\n");
    CHECK_EQ(p.cnt.lines_ok, 1);                 /* po przepelnieniu parser wraca */
}

static void t_non_ascii_inside(void)
{
    gnss_nmea_t p;
    gnss_nmea_init(&p);
    feed_str(&p, "$GPGSA,A,3,04,05,\x01,09*39\r\n");
    CHECK_EQ(p.cnt.err_format, 1);
    CHECK_EQ(p.cnt.lines_ok, 0);
}

static void t_bad_fields(void)
{
    gnss_nmea_t p;
    gnss_fix_t f;
    gnss_nmea_init(&p);
    feed_body(&p, "GPRMC,123519,A,4867.038,N,01131.000,E,0,0,230394,,");   /* minuty 67 */
    feed_body(&p, "GPRMC,123519,A,4807.038,X,01131.000,E,0,0,230394,,");   /* polkula */
    feed_body(&p, "GPRMC,123519,A,9107.038,N,01131.000,E,0,0,230394,,");   /* 91 deg */
    feed_body(&p, "GPRMC,253519,A,4807.038,N,01131.000,E,0,0,230394,,");   /* godzina 25 */
    feed_body(&p, "GPRMC,123519,A,4807.038,N,01131.000,E,0,0,321394,,");   /* miesiac 13 */
    feed_body(&p, "GPRMC,123519,Q,4807.038,N,01131.000,E,0,0,230394,,");   /* status */
    feed_body(&p, "GPRMC,123519,A,48.07.03,N,01131.000,E,0,0,230394,,");   /* format liczby */
    feed_body(&p, "GPRMC,123519,A,4807.038,N");                            /* za malo pol */
    feed_body(&p, "GPGGA,123519,4807.038,N,01131.000,E,9,08,0.9,545.4,M,46.9,M,,");  /* jakosc 9 */
    CHECK_EQ(p.cnt.err_format, 9);
    CHECK_EQ(p.cnt.lines_ok, 0);
    CHECK_EQ(p.work_have, 0);                    /* bledne zdanie nie otwiera epoki */
    CHECK(!gnss_nmea_take_fix(&p, &f));
}

static void t_gsa_without_epoch(void)
{
    gnss_nmea_t p;
    gnss_fix_t f;
    gnss_nmea_init(&p);
    feed_str(&p, "$GPGSA,A,3,04,05,,09,12,,,24,,,,,2.5,1.3,2.1*39\r\n");
    CHECK_EQ(p.work_have, 0);                    /* GSA bez RMC/GGA nie tworzy epoki */
    CHECK(!gnss_nmea_take_fix(&p, &f));
}

int main(void)
{
    struct { const char *name; void (*fn)(void); } tests[] = {
        { "suma kontrolna - zdania wzorcowe", t_checksum_reference },
        { "epoka wzorcowa RMC+GGA+GSA",      t_epoch_reference },
        { "epoka GN, polkula S/W, SBAS",     t_epoch_gn_south_west },
        { "brak rozwiazania",                t_no_fix },
        { "epoka bez GSA",                   t_epoch_without_gsa },
        { "daty",                            t_dates },
        { "GSV - stan nieba",                t_gsv_sky },
        { "PMTK",                            t_pmtk },
        { "NEG: zla suma",                   t_bad_checksum },
        { "NEG: brak lub zla postac sumy",   t_missing_checksum },
        { "NEG: linia ucieta, resynchronizacja", t_truncated_and_resync },
        { "NEG: przepelnienie bufora",       t_overflow },
        { "NEG: znak spoza ASCII",           t_non_ascii_inside },
        { "NEG: bledne pola",                t_bad_fields },
        { "NEG: GSA poza epoka",             t_gsa_without_epoch },
    };
    for (size_t i = 0; i < sizeof tests / sizeof tests[0]; i++) {
        int before = g_fail;
        tests[i].fn();
        printf("[%s] %s\n", g_fail == before ? " OK " : "FAIL", tests[i].name);
    }
    printf("\nWYNIK: %d kontroli, %d bledow\n", g_pass + g_fail, g_fail);
    return g_fail ? 1 : 0;
}
