/**
 * @file    gnss_nmea.c
 * @brief   Parser strumienia NMEA 0183 odbiornika Quectel L76-L.
 *
 *  Opis interfejsu i zalozen: gnss_nmea.h. Decyzje: docs/adr/0001.
 */
#include "gnss_nmea.h"

#include <string.h>

#define HAVE_RMC  0x01u
#define HAVE_GGA  0x02u
#define HAVE_GSA  0x04u
#define HAVE_PUB  0x80u   /* epoka juz opublikowana */

#define TOD_NONE  0xFFFFFFFFu

/* ------------------------------------------------------------- narzedzia -- */

static int hexval(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

static bool is_digit(char c) { return c >= '0' && c <= '9'; }

/**
 * Liczba stalopozycyjna: [+-]cyfry[.cyfry] -> wartosc * 10^decimals,
 * z zaokragleniem polowkowym. Pole puste: *present = false, wynik true.
 * Zwraca false przy zlym formacie.
 */
static bool parse_fixed(const char *s, unsigned decimals, int64_t *out, bool *present)
{
    *present = false;
    *out = 0;
    if (s == NULL || *s == '\0') return true;

    bool neg = false;
    if (*s == '-' || *s == '+') { neg = (*s == '-'); s++; }
    if (!is_digit(*s) && *s != '.') return false;

    int64_t v = 0;
    unsigned ndig = 0;
    while (is_digit(*s)) {
        if (++ndig > 12u) return false;
        v = v * 10 + (*s - '0');
        s++;
    }
    unsigned frac = 0;
    int round_digit = -1;
    if (*s == '.') {
        s++;
        if (!is_digit(*s) && ndig == 0u) return false;
        while (is_digit(*s)) {
            if (frac < decimals) {
                v = v * 10 + (*s - '0');
                frac++;
            } else if (round_digit < 0) {
                round_digit = *s - '0';
            }
            s++;
        }
    } else if (ndig == 0u) {
        return false;
    }
    if (*s != '\0') return false;
    for (; frac < decimals; frac++) v *= 10;
    if (round_digit >= 5) v += 1;
    *out = neg ? -v : v;
    *present = true;
    return true;
}

static bool parse_uint(const char *s, uint32_t max, uint32_t *out, bool *present)
{
    int64_t v;
    if (!parse_fixed(s, 0, &v, present)) return false;
    if (!*present) { *out = 0; return true; }
    if (v < 0 || v > (int64_t)max) return false;
    *out = (uint32_t)v;
    return true;
}

/** ddmm.mmmmmm / dddmm.mmmmmm + polkula -> 1e-7 deg. */
static bool parse_coord(const char *val, const char *hemi, uint32_t max_deg,
                        int32_t *out, bool *present)
{
    int64_t v;
    if (!parse_fixed(val, 6, &v, present)) return false;
    if (!*present) return (hemi == NULL || *hemi == '\0');
    if (v < 0) return false;
    int64_t deg   = v / 100000000;          /* 100 * 1e6 */
    int64_t min6  = v % 100000000;          /* minuty * 1e6 */
    if (min6 >= 60000000 || deg > (int64_t)max_deg) return false;
    int64_t e7 = deg * 10000000 + (min6 + 3) / 6;   /* min/60*1e7 = min6/6 */
    if (e7 > (int64_t)max_deg * 10000000) return false;
    if (hemi == NULL || hemi[1] != '\0') return false;
    switch (hemi[0]) {
    case 'N': case 'E': break;
    case 'S': case 'W': e7 = -e7; break;
    default: return false;
    }
    *out = (int32_t)e7;
    return true;
}

/** hhmmss[.sss] -> ms od polnocy. */
static bool parse_tod(const char *s, uint32_t *tod_ms, bool *present)
{
    int64_t v;
    if (!parse_fixed(s, 3, &v, present)) return false;
    if (!*present) return true;
    if (v < 0) return false;
    uint32_t ms  = (uint32_t)(v % 1000);
    uint32_t hms = (uint32_t)(v / 1000);
    uint32_t hh = hms / 10000u, mm = (hms / 100u) % 100u, ss = hms % 100u;
    if (hh > 23u || mm > 59u || ss > 60u) return false;
    *tod_ms = ((hh * 60u + mm) * 60u + ss) * 1000u + ms;
    return true;
}

int32_t gnss_nmea_days_from_civil(int32_t y, uint32_t m, uint32_t d)
{
    /* H. Hinnant, "chrono-compatible low-level date algorithms". */
    y -= (m <= 2u) ? 1 : 0;
    const int32_t era = (y >= 0 ? y : y - 399) / 400;
    const uint32_t yoe = (uint32_t)(y - era * 400);
    const uint32_t doy = (153u * (m + (m > 2u ? (uint32_t)-3 : 9u)) + 2u) / 5u + d - 1u;
    const uint32_t doe = yoe * 365u + yoe / 4u - yoe / 100u + doy;
    return era * 146097 + (int32_t)doe - 719468;
}

/* ------------------------------------------------------------- epoki ------ */

static uint8_t derive_fix_type(const gnss_fix_t *f)
{
    if (!f->pos_valid || f->gga_quality == 0u || f->gsa_mode == 1u) return GNSS_FIX_NONE;
    if (f->gga_quality == 2u) return GNSS_FIX_DGPS;
    if (f->gsa_mode == 2u) return GNSS_FIX_2D;
    /* gsa_mode 3 albo nieznany (GSA wylaczone) przy jakosci > 0 */
    return GNSS_FIX_3D;
}

static void publish(gnss_nmea_t *p)
{
    gnss_fix_t *w = &p->work;
    if (w->alt_valid) w->alt_ell_cm = w->alt_msl_cm + w->geoid_sep_cm;
    w->fix_type = derive_fix_type(w);
    p->fix = *w;
    p->fix_new = true;
    p->epoch++;
    p->cnt.fixes++;
    p->work_have |= HAVE_PUB;
}

/** Rozpoczyna epoke o czasie tod (z RMC/GGA), w razie potrzeby publikuje poprzednia. */
static void epoch_enter(gnss_nmea_t *p, uint32_t tod)
{
    if (p->work_tod_ms == tod) return;
    if ((p->work_have & (HAVE_RMC | HAVE_GGA)) == (HAVE_RMC | HAVE_GGA) &&
        !(p->work_have & HAVE_PUB)) {
        publish(p);      /* GSA nie dotarlo: publikacja przy nastepnej epoce */
    }
    memset(&p->work, 0, sizeof p->work);
    p->work_tod_ms = tod;
    p->work_have = 0;
}

static void epoch_check(gnss_nmea_t *p)
{
    if ((p->work_have & (HAVE_RMC | HAVE_GGA | HAVE_GSA)) == (HAVE_RMC | HAVE_GGA | HAVE_GSA) &&
        !(p->work_have & HAVE_PUB)) {
        publish(p);
    }
}

/* ------------------------------------------------------------- zdania ----- */

static bool do_rmc(gnss_nmea_t *p, char **f, unsigned n)
{
    /* $xxRMC,time,status,lat,N,lon,E,spd,cog,date,mv,mvE[,mode][,navstatus] */
    if (n < 10u) return false;
    uint32_t tod; bool has_t;
    if (!parse_tod(f[1], &tod, &has_t) || !has_t) return false;

    bool active = (f[2][0] == 'A' && f[2][1] == '\0');
    if (!active && !(f[2][0] == 'V' && f[2][1] == '\0')) return false;

    int32_t lat = 0, lon = 0; bool has_lat, has_lon;
    if (!parse_coord(f[3], f[4], 90u, &lat, &has_lat)) return false;
    if (!parse_coord(f[5], f[6], 180u, &lon, &has_lon)) return false;

    int64_t kn = 0, cog = 0; bool has_kn, has_cog;
    if (!parse_fixed(f[7], 3, &kn, &has_kn) || kn < 0) return false;
    if (!parse_fixed(f[8], 2, &cog, &has_cog) || cog < 0 || cog >= 36000) return false;

    uint32_t date = 0; bool has_d;
    if (!parse_uint(f[9], 311299u, &date, &has_d)) return false;
    int64_t t = 0;
    if (has_d) {
        uint32_t dd = date / 10000u, mo = (date / 100u) % 100u, yy = date % 100u;
        if (dd < 1u || dd > 31u || mo < 1u || mo > 12u) return false;
        /* MTK przed pierwszym rozwiazaniem podaje date z 1980 r. */
        int32_t year = (yy >= 80u) ? (int32_t)(1900u + yy) : (int32_t)(2000u + yy);
        t = (int64_t)gnss_nmea_days_from_civil(year, mo, dd) * 86400 + (int64_t)(tod / 1000u);
        if (t <= 0 || t > 0xFFFFFFFFLL) t = 0;
    }

    epoch_enter(p, tod);
    gnss_fix_t *w = &p->work;

    w->ms = (uint16_t)(tod % 1000u);
    w->unix_s = (uint32_t)t;
    w->time_valid = active && (t > 0);
    if (active && has_lat && has_lon) {
        w->pos_valid = true;
        w->lat_e7 = lat;
        w->lon_e7 = lon;
    }
    if (has_kn) {
        int64_t cms = (kn * 514444 + 5000000) / 10000000;   /* 1 kn = 0.514444 m/s */
        w->speed_cms = (cms > 0xFFFF) ? 0xFFFFu : (uint16_t)cms;
    }
    if (has_cog) {
        w->course_valid = active;
        w->course_cdeg = (uint16_t)cog;
    }
    p->work_have |= HAVE_RMC;
    epoch_check(p);
    return true;
}

static bool do_gga(gnss_nmea_t *p, char **f, unsigned n)
{
    /* $xxGGA,time,lat,N,lon,E,qual,nsat,hdop,alt,M,sep,M,age,ref */
    if (n < 13u) return false;
    uint32_t tod; bool has_t;
    if (!parse_tod(f[1], &tod, &has_t) || !has_t) return false;

    int32_t lat = 0, lon = 0; bool has_lat, has_lon;
    if (!parse_coord(f[2], f[3], 90u, &lat, &has_lat)) return false;
    if (!parse_coord(f[4], f[5], 180u, &lon, &has_lon)) return false;

    uint32_t q = 0, ns = 0; bool has_q, has_ns;
    if (!parse_uint(f[6], 8u, &q, &has_q)) return false;
    if (!parse_uint(f[7], 99u, &ns, &has_ns)) return false;
    int64_t hdop = 0, alt = 0, sep = 0; bool has_h, has_a, has_s;
    if (!parse_fixed(f[8], 2, &hdop, &has_h) || hdop < 0 || hdop > 65535) return false;
    if (!parse_fixed(f[9], 2, &alt, &has_a) || alt < -100000000 || alt > 100000000) return false;
    if (!parse_fixed(f[11], 2, &sep, &has_s) || sep < -100000 || sep > 100000) return false;

    epoch_enter(p, tod);
    gnss_fix_t *w = &p->work;
    w->gga_quality = (uint8_t)q;
    w->sats_used = (uint8_t)ns;
    if (has_h) w->hdop_c = (uint16_t)hdop;
    if (q > 0u && has_lat && has_lon) {
        w->pos_valid = true;
        w->lat_e7 = lat;
        w->lon_e7 = lon;
    }
    if (q > 0u && has_a) {
        w->alt_valid = true;
        w->alt_msl_cm = (int32_t)alt;
        w->geoid_sep_cm = has_s ? (int32_t)sep : 0;
    }
    p->work_have |= HAVE_GGA;
    epoch_check(p);
    return true;
}

static bool do_gsa(gnss_nmea_t *p, char **f, unsigned n)
{
    /* $xxGSA,sel,mode,prn x12,pdop,hdop,vdop[,sysid] */
    if (n < 18u) return false;
    uint32_t mode = 0; bool has_m;
    if (!parse_uint(f[2], 3u, &mode, &has_m)) return false;
    int64_t pd = 0, hd = 0, vd = 0; bool hp, hh, hv;
    if (!parse_fixed(f[15], 2, &pd, &hp) || pd < 0 || pd > 65535) return false;
    if (!parse_fixed(f[16], 2, &hd, &hh) || hd < 0 || hd > 65535) return false;
    if (!parse_fixed(f[17], 2, &vd, &hv) || vd < 0 || vd > 65535) return false;

    /* GSA nie niesie czasu: nalezy do epoki, ktora juz sie zaczela. */
    if (!(p->work_have & (HAVE_RMC | HAVE_GGA)) || (p->work_have & HAVE_GSA)) return true;
    gnss_fix_t *w = &p->work;
    w->gsa_mode = has_m ? (uint8_t)mode : 0u;
    if (hp) w->pdop_c = (uint16_t)pd;
    if (hv) w->vdop_c = (uint16_t)vd;
    if (hh && w->hdop_c == 0u) w->hdop_c = (uint16_t)hd;
    p->work_have |= HAVE_GSA;
    epoch_check(p);
    return true;
}

static unsigned talker_index(const char *addr)
{
    if (addr[0] == 'G' && addr[1] == 'P') return 0;
    if (addr[0] == 'G' && addr[1] == 'L') return 1;
    if (addr[0] == 'G' && addr[1] == 'A') return 2;
    if ((addr[0] == 'B' && addr[1] == 'D') || (addr[0] == 'G' && addr[1] == 'B')) return 3;
    if ((addr[0] == 'G' && addr[1] == 'Q') || (addr[0] == 'Q' && addr[1] == 'Z')) return 4;
    return 5;
}

static void sky_update(gnss_nmea_t *p)
{
    unsigned view = 0, trk = 0, mx = 0, sum = 0;
    for (unsigned i = 0; i < GNSS_NMEA_TALKERS; i++) {
        const gnss_nmea_gsv_t *g = &p->gsv[i];
        if (!g->have || (p->epoch - g->epoch) > GNSS_NMEA_SKY_STALE) continue;
        view += g->c_in_view;
        trk  += g->c_tracked;
        sum  += g->c_cn0_sum;
        if (g->c_cn0_max > mx) mx = g->c_cn0_max;
    }
    p->sky.in_view = (uint8_t)(view > 255u ? 255u : view);
    p->sky.tracked = (uint8_t)(trk > 255u ? 255u : trk);
    p->sky.cn0_max = (uint8_t)mx;
    p->sky.cn0_avg = (uint8_t)(trk ? (sum + trk / 2u) / trk : 0u);
}

static bool do_gsv(gnss_nmea_t *p, const char *addr, char **f, unsigned n)
{
    /* $xxGSV,total,num,inview,{prn,elev,az,snr} x0..4[,signal] */
    if (n < 4u) return false;
    uint32_t total, num, view; bool a, b, c;
    if (!parse_uint(f[1], 9u, &total, &a) || !a || total == 0u) return false;
    if (!parse_uint(f[2], 9u, &num, &b) || !b || num == 0u || num > total) return false;
    if (!parse_uint(f[3], 99u, &view, &c)) return false;

    gnss_nmea_gsv_t *g = &p->gsv[talker_index(addr)];
    if (num == 1u) {
        g->expect_next = 1u;
        g->total = (uint8_t)total;
        g->in_view = (uint8_t)view;
        g->tracked = 0; g->cn0_max = 0; g->cn0_sum = 0;
    }
    if (g->expect_next != num || g->total != total) {
        g->expect_next = 0;          /* seria niekompletna — porzucona */
        return true;
    }
    for (unsigned i = 4u; i + 3u < n; i += 4u) {
        uint32_t snr; bool has;
        if (!parse_uint(f[i + 3u], 99u, &snr, &has)) return false;
        if (has && snr > 0u) {
            g->tracked++;
            g->cn0_sum = (uint16_t)(g->cn0_sum + snr);
            if (snr > g->cn0_max) g->cn0_max = (uint8_t)snr;
        }
    }
    if (num == total) {
        g->have = true;
        g->epoch = p->epoch;
        g->c_in_view = g->in_view;
        g->c_tracked = g->tracked;
        g->c_cn0_max = g->cn0_max;
        g->c_cn0_sum = g->cn0_sum;
        g->expect_next = 0;
        sky_update(p);
    } else {
        g->expect_next = (uint8_t)(num + 1u);
    }
    return true;
}

static bool do_pmtk(gnss_nmea_t *p, const char *addr, char **f, unsigned n)
{
    if (strcmp(addr, "PMTK001") == 0) {
        uint32_t cmd, flag; bool a, b;
        if (n < 3u) return false;
        if (!parse_uint(f[1], 999u, &cmd, &a) || !a) return false;
        if (!parse_uint(f[2], 3u, &flag, &b) || !b) return false;
        p->ack_cmd = (uint16_t)cmd;
        p->ack_flag = (uint8_t)flag;
        p->ack_new = true;
        return true;
    }
    if (strcmp(addr, "PMTK010") == 0) {
        p->startup_seen = true;
        return true;
    }
    return false;
}

/* ------------------------------------------------------------- API -------- */

void gnss_nmea_init(gnss_nmea_t *p)
{
    memset(p, 0, sizeof *p);
    p->work_tod_ms = TOD_NONE;
}

bool gnss_nmea_process_line(gnss_nmea_t *p, const char *line, size_t len)
{
    char buf[GNSS_NMEA_LINE_MAX + 1u];
    if (len < 6u || len > GNSS_NMEA_LINE_MAX || line[0] != '$') {
        p->cnt.err_format++;
        return false;
    }
    /* Suma: XOR bajtow miedzy '$' a '*', dwie cyfry hex i koniec linii. */
    size_t star = 0;
    uint8_t cs = 0;
    for (size_t i = 1; i < len; i++) {
        char ch = line[i];
        if (ch == '*') { star = i; break; }
        if ((uint8_t)ch < 0x20u || (uint8_t)ch > 0x7Eu || ch == '$') {
            p->cnt.err_format++;
            return false;
        }
        cs ^= (uint8_t)ch;
    }
    if (star == 0u || star + 3u != len) { p->cnt.err_format++; return false; }
    int h1 = hexval(line[star + 1u]), h2 = hexval(line[star + 2u]);
    if (h1 < 0 || h2 < 0) { p->cnt.err_format++; return false; }
    if ((uint8_t)((h1 << 4) | h2) != cs) { p->cnt.err_checksum++; return false; }

    /* Podzial na pola w kopii (bez '$' i sumy). */
    size_t body = star - 1u;
    memcpy(buf, line + 1, body);
    buf[body] = '\0';
    char *f[GNSS_NMEA_FIELDS_MAX];
    unsigned n = 0;
    f[n++] = buf;
    for (size_t i = 0; i < body; i++) {
        if (buf[i] == ',') {
            buf[i] = '\0';
            if (n >= GNSS_NMEA_FIELDS_MAX) { p->cnt.err_format++; return false; }
            f[n++] = &buf[i + 1u];
        }
    }

    const char *addr = f[0];
    size_t alen = strlen(addr);
    bool ok;
    bool known = true;
    if (alen == 5u && strcmp(addr + 2, "RMC") == 0)      ok = do_rmc(p, f, n);
    else if (alen == 5u && strcmp(addr + 2, "GGA") == 0) ok = do_gga(p, f, n);
    else if (alen == 5u && strcmp(addr + 2, "GSA") == 0) ok = do_gsa(p, f, n);
    else if (alen == 5u && strcmp(addr + 2, "GSV") == 0) ok = do_gsv(p, addr, f, n);
    else if (alen == 7u && strncmp(addr, "PMTK", 4) == 0) {
        ok = do_pmtk(p, addr, f, n);
        if (!ok) { known = false; ok = true; }
    } else { known = false; ok = true; }

    if (!ok) { p->cnt.err_format++; return false; }
    if (!known) { p->cnt.lines_ignored++; return false; }
    p->cnt.lines_ok++;
    return true;
}

void gnss_nmea_feed(gnss_nmea_t *p, const uint8_t *data, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        uint8_t ch = data[i];
        if (ch == '$') {                 /* poczatek zdania zawsze resynchronizuje */
            if (p->in_line && !p->discard) p->cnt.err_format++;   /* poprzednie uciete */
            p->in_line = true;
            p->discard = false;
            p->line[0] = '$';
            p->len = 1;
            continue;
        }
        if (!p->in_line) continue;       /* smieci miedzy zdaniami */
        if (ch == '\r') continue;
        if (ch == '\n') {
            if (!p->discard) {
                p->line[p->len] = '\0';
                (void)gnss_nmea_process_line(p, p->line, p->len);
            }
            p->in_line = false;
            p->len = 0;
            continue;
        }
        if (p->discard) continue;
        if (p->len >= GNSS_NMEA_LINE_MAX) {
            p->discard = true;
            p->cnt.err_overflow++;
            continue;
        }
        p->line[p->len++] = (char)ch;
    }
}

