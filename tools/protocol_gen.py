"""AUTO-GENEROWANE — NIE EDYTOWAC RECZNIE

Zrodla: CanBusCommon/protocol/common.yaml oraz registry/
        protocol/device.yaml
Regeneracja: python common/protocol/generate.py protocol/device.yaml

Urzadzenie      : CanGnssNode (DEVICE_TYPE 0x0003)
Profil wspolny  : 2
Rewizja         : 2.1
Skrot bazy      : 46c5b193bd6273b0"""
from __future__ import annotations
import enum
import struct
from dataclasses import dataclass

PROFILE_VERSION = 2
PROTO_VERSION = 1
STANDARD_REVISION = "2.1"
COMMON_BASE_HASH = "46c5b193bd6273b0"
DEVICE_TYPE = 0x0003
DEVICE_NAME = "CanGnssNode"
FW_VERSION_MAJOR = 0
FW_VERSION_MINOR = 1
HW_VERSION_MAJOR = 1
HW_VERSION_MINOR = 0
BUILD_REVISION = 1
CAPABILITIES = 0x001237FF
CAN_BITRATE = 500000
CAN_ID_BITS = 29
CAN_MAX_DLC = 8

NODE_BROADCAST = 0xFF
NODE_UNCONFIGURED = 0x00
NODE_HOST = 0xFE
NODE_DEFAULT = 0x20

# ---- Uklad 29-bitowego identyfikatora ----
FUNC_SHIFT = 24
FUNC_MASK = 0x1F
NODE_SHIFT = 16
NODE_MASK = 0xFF
OBJ_SHIFT = 8
OBJ_MASK = 0xFF
ARG_SHIFT = 0
ARG_MASK = 0xFF

def id_pack(func: int, node: int, obj: int, arg: int) -> int:
    return (((func & FUNC_MASK) << FUNC_SHIFT) | ((node & NODE_MASK) << NODE_SHIFT)
            | ((obj & OBJ_MASK) << OBJ_SHIFT) | ((arg & ARG_MASK) << ARG_SHIFT))

def id_unpack(ident: int) -> tuple[int, int, int, int]:
    return ((ident >> FUNC_SHIFT) & FUNC_MASK, (ident >> NODE_SHIFT) & NODE_MASK,
            (ident >> OBJ_SHIFT) & OBJ_MASK, (ident >> ARG_SHIFT) & ARG_MASK)

class Func(enum.IntEnum):
    """Klasy komunikatow; mniejsza wartosc = wyzszy priorytet."""
    EVENT = 0x01
    REQUEST = 0x08
    RESPONSE = 0x09
    STREAM = 0x0A
    TELEMETRY = 0x10

class Channel(enum.IntEnum):
    """Kanaly telemetrii — id z globalnego rejestru wielkosci."""
    LATITUDE = 0x50
    LONGITUDE = 0x51
    ALTITUDE = 0x52
    SPEED = 0x53
    COURSE = 0x54
    SATELLITES = 0x55
    HDOP = 0x56
    FIX_TYPE = 0x57
    PITCH = 0x66
    ROLL = 0x67
    HEADING_MAG = 0x68
    MOTION_STATE = 0x69
    SUPPLY_VOLTAGE = 0x12
    SUPPLY_CURRENT = 0x13
    MCU_TEMP = 0x0A
    VDDA = 0x0B

class Sensor(enum.IntEnum):
    GNSS = 0x20
    FXOS8700CQ = 0x21
    ORIENTATION = 0x23
    SUPPLY_MON = 0x11
    MCU = 0x0C

class Quantity(enum.IntEnum):
    MCU_TEMP = 0x0A
    VDDA = 0x0B
    VBAT = 0x0C
    SUPPLY_VOLTAGE = 0x12
    SUPPLY_CURRENT = 0x13
    LATITUDE = 0x50
    LONGITUDE = 0x51
    ALTITUDE = 0x52
    SPEED = 0x53
    COURSE = 0x54
    SATELLITES = 0x55
    HDOP = 0x56
    FIX_TYPE = 0x57
    ACCEL_X = 0x60
    ACCEL_Y = 0x61
    ACCEL_Z = 0x62
    MAG_X = 0x63
    MAG_Y = 0x64
    MAG_Z = 0x65
    PITCH = 0x66
    ROLL = 0x67
    HEADING_MAG = 0x68
    MOTION_STATE = 0x69

class ReqOp(enum.IntEnum):
    PING = 0x00
    GET_INFO = 0x01
    READ_AVG = 0x02
    READ_SENSOR = 0x03
    READ_PARAM = 0x04
    WRITE_PARAM = 0x05
    LIST_SENSORS = 0x06
    SAVE_CONFIG = 0x07
    LOAD_DEFAULTS = 0x08
    GET_TIME = 0x09
    SET_TIME = 0x0A
    GET_STATUS = 0x0B
    RESET = 0x0C
    ENTER_BOOTLOADER = 0x0D
    GET_CAPABILITIES = 0x12
    LIST_PARAMS = 0x13
    LIST_CHANNELS = 0x14
    SET_TIME_SYNC = 0x15
    GET_COUNTERS = 0x16
    GET_STATUS_EXT = 0x20
    SELF_TEST = 0x21
    CLEAR_COUNTERS = 0x22
    READ_FIX = 0x80
    GET_GNSS_TIME = 0x81
    GET_SKY = 0x82
    GNSS_RESTART = 0x83
    MAG_CAL = 0x84

