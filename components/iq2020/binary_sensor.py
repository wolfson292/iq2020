import esphome.codegen as cg
from esphome.components import binary_sensor, sensor
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
    ENTITY_CATEGORY_NONE
)
from . import CONF_IQ2020_ID, IQ2020Component

DEPENDENCIES = ["iq2020"]


CONF_SUMMER_TIMER = "summer_timer"
CONF_SPA_LOCK = "spa_lock"
CONF_TEMP_LOCK = "temp_lock"
CONF_CLEAN_LOCK = "clean_lock"
CONF_PUMP = "pump"
CONF_SWG_GENERATING = "swg_generating"
CONF_SWG_BOOST = "swg_boost"
CONF_SWG_CARTRIDGE_DUE = "swg_cartridge_due"
CONF_ECON_MODE = "econ_mode"
CONF_CIRCULATION = "circulation"
CONF_SWG_CARTRIDGE_PRESENT = "swg_cartridge_present"
CONF_SWG_LEVEL_LOCKED = "swg_level_locked"
CONF_SWG_TESTING = "swg_testing"
CONF_HEATER = "heater"
CONF_FLOW_SWITCH = "flow_switch"
CONF_WATER_TEMP_FAULT = "water_temp_fault"
CONF_SWG_PRESENT = "swg_present"
CONF_COOLZONE_PRESENT = "coolzone_present"

TYPES = (
    CONF_SUMMER_TIMER,
    CONF_SPA_LOCK,
    CONF_TEMP_LOCK,
    CONF_CLEAN_LOCK,
    CONF_PUMP,
    CONF_SWG_GENERATING,
    CONF_SWG_BOOST,
    CONF_SWG_CARTRIDGE_DUE,
    CONF_ECON_MODE,
    CONF_CIRCULATION,
    CONF_SWG_CARTRIDGE_PRESENT,
    CONF_SWG_LEVEL_LOCKED,
    CONF_SWG_TESTING,
    CONF_HEATER,
    CONF_FLOW_SWITCH,
    CONF_WATER_TEMP_FAULT,
    CONF_SWG_PRESENT,
    CONF_COOLZONE_PRESENT,
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_IQ2020_ID): cv.use_id(IQ2020Component),
        
        cv.Optional(CONF_SUMMER_TIMER): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_NONE,
        ),
        cv.Optional(CONF_SPA_LOCK): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_NONE,
        ),
        cv.Optional(CONF_TEMP_LOCK): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_NONE,
        ),
        cv.Optional(CONF_CLEAN_LOCK): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_NONE,
        ),
        cv.Optional(CONF_PUMP): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_NONE,
        ),
        # SWG flags byte, payload[5] of the module's 1E/01 frame.
        # bit 0 = actively generating chlorine.
        cv.Optional(CONF_SWG_GENERATING): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_NONE,
        ),
        # bit 2 = the 24-hour boost cycle is running.
        cv.Optional(CONF_SWG_BOOST): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_NONE,
        ),
        # Cartridge age has reached the controller's 120-day replace prompt.
        cv.Optional(CONF_SWG_CARTRIDGE_DUE): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_NONE,
        ),
        # From the 02/41 filter-config reply. These two bits are not carried in
        # the 02/55 status block, so this is the only place they appear.
        cv.Optional(CONF_ECON_MODE): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_NONE,
        ),
        cv.Optional(CONF_CIRCULATION): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_NONE,
        ),
        # Cartridge seated in the cell. This is the flag the controller's own
        # replace wizard waits on.
        cv.Optional(CONF_SWG_CARTRIDGE_PRESENT): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_NONE,
        ),
        # Salt test reading above 9 - the controller locks output level
        # adjustment while this is set.
        cv.Optional(CONF_SWG_LEVEL_LOCKED): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_NONE,
        ),
        # bit 3 = the module is running its self-check. Set for the duration of
        # the water test and cleared when it finishes; while it is set the panel
        # replaces the salt status with "Testing".
        cv.Optional(CONF_SWG_TESTING): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_NONE,
        ),
        # 02/55 offset 6. The controller writes 5 when heating, not 1.
        cv.Optional(CONF_HEATER): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_NONE,
        ),
        # 02/55 offset 3 bit 3 - flow switch, debounced over 15 seconds.
        cv.Optional(CONF_FLOW_SWITCH): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_NONE,
        ),
        # 02/55 offset 3 bit 6 - the water temperature reading is unusable.
        cv.Optional(CONF_WATER_TEMP_FAULT): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        # 02/55 offset 8. Presence latches - set once the device has ever
        # answered on the bus, and not cleared afterwards.
        cv.Optional(CONF_SWG_PRESENT): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_COOLZONE_PRESENT): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        




    }
)

async def setup_conf(config, key, hub):
    if sensor_config := config.get(key):
        sens = await binary_sensor.new_binary_sensor(sensor_config)
        cg.add(getattr(hub, f"set_{key}_binary_sensor")(sens))


async def to_code(config):
    iq2020_component = await cg.get_variable(config[CONF_IQ2020_ID])
    for key in TYPES:
        await setup_conf(config, key, iq2020_component)
    