bool gnss_nmea_take_fix(gnss_nmea_t *p, gnss_fix_t *out)
{
    if (!p->fix_new) return false;
    *out = p->fix;
    p->fix_new = false;
    return true;
}

bool gnss_nmea_take_ack(gnss_nmea_t *p, uint16_t *cmd, uint8_t *flag)
{
    if (!p->ack_new) return false;
    *cmd = p->ack_cmd;
    *flag = p->ack_flag;
    p->ack_new = false;
    return true;
}

size_t gnss_nmea_build(char *out, size_t cap, const char *body)
{
    static const char hex[] = "0123456789ABCDEF";
    size_t blen = strlen(body);
    if (cap < blen + 7u) return 0;           /* $ + body + *HH + \r\n + \0 */
    uint8_t cs = 0;
    out[0] = '$';
    for (size_t i = 0; i < blen; i++) {
        char ch = body[i];
        if (ch == '$' || ch == '*' || (uint8_t)ch < 0x20u || (uint8_t)ch > 0x7Eu) return 0;
        cs ^= (uint8_t)ch;
        out[1u + i] = ch;
    }
    size_t k = 1u + blen;
    out[k++] = '*';
    out[k++] = hex[cs >> 4];
    out[k++] = hex[cs & 0x0Fu];
    out[k++] = '\r';
    out[k++] = '\n';
    out[k] = '\0';
    return k;
}