class Status(enum.IntEnum):
    OK = 0x00
    ERR_UNKNOWN_CMD = 0x01
    ERR_BAD_PARAM = 0x02
    ERR_OUT_OF_RANGE = 0x03
    ERR_SENSOR_ABSENT = 0x04
    ERR_SENSOR_FAULT = 0x05
    ERR_BUSY = 0x06
    ERR_READONLY = 0x07
    ERR_NOT_READY = 0x08
    ERR_CRC = 0x09

class EventSrc(enum.IntEnum):
    """Zrodla zdarzen (pole OBJ przy FUNC=EVENT); pozostale wartosci to id czujnika."""
    SYSTEM = 0x00

class EventCode(enum.IntEnum):
    BOOT = 0x01
    SENSOR_ALERT = 0x03
    SENSOR_FAULT = 0x04
    SENSOR_BACK = 0x05
    BUS_RECOVERED = 0x06
    HEARTBEAT = 0x07
    SENSOR_IRQ = 0x08
    CRITICAL_ALARM = 0x09
    SENSOR_RANGE = 0x0C
    ADDR_CONFLICT = 0x0D
    CONFIG_CHANGED = 0x0E
    TIME_SYNC = 0x20
    TIME_FOLLOW_UP = 0x21
    FIX_ACQUIRED = 0x80
    FIX_LOST = 0x81
    SITUATION_CHANGED = 0x82
    SHOCK = 0x83
    FREEFALL = 0x84
    TILT_ALARM = 0x85

class ReadingStatus(enum.IntEnum):
    OK = 0x00
    WARN_LOW = 0x01
    WARN_HIGH = 0x02
    ERR_LOW = 0x03
    ERR_HIGH = 0x04

class SensorStatus(enum.IntEnum):
    PRESENT = 0x00
    ABSENT = 0x01
    FAULT = 0x02
    DISABLED = 0x03

class Capability(enum.IntEnum):
    """Bitmapa zglaszana w GET_INFO / GET_CAPABILITIES."""
    TELEMETRY = 0x01
    SENSORS = 0x02
    PARAMS_NV = 0x04
    RTC = 0x08
    BOOTLOADER = 0x10
    TIME_SYNC = 0x20
    LIST_PARAMS = 0x40
    LIST_CHANNELS = 0x80
    COUNTERS = 0x100
    SELF_TEST = 0x200
    CAL_LOCK = 0x400
    RANGE_LIMITS = 0x800
    ADDR_CONFLICT_DETECT = 0x1000
    STATUS_EXT = 0x2000
    SENSOR_FUSION = 0x4000
    OUTPUTS = 0x8000
    DISPLAY = 0x10000
    GNSS = 0x20000
    RADIOMETRY = 0x40000
    LIGHTNING = 0x80000
    TIME_SOURCE = 0x100000
    TIME_SYNC_PRECISE = 0x200000

# ---- Bajty zabezpieczajace komend destrukcyjnych ----
GUARD = {
    ReqOp.LOAD_DEFAULTS: 0x5A,
    ReqOp.RESET: 0xA5,
    ReqOp.ENTER_BOOTLOADER: 0xB0,
    ReqOp.GNSS_RESTART: 0xC5,
}

# ---- Uklad pola danych ramki TELEMETRY ----
TLM_OFF_SEQ = 0
TLM_OFF_FLAGS = 1
TLM_OFF_VALUE = 2
TLM_OFF_QUAL = 6
TLM_FLAG_VALID = 1 << 0
TLM_FLAG_CLAMPED = 1 << 5
TLM_FLAG_RANGE_WARN = 1 << 6
TLM_FLAG_RANGE_ERR_DROPPED = 1 << 7
TLM_SRC_SHIFT = 1
TLM_SRC_MASK = 0x0F

STREAM_LAST = 1 << 7
STREAM_IMASK = 0x7F
STREAM_MAX_PAYLOAD = 7
RESPONSE_MAX_DATA = 7

