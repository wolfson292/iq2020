import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    DEVICE_CLASS_DISTANCE,
    UNIT_CENTIMETER,
    UNIT_PERCENT,
    UNIT_VOLT,
    CONF_LIGHT,
    DEVICE_CLASS_ILLUMINANCE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    DEVICE_CLASS_VOLTAGE,
    DEVICE_CLASS_CURRENT,
    UNIT_AMPERE,
    ICON_WATER,
    ICON_FLASH,
    ICON_MOTION_SENSOR,
    ICON_LIGHTBULB,
    DEVICE_CLASS_VOLATILE_ORGANIC_COMPOUNDS_PARTS,
    UNIT_PARTS_PER_MILLION,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_POWER,
    UNIT_CELSIUS,
    UNIT_WATT,
    DEVICE_CLASS_DURATION,
    UNIT_SECOND,
    ICON_TIMER,
    DEVICE_CLASS_SWITCH,
    ICON_THERMOMETER,
    ICON_POWER,
    ICON_HEATING_COIL,
    DEVICE_CLASS_SPEED,
    ICON_BLUR,
    ICON_MAGNET,
    ICON_BUG
)
from . import CONF_IQ2020_ID, IQ2020Component

DEPENDENCIES = ["iq2020"]


CONF_JETS1_TIMEOUT = "jets1_timeout"
CONF_JETS2_TIMEOUT = "jets2_timeout"
CONF_JETS3_TIMEOUT = "jets3_timeout"
CONF_BLOWER_TIMEOUT = "blower_timeout"
CONF_LIGHTS_TIMEOUT = "lights_timeout"
CONF_JETS1_SPEED = "jets1_speed"
CONF_JETS2_SPEED = "jets2_speed"
CONF_JETS3_SPEED = "jets3_speed"
CONF_BLOWER_SPEED = "blower_speed"
CONF_HIGH_LIMIT_TEMP = "high_limit_temp"
CONF_HEATER_SEC = "heater_seconds"
CONF_JET1_SEC = "jet1_seconds"
CONF_LIFETIME_SEC = "lifetime_seconds"
CONF_LOST_LINE = "lost_lines"
CONF_BUS_RESYNCS = "bus_resyncs"
CONF_SENDS_DEFERRED = "sends_deferred"
CONF_JET2_SEC = "jet2_seconds"
CONF_JET3_SEC = "jet3_seconds"
CONF_BLOWER_SEC = "blower_seconds"
CONF_LIGHT_SEC = "lights_seconds"
CONF_PUMP_SEC = "pump_seconds"
CONF_JET1_LOW_SEC = "jet1_low_seconds"
CONF_JET2_LOW_SEC = "jet2_low_seconds"
CONF_TEMP_SET = "temp_set"
CONF_WATER_TEMP = "water_temp"
CONF_BASELINE_VOLT = "baseline_voltage"
CONF_HEATER_VOLT = "heater_voltage"
CONF_AUX_VOLT = "aux_voltage"
CONF_JETS_BLOWER_VOLT = "jets_blower_voltage"
CONF_BASELINE_CURRENT = "baseline_current"
CONF_HEATER_CURRENT = "heater_current"
CONF_AUX_CURRENT = "aux_current"
CONF_JETS_BLOWER_CURRENT = "jets_blower_current"
CONF_BASELINE_POWER = "baseline_power"
CONF_AUX_POWER = "aux_power"
CONF_JETS_BLOWER_POWER = "jets_blower_power"
CONF_HEATER_POWER = "heater_power"
CONF_FILTER1_TIME = "filter1_time"
CONF_FILTER2_TIME = "filter2_time"
CONF_PCB_TEMP = "pcb_temp"
CONF_PANEL_TYPE = "panel_type"
CONF_PERIPH_CURRENT = "periph_current"
CONF_RTC_SEC = "rtc_seconds"
CONF_RTC_MINUTE = "rtc_minutes"
CONF_RTC_HOUR = "rtc_hours"
CONF_RTC_DAY = "rtc_days"
CONF_RTC_MONTH = "rtc_months"
CONF_RTC_YEAR = "rtc_years"
CONF_DAILY_CLEAN = "daily_clean_cycle"

