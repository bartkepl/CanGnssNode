/* AUTO-GENEROWANE — NIE EDYTOWAC RECZNIE
 * 
 * Zrodla: CanBusCommon/protocol/common.yaml oraz registry/
 *         protocol/device.yaml
 * Regeneracja: python common/protocol/generate.py protocol/device.yaml
 * 
 * Urzadzenie      : CanGnssNode (DEVICE_TYPE 0x0003)
 * Profil wspolny  : 2
 * Rewizja         : 2.1
 * Skrot bazy      : 46c5b193bd6273b0
 */
#include "protocol_gen.h"

const mpc_param_desc_t mpc_param_table[MPC_PARAM_COUNT] = {
  { MPC_PARAM_NODE_ID, MPC_PTYPE_U8, MPC_ACC_RW, MPC_STORE_NV, MPC_PFLAG_MANDATORY, 32, 47, 32 },  /* Adres wezla na magistrali. Zakres przydzielony typowi urzadzenia - patrz registry/node_addresses.yaml. Zmiana dziala od razu. */
  { MPC_PARAM_TELEMETRY_PERIOD, MPC_PTYPE_U16, MPC_ACC_RW, MPC_STORE_NV, MPC_PFLAG_MANDATORY, 100, 60000, 2000 },  /* Okres cyklicznego rozglaszania telemetrii. */
  { MPC_PARAM_MEASURE_PERIOD, MPC_PTYPE_U16, MPC_ACC_RW, MPC_STORE_NV, 0, 100, 60000, 1000 },  /* Okres pelnego cyklu pomiarowego. Dolna granica moze byc podniesiona przez device.yaml, gdy peryferia tego wymagaja. */
  { MPC_PARAM_CAN_TERMINATION, MPC_PTYPE_U8, MPC_ACC_RW, MPC_STORE_NV, 0, 0, 1, 1 },  /* Rezystor terminujacy 120 Ohm na plytce. 1 = wlaczony (wezel na koncu magistrali). Ustaw 0 dla wezla w srodku. */
  { MPC_PARAM_LOW_POWER, MPC_PTYPE_U8, MPC_ACC_RW, MPC_STORE_NV, 0, 0, 1, 1 },  /* Usypianie rdzenia (WFI) miedzy zdarzeniami. Nie wplywa na timing. */
  { MPC_PARAM_CAL_LOCK, MPC_PTYPE_U8, MPC_ACC_RW, MPC_STORE_NV, 0, 0, 1, 1 },  /* Blokada zapisu parametrow kalibracyjnych. 1 = zablokowane (WRITE_PARAM na parametr oznaczony 'calibration' zwraca ERR_READONLY). Domyslnie WLACZONA - metrologia wezla nie moze byc zmieniona jedna przypadkowa ramka. Zdjecie blokady jest swiadoma, osobna czynnoscia operatora. */
  { MPC_PARAM_HEARTBEAT_PERIOD, MPC_PTYPE_U16, MPC_ACC_RW, MPC_STORE_NV, 0, 1000, 60000, 10000 },  /* Okres zdarzenia HEARTBEAT. Host uznaje wezel za utracony po braku HEARTBEAT przez 3 okresy. */
  { MPC_PARAM_TIME_SYNC_PERIOD, MPC_PTYPE_U16, MPC_ACC_RW, MPC_STORE_NV, 0, 0, 3600, 10 },  /* Okres nadawania SET_TIME_SYNC przez wzorzec czasu. 0 = wezel nie nadaje czasu (wzorzec wylaczony). */
  { MPC_PARAM_TIME_HOLDOVER, MPC_PTYPE_U16, MPC_ACC_RW, MPC_STORE_NV, 0, 0, 65535, 600 },  /* Jak dlugo po utracie wzorca (fix, PPS) zrodlo nadaje dalej z flaga HOLDOVER. Po tym czasie milknie. 0 = milknie od razu. */
  { MPC_PARAM_EN_GNSS, MPC_PTYPE_U8, MPC_ACC_RW, MPC_STORE_NV, 0, 0, 1, 1 },  /* Aktywacja odbiornika GNSS. 0 = modul w trybie Standby (PMTK161). */
  { MPC_PARAM_EN_IMU, MPC_PTYPE_U8, MPC_ACC_RW, MPC_STORE_NV, 0, 0, 1, 1 },  /* Aktywacja czujnika FXOS8700CQ i wielkosci orientacji. */
  { MPC_PARAM_GNSS_CONSTELLATIONS, MPC_PTYPE_U8, MPC_ACC_RW, MPC_STORE_NV, 0, 1, 15, 7 },  /* Konstelacje (PMTK353): bit0 GPS, bit1 GLONASS, bit2 Galileo, bit3 QZSS. */
  { MPC_PARAM_GNSS_SBAS, MPC_PTYPE_U8, MPC_ACC_RW, MPC_STORE_NV, 0, 0, 1, 1 },  /* Korekcje SBAS (EGNOS/WAAS/MSAS/GAGAN). */
  { MPC_PARAM_GNSS_POWER_POLICY, MPC_PTYPE_U8, MPC_ACC_RW, MPC_STORE_NV, 0, 0, 3, 0 },  /* Tryb pracy L76-L: 0 = automatycznie wg stanu ruchu (ADR 0003), 1 = stale wlaczony, 2 = okresowy (PMTK225), 3 = AlwaysLocate. */
  { MPC_PARAM_GNSS_MIN_SATS, MPC_PTYPE_U8, MPC_ACC_RW, MPC_STORE_NV, 0, 3, 12, 4 },  /* Minimalna liczba satelitow dla rozwiazania waznego (VALID). */
  { MPC_PARAM_GNSS_MAX_HDOP, MPC_PTYPE_U16, MPC_ACC_RW, MPC_STORE_NV, 0, 100, 5000, 500 },  /* Maksymalne HDOP dla rozwiazania waznego (VALID); 500 = 5.0. */
  { MPC_PARAM_STATIC_AVERAGING, MPC_PTYPE_U8, MPC_ACC_RW, MPC_STORE_NV, 0, 0, 1, 1 },  /* Usrednianie pozycji w stanie STATIC. */
  { MPC_PARAM_STATIC_AVG_WINDOW_S, MPC_PTYPE_U16, MPC_ACC_RW, MPC_STORE_NV, 0, 10, 3600, 300 },  /* Okno usredniania pozycji w spoczynku. */
  { MPC_PARAM_PRECISE_SYNC_PERIOD, MPC_PTYPE_U16, MPC_ACC_RW, MPC_STORE_NV, 0, 0, 60000, 1000 },  /* Okres pary TIME_SYNC / TIME_FOLLOW_UP (poziom 2, provisional). 0 = poziom 2 wylaczony. Nadawane wylacznie przy jakosci PPS. */
  { MPC_PARAM_IMU_SHOCK_MG, MPC_PTYPE_U16, MPC_ACC_RW, MPC_STORE_NV, 0, 500, 7900, 2500 },  /* Prog zdarzenia SHOCK (zakres FXOS +-8 g). */
  { MPC_PARAM_IMU_TILT_LIMIT, MPC_PTYPE_U16, MPC_ACC_RW, MPC_STORE_NV, 0, 100, 9000, 3000 },  /* Prog zdarzenia TILT_ALARM (pochylenie lub przechylenie). */
  { MPC_PARAM_IMU_STATIC_MG, MPC_PTYPE_U16, MPC_ACC_RW, MPC_STORE_NV, 0, 5, 500, 30 },  /* Prog odchylenia przyspieszenia, ponizej ktorego wezel uznaje spoczynek. */
  { MPC_PARAM_IMU_STATIC_S, MPC_PTYPE_U16, MPC_ACC_RW, MPC_STORE_NV, 0, 5, 600, 30 },  /* Czas spelnienia warunku spoczynku przed przejsciem w STATIC. */
  { MPC_PARAM_MOVING_SPEED, MPC_PTYPE_U16, MPC_ACC_RW, MPC_STORE_NV, 0, 50, 3000, 300 },  /* Predkosc GNSS, powyzej ktorej stan ruchu to MOVING (jazda). */
  { MPC_PARAM_MAG_DECLINATION, MPC_PTYPE_I16, MPC_ACC_RW, MPC_STORE_NV, 0, -3000, 3000, 0 },  /* Deklinacja magnetyczna miejsca instalacji (dodatnia = wschodnia). Kurs geograficzny = HEADING_MAG + deklinacja; wielkosc HEADING_MAG pozostaje kursem magnetycznym. */
  { MPC_PARAM_CAL_MAG_OFF_X, MPC_PTYPE_F32, MPC_ACC_RW, MPC_STORE_NV, MPC_PFLAG_CALIBRATION, -1007026176, 1140457472, 0 },  /* Offset hard-iron magnetometru, os X wezla. */
  { MPC_PARAM_CAL_MAG_OFF_Y, MPC_PTYPE_F32, MPC_ACC_RW, MPC_STORE_NV, MPC_PFLAG_CALIBRATION, -1007026176, 1140457472, 0 },  /* Offset hard-iron magnetometru, os Y wezla. */
  { MPC_PARAM_CAL_MAG_OFF_Z, MPC_PTYPE_F32, MPC_ACC_RW, MPC_STORE_NV, MPC_PFLAG_CALIBRATION, -1007026176, 1140457472, 0 },  /* Offset hard-iron magnetometru, os Z wezla. */
  { MPC_PARAM_CAL_MOUNT_ORIENT, MPC_PTYPE_U8, MPC_ACC_RW, MPC_STORE_NV, MPC_PFLAG_CALIBRATION, 0, 23, 0 },  /* Orientacja montazu: jeden z 24 obrotow o wielokrotnosc 90 st przeksztalcajacy osie czujnika na osie wezla (tablica w ADR 0004). */
  { MPC_PARAM_CAL_VIN_C0, MPC_PTYPE_F32, MPC_ACC_RW, MPC_STORE_NV, MPC_PFLAG_CALIBRATION, -1082130432, 1065353216, 0 },  /* Napiecie zasilania: offset c0 (V = c1 * V_nom + c0). */
  { MPC_PARAM_CAL_VIN_C1, MPC_PTYPE_F32, MPC_ACC_RW, MPC_STORE_NV, MPC_PFLAG_CALIBRATION, 1056964608, 1069547520, 1065353216 },  /* Napiecie zasilania: wzmocnienie c1 wzgledem nominalnego dzielnika 1M/100k. */
  { MPC_PARAM_CAL_ISENS_C0, MPC_PTYPE_F32, MPC_ACC_RW, MPC_STORE_NV, MPC_PFLAG_CALIBRATION, -1165815185, 981668463, 0 },  /* Prad zasilania: offset c0 (I = c1 * I_nom + c0). */
  { MPC_PARAM_CAL_ISENS_C1, MPC_PTYPE_F32, MPC_ACC_RW, MPC_STORE_NV, MPC_PFLAG_CALIBRATION, 1056964608, 1069547520, 1065353216 },  /* Prad zasilania: wzmocnienie c1 wzgledem nominalnego INA186A4 (G = 200, shunt 0.5 Ohm). */
};

const uint8_t mpc_sensor_ids[MPC_SENSOR_COUNT] = { MPC_SENSOR_GNSS, MPC_SENSOR_FXOS8700CQ, MPC_SENSOR_ORIENTATION, MPC_SENSOR_SUPPLY_MON, MPC_SENSOR_MCU };

const uint8_t mpc_channel_ids[MPC_CHANNEL_COUNT] = { MPC_CH_LATITUDE, MPC_CH_LONGITUDE, MPC_CH_ALTITUDE, MPC_CH_SPEED, MPC_CH_COURSE, MPC_CH_SATELLITES, MPC_CH_HDOP, MPC_CH_FIX_TYPE, MPC_CH_PITCH, MPC_CH_ROLL, MPC_CH_HEADING_MAG, MPC_CH_MOTION_STATE, MPC_CH_SUPPLY_VOLTAGE, MPC_CH_SUPPLY_CURRENT, MPC_CH_MCU_TEMP, MPC_CH_VDDA };