EVENT_PAYLOADS = {
    'BOOT': {'len': 6, 'fields': [('RESET_CAUSE', 0, 'u8'), ('PROTO_VERSION', 1, 'u8'), ('LAST_FAULT', 2, 'u8'), ('FAULT_COUNT', 3, 'u8'), ('BOOT_COUNT', 4, 'u8'), ('POST_MASK', 5, 'u8')], 'meanings': {'RESET_CAUSE': {'bits': {'LPWR': 0, 'WWDG': 1, 'IWDG': 2, 'SOFT': 3, 'BOR_POR': 4, 'PIN': 5, 'OBL': 6}}, 'POST_MASK': {'bits': {'RAM': 0, 'FLASH': 1, 'CPU': 2, 'CLOCK': 3}}}},
    'HEARTBEAT': {'len': 7, 'fields': [('UPTIME_S', 0, 'u16'), ('BUSOFF', 2, 'u8'), ('I2C_ERR', 3, 'u8'), ('CAN_DROP', 4, 'u8'), ('SENSORS', 5, 'u8'), ('FLAGS', 6, 'u8')], 'meanings': {'FLAGS': {'bits': {'LSE': 0, 'VDD_LOW': 1, 'STACK_LOW': 2, 'RESET_LOOP': 3, 'SELFTEST': 4, 'TIME_SYNCED': 5}}}},
    'SENSOR_IRQ': {'len': 1, 'fields': [('LINES', 0, 'u8')], 'meanings': {'LINES': {'bits': {'IMU_INT': 0}}}},
    'CRITICAL_ALARM': {'len': 2, 'fields': [('SYS', 0, 'u8'), ('ERR', 1, 'u8')], 'meanings': {}},
    'SENSOR_RANGE': {'len': 6, 'fields': [('QTY', 0, 'u8'), ('READING_STATUS', 1, 'u8'), ('VALUE', 2, 'i32')], 'meanings': {}},
    'ADDR_CONFLICT': {'len': 1, 'fields': [('NODE', 0, 'u8')], 'meanings': {}},
    'TIME_SYNC': {'len': 2, 'fields': [('SEQ', 0, 'u8'), ('QUALITY', 1, 'u8')], 'meanings': {'QUALITY': {'bits': {'FIX': 0, 'PPS': 1, 'HOLDOVER': 2}}}},
    'TIME_FOLLOW_UP': {'len': 8, 'fields': [('SEQ', 0, 'u8'), ('UNIX', 1, 'u32'), ('USEC', 5, 'u24')], 'meanings': {}},
    'CONFIG_CHANGED': {'len': 2, 'fields': [('PARAM_ID', 0, 'u8'), ('SOURCE', 1, 'u8')], 'meanings': {'SOURCE': {'values': {'CAN': 0, 'LOCAL': 1, 'AUTO': 2}}}},
    'FIX_ACQUIRED': {'len': 4, 'fields': [('FIX_TYPE', 0, 'u8'), ('SATELLITES', 1, 'u8'), ('TTFF_S', 2, 'u16')], 'meanings': {}},
    'SITUATION_CHANGED': {'len': 2, 'fields': [('OLD', 0, 'u8'), ('NEW', 1, 'u8')], 'meanings': {'OLD': {'values': {'UNKNOWN': 0, 'STATIC': 1, 'HANDLING': 2, 'MOVING': 3}}, 'NEW': {'values': {'UNKNOWN': 0, 'STATIC': 1, 'HANDLING': 2, 'MOVING': 3}}}},
    'SHOCK': {'len': 3, 'fields': [('AXES', 0, 'u8'), ('PEAK_MG', 1, 'u16')], 'meanings': {'AXES': {'bits': {'X': 0, 'Y': 1, 'Z': 2}}}},
    'FREEFALL': {'len': 2, 'fields': [('DURATION_MS', 0, 'u16')], 'meanings': {}},
    'TILT_ALARM': {'len': 5, 'fields': [('STATE', 0, 'u8'), ('PITCH', 1, 'i16'), ('ROLL', 3, 'i16')], 'meanings': {'STATE': {'values': {'CLEARED': 0, 'EXCEEDED': 1}}}},
}

EVENTS_WITHOUT_PAYLOAD = ['SENSOR_ALERT', 'SENSOR_FAULT', 'SENSOR_BACK', 'BUS_RECOVERED', 'FIX_LOST']

RECORD_LAYOUTS = {
    'sensor_entry': {'len': 2, 'fields': [('SENSOR', 0, 'u8'), ('STATUS', 1, 'u8')], 'meanings': {}},
    'channel_entry': {'len': 2, 'fields': [('QUANTITY', 0, 'u8'), ('FLAGS', 1, 'u8')], 'meanings': {}},
    'param_entry': {'len': 3, 'fields': [('ID', 0, 'u8'), ('TYPE', 1, 'u8'), ('FLAGS', 2, 'u8')], 'meanings': {}},
    'sensor_reading': {'len': 6, 'fields': [('QTY', 0, 'u8'), ('VALUE', 1, 'i32'), ('READING_STATUS', 5, 'u8')], 'meanings': {}},
}

RECORD_OF_OPCODE = {
    0x03: 'sensor_reading',   # READ_SENSOR
    0x06: 'sensor_entry',   # LIST_SENSORS
    0x13: 'param_entry',   # LIST_PARAMS
    0x14: 'channel_entry',   # LIST_CHANNELS
}

PARAM_TYPE_CODES = {0: 'u8', 1: 'u16', 2: 'u32', 3: 'i16', 4: 'i32', 5: 'f32'}