CONF_SWG_AGE = "swg_age"
CONF_SWG_BOOST_MODE = "swg_boost_mode"
CONF_SWG_ERROR = "swg_error"
CONF_SWG_SALINITY = "swg_salinity"
CONF_SWG_SALINITY_INDEX = "swg_salinity_index"
CONF_SWG_CELL_RUNTIME = "swg_cell_runtime"
CONF_SWG_SPA_SIZE = "swg_spa_size"
CONF_SWG_OUTPUT_LEVEL = "swg_output_level"
CONF_SWG_SALT_TEST = "swg_salt_test"
CONF_SWG_CELL_STATE = "swg_cell_state"


TYPES = (
    CONF_JETS1_TIMEOUT,
    CONF_JETS2_TIMEOUT,
    CONF_JETS3_TIMEOUT,
    CONF_BLOWER_TIMEOUT,
    CONF_LIGHTS_TIMEOUT,
    CONF_JETS1_SPEED,
    CONF_JETS2_SPEED,
    CONF_JETS3_SPEED,
    CONF_BLOWER_SPEED,
    CONF_HIGH_LIMIT_TEMP,
    CONF_HEATER_SEC,
    CONF_JET1_SEC,
    CONF_LIFETIME_SEC,
    CONF_LOST_LINE,
    CONF_BUS_RESYNCS,
    CONF_SENDS_DEFERRED,
    CONF_JET2_SEC,
    CONF_JET3_SEC,
    CONF_BLOWER_SEC,
    CONF_LIGHT_SEC,
    CONF_PUMP_SEC,
    CONF_JET1_LOW_SEC,
    CONF_JET2_LOW_SEC,
    CONF_TEMP_SET,
    CONF_WATER_TEMP,
    CONF_BASELINE_VOLT,
    CONF_HEATER_VOLT,
    CONF_AUX_VOLT,
    CONF_JETS_BLOWER_VOLT,
    CONF_BASELINE_CURRENT,
    CONF_HEATER_CURRENT,
    CONF_AUX_CURRENT,
    CONF_JETS_BLOWER_CURRENT,
    CONF_BASELINE_POWER,
    CONF_AUX_POWER,
    CONF_JETS_BLOWER_POWER,
    CONF_HEATER_POWER,
    CONF_FILTER1_TIME,
    CONF_FILTER2_TIME,
    CONF_PCB_TEMP,
    CONF_PANEL_TYPE,
    CONF_PERIPH_CURRENT,
    CONF_RTC_SEC,
    CONF_RTC_MINUTE,
    CONF_RTC_HOUR,
    CONF_RTC_DAY,
    CONF_RTC_MONTH,
    CONF_RTC_YEAR,
    CONF_DAILY_CLEAN,
    CONF_SWG_AGE,
    CONF_SWG_BOOST_MODE,
    CONF_SWG_ERROR,
    CONF_SWG_SALINITY,
    CONF_SWG_SALINITY_INDEX,
    CONF_SWG_CELL_RUNTIME,
    CONF_SWG_SPA_SIZE,
    CONF_SWG_OUTPUT_LEVEL,
    CONF_SWG_SALT_TEST,
    CONF_SWG_CELL_STATE,
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_IQ2020_ID): cv.use_id(IQ2020Component),
        
        cv.Optional(CONF_TEMP_SET): sensor.sensor_schema(
            device_class=DEVICE_CLASS_TEMPERATURE,
            unit_of_measurement="°F",
            icon=ICON_THERMOMETER,
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_HIGH_LIMIT_TEMP): sensor.sensor_schema(
            device_class=DEVICE_CLASS_TEMPERATURE,
            unit_of_measurement="°F",
            icon=ICON_THERMOMETER,
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_WATER_TEMP): sensor.sensor_schema(
            device_class=DEVICE_CLASS_TEMPERATURE,
            unit_of_measurement="°F",
            icon=ICON_THERMOMETER,
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_PCB_TEMP): sensor.sensor_schema(
            device_class=DEVICE_CLASS_TEMPERATURE,
            unit_of_measurement="°F",
            icon=ICON_THERMOMETER,
            accuracy_decimals=0,
        ),
        # Topside panel model code. Static for a given spa, and 0 until the
        # panel has reported in - the same byte the 01/00 version reply ends
        # with.
        cv.Optional(CONF_PANEL_TYPE): sensor.sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            accuracy_decimals=0,
        ),

        cv.Optional(CONF_BASELINE_POWER): sensor.sensor_schema(
            device_class=DEVICE_CLASS_POWER,
            unit_of_measurement=UNIT_WATT,
            icon=ICON_POWER,
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_AUX_POWER): sensor.sensor_schema(
            device_class=DEVICE_CLASS_POWER,
            unit_of_measurement=UNIT_WATT,
            icon=ICON_POWER,
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_JETS_BLOWER_POWER): sensor.sensor_schema(
            device_class=DEVICE_CLASS_POWER,
            unit_of_measurement=UNIT_WATT,
            icon=ICON_POWER,
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_HEATER_POWER): sensor.sensor_schema(
            device_class=DEVICE_CLASS_POWER,
            unit_of_measurement=UNIT_WATT,
            icon=ICON_POWER,
            accuracy_decimals=0,
        ),

        cv.Optional(CONF_BASELINE_VOLT): sensor.sensor_schema(
            device_class=DEVICE_CLASS_VOLTAGE,
            unit_of_measurement=UNIT_VOLT,
            icon=ICON_POWER,
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_AUX_VOLT): sensor.sensor_schema(
            device_class=DEVICE_CLASS_VOLTAGE,
            unit_of_measurement=UNIT_VOLT,
            icon=ICON_POWER,
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_JETS_BLOWER_VOLT): sensor.sensor_schema(
            device_class=DEVICE_CLASS_VOLTAGE,
            unit_of_measurement=UNIT_VOLT,
            icon=ICON_POWER,
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_HEATER_VOLT): sensor.sensor_schema(
            device_class=DEVICE_CLASS_VOLTAGE,
            unit_of_measurement=UNIT_VOLT,
            icon=ICON_POWER,
            accuracy_decimals=0,
        ),

        cv.Optional(CONF_BASELINE_CURRENT): sensor.sensor_schema(
            device_class=DEVICE_CLASS_CURRENT,
            unit_of_measurement=UNIT_AMPERE,
            icon=ICON_POWER,
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_AUX_CURRENT): sensor.sensor_schema(
            device_class=DEVICE_CLASS_CURRENT,
            unit_of_measurement=UNIT_AMPERE,
            icon=ICON_POWER,
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_JETS_BLOWER_CURRENT): sensor.sensor_schema(
            device_class=DEVICE_CLASS_CURRENT,
            unit_of_measurement=UNIT_AMPERE,
            icon=ICON_POWER,
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_HEATER_CURRENT): sensor.sensor_schema(
            device_class=DEVICE_CLASS_CURRENT,
            unit_of_measurement=UNIT_AMPERE,
            icon=ICON_POWER,
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_PERIPH_CURRENT): sensor.sensor_schema(
            device_class=DEVICE_CLASS_CURRENT,
            unit_of_measurement=UNIT_AMPERE,
            icon=ICON_POWER,
            accuracy_decimals=3,
        ),

        
        cv.Optional(CONF_HEATER_SEC): sensor.sensor_schema(
            device_class=DEVICE_CLASS_DURATION,
            unit_of_measurement=UNIT_SECOND,
            icon=ICON_TIMER,
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_JET1_SEC): sensor.sensor_schema(
            device_class=DEVICE_CLASS_DURATION,
            unit_of_measurement=UNIT_SECOND,
            icon=ICON_TIMER,
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_JET2_SEC): sensor.sensor_schema(
            device_class=DEVICE_CLASS_DURATION,
            unit_of_measurement=UNIT_SECOND,
            icon=ICON_TIMER,
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_JET3_SEC): sensor.sensor_schema(
            device_class=DEVICE_CLASS_DURATION,
            unit_of_measurement=UNIT_SECOND,
            icon=ICON_TIMER,
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_BLOWER_SEC): sensor.sensor_schema(
            device_class=DEVICE_CLASS_DURATION,
            unit_of_measurement=UNIT_SECOND,
            icon=ICON_TIMER,
        ),
        cv.Optional(CONF_JET1_LOW_SEC): sensor.sensor_schema(
            device_class=DEVICE_CLASS_DURATION,
            unit_of_measurement=UNIT_SECOND,
            icon=ICON_TIMER,
        ),
        cv.Optional(CONF_JET2_LOW_SEC): sensor.sensor_schema(
            device_class=DEVICE_CLASS_DURATION,
            unit_of_measurement=UNIT_SECOND,
            icon=ICON_TIMER,
        ),
        cv.Optional(CONF_LIGHT_SEC): sensor.sensor_schema(
            device_class=DEVICE_CLASS_DURATION,
            unit_of_measurement=UNIT_SECOND,
            icon=ICON_TIMER,
        ),
        cv.Optional(CONF_PUMP_SEC): sensor.sensor_schema(
            device_class=DEVICE_CLASS_DURATION,
            unit_of_measurement=UNIT_SECOND,
            icon=ICON_TIMER,
        ),
        cv.Optional(CONF_LIFETIME_SEC): sensor.sensor_schema(
            device_class=DEVICE_CLASS_DURATION,
            unit_of_measurement=UNIT_SECOND,
            icon=ICON_TIMER,
        ),


        cv.Optional(CONF_JETS1_TIMEOUT): sensor.sensor_schema(
            device_class=DEVICE_CLASS_DURATION,
            unit_of_measurement=UNIT_SECOND,
            icon=ICON_TIMER,
        ),
        cv.Optional(CONF_JETS2_TIMEOUT): sensor.sensor_schema(
            device_class=DEVICE_CLASS_DURATION,
            unit_of_measurement=UNIT_SECOND,
            icon=ICON_TIMER,
        ),
        cv.Optional(CONF_JETS3_TIMEOUT): sensor.sensor_schema(
            device_class=DEVICE_CLASS_DURATION,
            unit_of_measurement=UNIT_SECOND,
            icon=ICON_TIMER,
        ),
        cv.Optional(CONF_BLOWER_TIMEOUT): sensor.sensor_schema(
            device_class=DEVICE_CLASS_DURATION,
            unit_of_measurement=UNIT_SECOND,
            icon=ICON_TIMER,
        ),
        cv.Optional(CONF_LIGHTS_TIMEOUT): sensor.sensor_schema(
            device_class=DEVICE_CLASS_DURATION,
            unit_of_measurement=UNIT_SECOND,
            icon=ICON_TIMER,
        ),

        cv.Optional(CONF_JETS1_SPEED): sensor.sensor_schema(
            device_class=DEVICE_CLASS_SPEED,
            icon=ICON_BLUR,
        ),
        cv.Optional(CONF_JETS2_SPEED): sensor.sensor_schema(
            device_class=DEVICE_CLASS_SPEED,
            icon=ICON_BLUR,
        ),
        cv.Optional(CONF_JETS3_SPEED): sensor.sensor_schema(
            device_class=DEVICE_CLASS_SPEED,
            icon=ICON_BLUR,
        ),
        cv.Optional(CONF_BLOWER_SPEED): sensor.sensor_schema(
            device_class=DEVICE_CLASS_SPEED,
            icon=ICON_BLUR,
        ),

        cv.Optional(CONF_LOST_LINE): sensor.sensor_schema(
            icon=ICON_MAGNET,
        ),
        cv.Optional(CONF_DAILY_CLEAN): sensor.sensor_schema(
            icon=ICON_MAGNET,
        ),

        cv.Optional(CONF_FILTER1_TIME): sensor.sensor_schema(
            device_class=DEVICE_CLASS_DURATION,
            unit_of_measurement=UNIT_SECOND,
            icon=ICON_TIMER,
        ),
        cv.Optional(CONF_FILTER2_TIME): sensor.sensor_schema(
            device_class=DEVICE_CLASS_DURATION,
            unit_of_measurement=UNIT_SECOND,
            icon=ICON_TIMER,
        ),

        cv.Optional(CONF_RTC_SEC): sensor.sensor_schema(
            device_class=DEVICE_CLASS_DURATION,
            unit_of_measurement=UNIT_SECOND,
            icon=ICON_TIMER,
        ),
        cv.Optional(CONF_RTC_MINUTE): sensor.sensor_schema(
            device_class=DEVICE_CLASS_DURATION,
            unit_of_measurement="minute",
            icon=ICON_TIMER,
        ),
        cv.Optional(CONF_RTC_HOUR): sensor.sensor_schema(
            device_class=DEVICE_CLASS_DURATION,
            unit_of_measurement="hour",
            icon=ICON_TIMER,
        ),
        cv.Optional(CONF_RTC_DAY): sensor.sensor_schema(
            device_class=DEVICE_CLASS_DURATION,
            unit_of_measurement="day",
            icon=ICON_TIMER,
        ),
        cv.Optional(CONF_RTC_MONTH): sensor.sensor_schema(
            device_class=DEVICE_CLASS_DURATION,
            unit_of_measurement="month",
            icon=ICON_TIMER,
        ),
        cv.Optional(CONF_RTC_YEAR): sensor.sensor_schema(
            device_class=DEVICE_CLASS_DURATION,
            unit_of_measurement="year",
            icon=ICON_TIMER,
        ),


        # Cartridge age. The controller compares this against 120 for its
        # "Cartridge Reached 4 Months" prompt, so the unit is days, not seconds.
        cv.Optional(CONF_SWG_AGE): sensor.sensor_schema(
            device_class=DEVICE_CLASS_DURATION,
            unit_of_measurement="d",
            icon=ICON_MAGNET,
        ),
        # 1 while the 24-hour boost cycle is running, else 0. Prefer the
        # swg_boost binary_sensor; this stays for existing configs.
        cv.Optional(CONF_SWG_BOOST_MODE): sensor.sensor_schema(
            icon=ICON_MAGNET,
        ),
        # SWG error code. Codes 1, 2, 4 and 5 are the ones the controller
        # escalates to "Service Required".
        cv.Optional(CONF_SWG_ERROR): sensor.sensor_schema(
            icon=ICON_MAGNET,
        ),
        # Position along the panel's salt scale, 0-100%. The panel shows a bar,
        # not a concentration, and nothing on the bus carries a ppm figure - so
        # this reports where the marker sits, which is what you actually see.
        cv.Optional(CONF_SWG_SALINITY): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=0,
            icon=ICON_WATER,
        ),
        # Raw 6-bit salinity index, 0-63. >= 24 is the controller's "high" test.
        cv.Optional(CONF_SWG_SALINITY_INDEX): sensor.sensor_schema(
            icon=ICON_WATER,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        # 24-bit counter from the SWG module. Captures put its cadence at one
        # count per 60-145 minutes of wall clock, which is what a counter that
        # only advances while the cell is generating looks like at part duty.
        cv.Optional(CONF_SWG_CELL_RUNTIME): sensor.sensor_schema(
            unit_of_measurement="h",
            device_class=DEVICE_CLASS_DURATION,
            icon=ICON_TIMER,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        # Live state byte relayed by the controller without being interpreted.
        # Observed values 0, 2, 6, 8, stepping during a water test. Raw on
        # purpose - the meaning is not established.
        cv.Optional(CONF_SWG_CELL_STATE): sensor.sensor_schema(
            icon=ICON_MAGNET,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_SWG_SPA_SIZE): sensor.sensor_schema(
            icon=ICON_MAGNET,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        # Output level 0-10 as reported by the module itself.
        cv.Optional(CONF_SWG_OUTPUT_LEVEL): sensor.sensor_schema(
            icon=ICON_MAGNET,
        ),
        # Salt test reading. 15 raises the "Test Water" prompt, 20 the
        # "Level Set To 3" prompt, and anything above 9 locks level adjustment.
        # Frames abandoned and resynced: a mid-frame gap, an implausible
        # length byte, or a failed checksum. A rate rather than an event - what
        # matters is whether it climbs over hours.
        cv.Optional(CONF_BUS_RESYNCS): sensor.sensor_schema(
            icon=ICON_BUG,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        # Sends held back because the bus was busy. If this stays at zero the
        # idle gate is never engaging and is not doing anything.
        cv.Optional(CONF_SENDS_DEFERRED): sensor.sensor_schema(
            icon=ICON_BUG,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_SWG_SALT_TEST): sensor.sensor_schema(
            icon=ICON_WATER,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),






    }
)

async def setup_conf(config, key, hub):
    if sensor_config := config.get(key):
        sens = await sensor.new_sensor(sensor_config)
        cg.add(getattr(hub, f"set_{key}_sensor")(sens))


async def to_code(config):
    iq2020_component = await cg.get_variable(config[CONF_IQ2020_ID])
    for key in TYPES:
        await setup_conf(config, key, iq2020_component)
    