STATUS_LEN = 7
STATUS_OFF_SYS = 0
STATUS_OFF_ERR = 1
STATUS_OFF_PRESENT = 2
STATUS_OFF_FAULT = 4
STATUS_OFF_NHEALTHY = 6
STATUS_SYS_LSE_ACTIVE = 1 << 0
STATUS_SYS_RTC_OK = 1 << 1
STATUS_SYS_CAN_OK = 1 << 2
STATUS_SYS_I2C_OK = 1 << 3
STATUS_SYS_ADC_OK = 1 << 4
STATUS_SYS_FPU_OK = 1 << 5
STATUS_SYS_CONFIG_OK = 1 << 6
STATUS_SYS_SENSORS_OK = 1 << 7
STATUS_ERR_CRITICAL = 1 << 0
STATUS_ERR_SENSOR_FAULT = 1 << 1
STATUS_ERR_NO_SENSORS = 1 << 2
STATUS_ERR_WDT_RESET = 1 << 3
STATUS_ERR_NVM_ERROR = 1 << 4
STATUS_ERR_CAN_BUSOFF = 1 << 5
STATUS_ERR_I2C_RECOVERED = 1 << 6
STATUS_ERR_VBAT_LOW = 1 << 7

STATUS_EXT_LEN = 13
STATUS_EXT_LEN_BASE = 10
STATUS_EXT_OFF_SYS2 = 0
STATUS_EXT_OFF_ERR2 = 1
STATUS_EXT_OFF_UPTIME_S = 2
STATUS_EXT_OFF_BOOT_COUNT = 6
STATUS_EXT_OFF_LAST_FAULT = 8
STATUS_EXT_OFF_POST_MASK = 9
STATUS_EXT_OFF_TIME_SOURCE = 10
STATUS_EXT_OFF_TIME_AGE_S = 11
STATUS_EXT_ERR2_ADDR_CONFLICT = 1 << 0
STATUS_EXT_ERR2_CAL_LOCKED = 1 << 1
STATUS_EXT_ERR2_TIME_UNSYNCED = 1 << 2
STATUS_EXT_ERR2_RESET_LOOP = 1 << 3

COUNTERS_LEN = 16
COUNTERS_OFF_RX_FRAMES = 0
COUNTERS_OFF_TX_FRAMES = 4
COUNTERS_OFF_TX_DROPPED = 8
COUNTERS_OFF_RX_DROPPED = 10
COUNTERS_OFF_BUSOFF = 12
COUNTERS_OFF_I2C_ERR = 14

INFO_LEN = 28
INFO_OFF_PROTO_VERSION = 0
INFO_OFF_FW_MAJOR = 1
INFO_OFF_FW_MINOR = 2
INFO_OFF_NODE = 3
INFO_OFF_HW_MAJOR = 4
INFO_OFF_HW_MINOR = 5
INFO_OFF_BUILD_REVISION = 6
INFO_OFF_BUILD_FLAGS = 8
INFO_OFF_UID = 9
INFO_OFF_DEVICE_TYPE = 21
INFO_OFF_PROFILE_VERSION = 23
INFO_OFF_CAPABILITIES = 24
INFO_FLAG_DEBUG = 1 << 0

READ_AVG_LEN = 5
READ_AVG_OFF_FLAGS = 0
READ_AVG_OFF_VALUE = 1

TIME_LEN = 7
TIME_OFF_SEC = 0
TIME_OFF_MIN = 1
TIME_OFF_HOUR = 2
TIME_OFF_DAY = 3
TIME_OFF_MONTH = 4
TIME_OFF_YEAR = 5
TIME_OFF_WDAY = 6

TIME_SYNC_LEN = 7
TIME_SYNC_LEN_BASE = 6
TIME_SYNC_OFF_UNIX = 0
TIME_SYNC_OFF_MS = 4
TIME_SYNC_OFF_QUALITY = 6

TIME_SYNC_QUALITY_FIX = 1 << 0
TIME_SYNC_QUALITY_PPS = 1 << 1
TIME_SYNC_QUALITY_HOLDOVER = 1 << 2
TIME_SOURCE_UNSPECIFIED = 0x00
TIME_SOURCE_HOST = 0xFE
TIME_SOURCE_NONE = 0xFF
TIME_SYNC_LEGACY_MIN_LEN = 4
TIME_SYNC_SILENCE_PERIODS = 2
TIME_SYNC_VALID_S = 3600
TIME_SYNC_FOLLOW_UP_WINDOW_MS = 100

# Status kazdego numeru zakresu bootloadera (ADR 0006).
BOOTLOADER_OPCODES = {
    0x70: ('HELLO', 'in_service'),
    0x71: ('ERASE', 'in_service'),
    0x72: ('PROG_START', 'in_service'),
    0x73: ('PROG_DATA', 'in_service'),
    0x74: ('PROG_END', 'in_service'),
    0x75: ('VERIFY', 'in_service'),
    0x76: ('GO', 'in_service'),
    0x77: (None, 'reserved'),
    0x78: ('PROG_BLOCK', 'retired'),
}

class StatusSys(enum.IntFlag):
    LSE_ACTIVE = 1 << 0
    RTC_OK = 1 << 1
    CAN_OK = 1 << 2
    I2C_OK = 1 << 3
    ADC_OK = 1 << 4
    FPU_OK = 1 << 5
    CONFIG_OK = 1 << 6
    SENSORS_OK = 1 << 7

STATUS_SYS_DESC = {
    StatusSys.LSE_ACTIVE: 'Zegar LSE + autotrim MSI aktywny',
    StatusSys.RTC_OK: 'RTC obecny (oscylator wystartowal)',
    StatusSys.CAN_OK: 'Sterownik CAN zainicjowany i pracuje',
    StatusSys.I2C_OK: 'Magistrala I2C zainicjowana i pracuje',
    StatusSys.ADC_OK: 'Wewnetrzny ADC sprawny',
    StatusSys.FPU_OK: 'FPU wlaczone (CP10/CP11 w CPACR)',
    StatusSys.CONFIG_OK: 'Konfiguracja zaladowana i wazna',
    StatusSys.SENSORS_OK: 'Przynajmniej jedno zrodlo dostarcza dane',
}

class StatusErr(enum.IntFlag):
    CRITICAL = 1 << 0
    SENSOR_FAULT = 1 << 1
    NO_SENSORS = 1 << 2
    WDT_RESET = 1 << 3
    NVM_ERROR = 1 << 4
    CAN_BUSOFF = 1 << 5
    I2C_RECOVERED = 1 << 6
    VBAT_LOW = 1 << 7

STATUS_ERR_DESC = {
    StatusErr.CRITICAL: 'Blad krytyczny aktywny (wyzwala EVENT CRITICAL_ALARM)',
    StatusErr.SENSOR_FAULT: 'Co najmniej jeden czujnik w awarii',
    StatusErr.NO_SENSORS: 'Zadne zrodlo nie dostarcza danych',
    StatusErr.WDT_RESET: 'Ostatni reset od watchdoga (IWDG/WWDG)',
    StatusErr.NVM_ERROR: 'Blad zapisu/odczytu pamieci nieulotnej',
    StatusErr.CAN_BUSOFF: 'Wystapil i zostal odzyskany bus-off CAN',
    StatusErr.I2C_RECOVERED: 'Wykonano odzysk zawieszonej magistrali I2C',
    StatusErr.VBAT_LOW: 'Napiecie VBAT ponizej progu',
}

class StatusErr2(enum.IntFlag):
    ADDR_CONFLICT = 1 << 0
    CAL_LOCKED = 1 << 1
    TIME_UNSYNCED = 1 << 2
    RESET_LOOP = 1 << 3

STATUS_ERR2_DESC = {
    StatusErr2.ADDR_CONFLICT: 'Wykryto inny wezel o tym samym adresie - telemetria wstrzymana',
    StatusErr2.CAL_LOCKED: 'Parametry kalibracyjne zablokowane (CAL_LOCK=1)',
    StatusErr2.TIME_UNSYNCED: 'RTC nie byl synchronizowany od startu',
    StatusErr2.RESET_LOOP: 'Wykryto petle resetow',
}

class InfoFlags(enum.IntFlag):
    DEBUG = 1 << 0

INFO_FLAGS_DESC = {
    InfoFlags.DEBUG: 'Build Debug - NIE uzywac w produkcji',
}

# ---- Skale (wartosc_fizyczna = raw * skala) ----
SCALES = {
    0x0A: 0.01,  # MCU_TEMP [degC]
    0x0B: 0.001,  # VDDA [V]
    0x0C: 0.001,  # VBAT [V]
    0x12: 0.001,  # SUPPLY_VOLTAGE [V]
    0x13: 1e-06,  # SUPPLY_CURRENT [A]
    0x50: 1e-07,  # LATITUDE [deg]
    0x51: 1e-07,  # LONGITUDE [deg]
    0x52: 0.01,  # ALTITUDE [m]
    0x53: 0.01,  # SPEED [m/s]
    0x54: 0.01,  # COURSE [deg]
    0x55: 1.0,  # SATELLITES [-]
    0x56: 0.01,  # HDOP [-]
    0x57: 1.0,  # FIX_TYPE [-]
    0x60: 0.001,  # ACCEL_X [m/s2]
    0x61: 0.001,  # ACCEL_Y [m/s2]
    0x62: 0.001,  # ACCEL_Z [m/s2]
    0x63: 0.01,  # MAG_X [uT]
    0x64: 0.01,  # MAG_Y [uT]
    0x65: 0.01,  # MAG_Z [uT]
    0x66: 0.01,  # PITCH [deg]
    0x67: 0.01,  # ROLL [deg]
    0x68: 0.01,  # HEADING_MAG [deg]
    0x69: 1.0,  # MOTION_STATE [-]
}

UNITS = {
    0x0A: 'degC',
    0x0B: 'V',
    0x0C: 'V',
    0x12: 'V',
    0x13: 'A',
    0x50: 'deg',
    0x51: 'deg',
    0x52: 'm',
    0x53: 'm/s',
    0x54: 'deg',
    0x55: '-',
    0x56: '-',
    0x57: '-',
    0x60: 'm/s2',
    0x61: 'm/s2',
    0x62: 'm/s2',
    0x63: 'uT',
    0x64: 'uT',
    0x65: 'uT',
    0x66: 'deg',
    0x67: 'deg',
    0x68: 'deg',
    0x69: '-',
}

def scale_of(qty: int) -> float:
    return SCALES.get(int(qty), 1.0)

def unit_of(qty: int) -> str:
    return UNITS.get(int(qty), '?')

def telemetry_unpack(data: bytes) -> dict:
    """Rozpakowuje pole danych ramki TELEMETRY (8 B)."""
    buf = bytes(data).ljust(8, b'\x00')
    flags = buf[TLM_OFF_FLAGS]
    return {
        'seq': buf[TLM_OFF_SEQ],
        'flags': flags,
        'value': int.from_bytes(buf[TLM_OFF_VALUE:TLM_OFF_VALUE + 4], 'little', signed=True),
        'qual': int.from_bytes(buf[TLM_OFF_QUAL:TLM_OFF_QUAL + 2], 'little'),
        'valid': bool(flags & TLM_FLAG_VALID),
        'clamped': bool(flags & TLM_FLAG_CLAMPED),
        'range_warn': bool(flags & TLM_FLAG_RANGE_WARN),
        'range_err_dropped': bool(flags & TLM_FLAG_RANGE_ERR_DROPPED),
        'n_sensors': (flags >> TLM_SRC_SHIFT) & TLM_SRC_MASK,
    }

def telemetry_pack(seq: int, value: int, n_sensors: int = 0, valid: bool = True,
                   clamped: bool = False, range_warn: bool = False,
                   range_err_dropped: bool = False, qual: int = 0) -> bytes:
    """Buduje pole danych ramki TELEMETRY (8 B). Uzywane przez symulatory."""
    flags = ((n_sensors & TLM_SRC_MASK) << TLM_SRC_SHIFT)
    if valid:             flags |= TLM_FLAG_VALID
    if clamped:           flags |= TLM_FLAG_CLAMPED
    if range_warn:        flags |= TLM_FLAG_RANGE_WARN
    if range_err_dropped: flags |= TLM_FLAG_RANGE_ERR_DROPPED
    return (bytes([seq & 0xFF, flags & 0xFF])
            + int(value).to_bytes(4, 'little', signed=True)
            + int(qual).to_bytes(2, 'little'))

@dataclass(frozen=True)
class ParamDesc:
    id: int
    name: str
    type: str
    access: str
    storage: str
    min: float
    max: float
    default: float
    unit: str
    calibration: bool
    desc: str

    @property
    def struct_fmt(self) -> str:
        """Format struct dla wartosci tego parametru (little-endian)."""
        return '<' + PY_STRUCT[self.type]

    @property
    def size(self) -> int:
        """Rozmiar wartosci w bajtach."""
        return struct.calcsize(self.struct_fmt)

PARAMS: list[ParamDesc] = [
    ParamDesc(0x01, 'NODE_ID', 'u8', 'rw', 'nv', 32, 47, 32, '-', False, 'Adres wezla na magistrali. Zakres przydzielony typowi urzadzenia - patrz registry/node_addresses.yaml. Zmiana dziala od razu.'),
    ParamDesc(0x02, 'TELEMETRY_PERIOD', 'u16', 'rw', 'nv', 100, 60000, 2000, 'ms', False, 'Okres cyklicznego rozglaszania telemetrii.'),
    ParamDesc(0x03, 'MEASURE_PERIOD', 'u16', 'rw', 'nv', 100, 60000, 1000, 'ms', False, 'Okres pelnego cyklu pomiarowego. Dolna granica moze byc podniesiona przez device.yaml, gdy peryferia tego wymagaja.'),
    ParamDesc(0x04, 'CAN_TERMINATION', 'u8', 'rw', 'nv', 0, 1, 1, '0/1', False, 'Rezystor terminujacy 120 Ohm na plytce. 1 = wlaczony (wezel na koncu magistrali). Ustaw 0 dla wezla w srodku.'),
    ParamDesc(0x07, 'LOW_POWER', 'u8', 'rw', 'nv', 0, 1, 1, '0/1', False, 'Usypianie rdzenia (WFI) miedzy zdarzeniami. Nie wplywa na timing.'),
    ParamDesc(0x09, 'CAL_LOCK', 'u8', 'rw', 'nv', 0, 1, 1, '0/1', False, 'Blokada zapisu parametrow kalibracyjnych. 1 = zablokowane (WRITE_PARAM na parametr oznaczony \'calibration\' zwraca ERR_READONLY). Domyslnie WLACZONA - metrologia wezla nie moze byc zmieniona jedna przypadkowa ramka. Zdjecie blokady jest swiadoma, osobna czynnoscia operatora.'),
    ParamDesc(0x0A, 'HEARTBEAT_PERIOD', 'u16', 'rw', 'nv', 1000, 60000, 10000, 'ms', False, 'Okres zdarzenia HEARTBEAT. Host uznaje wezel za utracony po braku HEARTBEAT przez 3 okresy.'),
    ParamDesc(0x0B, 'TIME_SYNC_PERIOD', 'u16', 'rw', 'nv', 0, 3600, 10, 's', False, 'Okres nadawania SET_TIME_SYNC przez wzorzec czasu. 0 = wezel nie nadaje czasu (wzorzec wylaczony).'),
    ParamDesc(0x0C, 'TIME_HOLDOVER', 'u16', 'rw', 'nv', 0, 65535, 600, 's', False, 'Jak dlugo po utracie wzorca (fix, PPS) zrodlo nadaje dalej z flaga HOLDOVER. Po tym czasie milknie. 0 = milknie od razu.'),
    ParamDesc(0x10, 'EN_GNSS', 'u8', 'rw', 'nv', 0, 1, 1, '-', False, 'Aktywacja odbiornika GNSS. 0 = modul w trybie Standby (PMTK161).'),
    ParamDesc(0x11, 'EN_IMU', 'u8', 'rw', 'nv', 0, 1, 1, '-', False, 'Aktywacja czujnika FXOS8700CQ i wielkosci orientacji.'),
    ParamDesc(0x12, 'GNSS_CONSTELLATIONS', 'u8', 'rw', 'nv', 1, 15, 7, 'mask', False, 'Konstelacje (PMTK353): bit0 GPS, bit1 GLONASS, bit2 Galileo, bit3 QZSS.'),
    ParamDesc(0x13, 'GNSS_SBAS', 'u8', 'rw', 'nv', 0, 1, 1, '0/1', False, 'Korekcje SBAS (EGNOS/WAAS/MSAS/GAGAN).'),
    ParamDesc(0x14, 'GNSS_POWER_POLICY', 'u8', 'rw', 'nv', 0, 3, 0, '-', False, 'Tryb pracy L76-L: 0 = automatycznie wg stanu ruchu (ADR 0003), 1 = stale wlaczony, 2 = okresowy (PMTK225), 3 = AlwaysLocate.'),
    ParamDesc(0x15, 'GNSS_MIN_SATS', 'u8', 'rw', 'nv', 3, 12, 4, '-', False, 'Minimalna liczba satelitow dla rozwiazania waznego (VALID).'),
    ParamDesc(0x16, 'GNSS_MAX_HDOP', 'u16', 'rw', 'nv', 100, 5000, 500, '0.01', False, 'Maksymalne HDOP dla rozwiazania waznego (VALID); 500 = 5.0.'),
    ParamDesc(0x17, 'STATIC_AVERAGING', 'u8', 'rw', 'nv', 0, 1, 1, '0/1', False, 'Usrednianie pozycji w stanie STATIC.'),
    ParamDesc(0x18, 'STATIC_AVG_WINDOW_S', 'u16', 'rw', 'nv', 10, 3600, 300, 's', False, 'Okno usredniania pozycji w spoczynku.'),
    ParamDesc(0x19, 'PRECISE_SYNC_PERIOD', 'u16', 'rw', 'nv', 0, 60000, 1000, 'ms', False, 'Okres pary TIME_SYNC / TIME_FOLLOW_UP (poziom 2, provisional). 0 = poziom 2 wylaczony. Nadawane wylacznie przy jakosci PPS.'),
    ParamDesc(0x20, 'IMU_SHOCK_MG', 'u16', 'rw', 'nv', 500, 7900, 2500, 'mg', False, 'Prog zdarzenia SHOCK (zakres FXOS +-8 g).'),
    ParamDesc(0x21, 'IMU_TILT_LIMIT', 'u16', 'rw', 'nv', 100, 9000, 3000, '0.01 deg', False, 'Prog zdarzenia TILT_ALARM (pochylenie lub przechylenie).'),
    ParamDesc(0x22, 'IMU_STATIC_MG', 'u16', 'rw', 'nv', 5, 500, 30, 'mg', False, 'Prog odchylenia przyspieszenia, ponizej ktorego wezel uznaje spoczynek.'),
    ParamDesc(0x23, 'IMU_STATIC_S', 'u16', 'rw', 'nv', 5, 600, 30, 's', False, 'Czas spelnienia warunku spoczynku przed przejsciem w STATIC.'),
    ParamDesc(0x24, 'MOVING_SPEED', 'u16', 'rw', 'nv', 50, 3000, 300, 'cm/s', False, 'Predkosc GNSS, powyzej ktorej stan ruchu to MOVING (jazda).'),
    ParamDesc(0x25, 'MAG_DECLINATION', 'i16', 'rw', 'nv', -3000, 3000, 0, '0.01 deg', False, 'Deklinacja magnetyczna miejsca instalacji (dodatnia = wschodnia). Kurs geograficzny = HEADING_MAG + deklinacja; wielkosc HEADING_MAG pozostaje kursem magnetycznym.'),
    ParamDesc(0x30, 'CAL_MAG_OFF_X', 'f32', 'rw', 'nv', -500.0, 500.0, 0.0, 'uT', True, 'Offset hard-iron magnetometru, os X wezla.'),
    ParamDesc(0x31, 'CAL_MAG_OFF_Y', 'f32', 'rw', 'nv', -500.0, 500.0, 0.0, 'uT', True, 'Offset hard-iron magnetometru, os Y wezla.'),
    ParamDesc(0x32, 'CAL_MAG_OFF_Z', 'f32', 'rw', 'nv', -500.0, 500.0, 0.0, 'uT', True, 'Offset hard-iron magnetometru, os Z wezla.'),
    ParamDesc(0x33, 'CAL_MOUNT_ORIENT', 'u8', 'rw', 'nv', 0, 23, 0, '-', True, 'Orientacja montazu: jeden z 24 obrotow o wielokrotnosc 90 st przeksztalcajacy osie czujnika na osie wezla (definicja w ADR 0004).'),
    ParamDesc(0x34, 'CAL_VIN_C0', 'f32', 'rw', 'nv', -1.0, 1.0, 0.0, 'V', True, 'Napiecie zasilania: offset c0 (V = c1 * V_nom + c0).'),
    ParamDesc(0x35, 'CAL_VIN_C1', 'f32', 'rw', 'nv', 0.5, 1.5, 1.0, '-', True, 'Napiecie zasilania: wzmocnienie c1 wzgledem nominalnego dzielnika 1M/100k.'),
    ParamDesc(0x36, 'CAL_ISENS_C0', 'f32', 'rw', 'nv', -0.001, 0.001, 0.0, 'A', True, 'Prad zasilania: offset c0 (I = c1 * I_nom + c0).'),
    ParamDesc(0x37, 'CAL_ISENS_C1', 'f32', 'rw', 'nv', 0.5, 1.5, 1.0, '-', True, 'Prad zasilania: wzmocnienie c1 wzgledem nominalnego INA186A4 (G = 200, shunt 0.5 Ohm).'),
]
PARAM_BY_ID = {p.id: p for p in PARAMS}
PARAM_BY_NAME = {p.name: p for p in PARAMS}

PY_STRUCT = {'u8': 'B', 'u16': 'H', 'u32': 'I', 'i16': 'h', 'i32': 'i', 'f32': 'f'}

def pack_param(p: ParamDesc, value) -> bytes:
    fmt = '<' + PY_STRUCT[p.type]
    return struct.pack(fmt, float(value) if p.type == 'f32' else int(value))

def unpack_param(p: ParamDesc, data: bytes):
    fmt = '<' + PY_STRUCT[p.type]
    return struct.unpack(fmt, data[:struct.calcsize(fmt)])[0]

SENSOR_IDS = [0x20, 0x21, 0x23, 0x11, 0x0C]
SENSOR_NAMES = {0x20: 'GNSS', 0x21: 'FXOS8700CQ', 0x23: 'ORIENTATION', 0x11: 'SUPPLY_MON', 0x0C: 'MCU'}

CHANNEL_IDS = [0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x66, 0x67, 0x68, 0x69, 0x12, 0x13, 0x0A, 0x0B]

# ---- Zgodnosc wsteczna: jak dekodowac wezly sprzed ujednolicenia ----
# Profil rozpoznaje sie po dlugosci odpowiedzi GET_INFO:
#   4 B -> profil 0,  21 B -> profil 1,  28 B -> profil 2
LEGACY_OPCODES = {
    (0, 'MPCC', 0x0D): ('GET_OUTPUT', 0x80),
    (0, 'MPCC', 0x0E): ('SET_OUTPUT', 0x81),
    (0, 'MPCC', 0x0F): ('SET_LED', 0x82),
    (0, 'MPCC', 0x10): ('READ_ADC', 0x83),
    (0, 'MPCC', 0x11): ('ENTER_BOOTLOADER', 0x0D),
    (1, 'MPSWP', 0x0E): ('CALIB_ANTENNA', 0x80),
}
LEGACY_EVENTS = {
    (1, 'MPSWP', 0x02): ('LIGHTNING', 0x80),
    (1, 'MPSWP', 0x03): ('STS31_ALERT', 0x03),
    (1, 'MPSWP', 0x0A): ('CALIB_DONE', 0x81),
    (1, 'MPSWP', 0x0B): ('SENSOR_RANGE', 0x0C),
    (0, 'MPCC', 0x03): ('STS31_ALERT', 0x03),
    (0, 'MPCC', 0x0A): ('BUTTON', 0x80),
    (0, 'MPCC', 0x0B): ('OUTPUT_CHANGED', 0x81),
}
RETIRED_OPCODES = [0x0E, 0x0F, 0x10, 0x11]
RETIRED_EVENTS = [0x02, 0x0A, 0x0B]

