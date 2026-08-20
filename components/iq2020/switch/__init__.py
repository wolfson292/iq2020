import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import (
    DEVICE_CLASS_SWITCH,
    ENTITY_CATEGORY_CONFIG,
    ICON_RADIOACTIVE,
    ICON_FAN,
)
from .. import CONF_IQ2020_ID, IQ2020Component, iq2020_ns

CleanModeSwitch = iq2020_ns.class_("CleanModeSwitch", switch.Switch)
Jets1Switch = iq2020_ns.class_("Jets1Switch", switch.Switch)
Jets2Switch = iq2020_ns.class_("Jets2Switch", switch.Switch)
Jets3Switch = iq2020_ns.class_("Jets3Switch", switch.Switch)
BlowerSwitch = iq2020_ns.class_("BlowerSwitch", switch.Switch)
SummerTimerSwitch = iq2020_ns.class_("SummerTimerSwitch", switch.Switch)
SpaLockSwitch = iq2020_ns.class_("SpaLockSwitch", switch.Switch)
TempLockSwitch = iq2020_ns.class_("TempLockSwitch", switch.Switch)
SWGBoostSwitch = iq2020_ns.class_("SWGBoostSwitch", switch.Switch)

CONF_CLEAN_MODE = "clean_mode"
CONF_JETS1 = "jets1"
CONF_JETS2 = "jets2"
CONF_JETS3 = "jets3"
CONF_BLOWER = "blower"
CONF_SUMMER_TIMER = "summer_timer"
CONF_SPA_LOCK = "spa_lock"
CONF_TEMP_LOCK = "temp_lock"
CONF_SWG_BOOST = "swg_boost"

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_IQ2020_ID): cv.use_id(IQ2020Component),
    cv.Optional(CONF_CLEAN_MODE): switch.switch_schema(
        CleanModeSwitch,
        device_class=DEVICE_CLASS_SWITCH,
        icon=ICON_RADIOACTIVE,
        default_restore_mode="RESTORE_DEFAULT_OFF",
    ),
    cv.Optional(CONF_JETS1): switch.switch_schema(
        Jets1Switch,
        device_class=DEVICE_CLASS_SWITCH,
        icon=ICON_FAN,
        default_restore_mode="RESTORE_DEFAULT_OFF",
    ),
    # Jets 2 and 3 are present on larger spas. On a spa that does not have
    # them the controller answers 0 whatever is sent, so the switch simply
    # falls back to off rather than misreporting.
    cv.Optional(CONF_JETS2): switch.switch_schema(
        Jets2Switch,
        device_class=DEVICE_CLASS_SWITCH,
        icon=ICON_FAN,
        default_restore_mode="RESTORE_DEFAULT_OFF",
    ),
    cv.Optional(CONF_JETS3): switch.switch_schema(
        Jets3Switch,
        device_class=DEVICE_CLASS_SWITCH,
        icon=ICON_FAN,
        default_restore_mode="RESTORE_DEFAULT_OFF",
    ),
    cv.Optional(CONF_BLOWER): switch.switch_schema(
        BlowerSwitch,
        device_class=DEVICE_CLASS_SWITCH,
        icon=ICON_FAN,
        default_restore_mode="RESTORE_DEFAULT_OFF",
    ),
    cv.Optional(CONF_SUMMER_TIMER): switch.switch_schema(
        SummerTimerSwitch,
        device_class=DEVICE_CLASS_SWITCH,
        icon="mdi:weather-sunny",
        entity_category=ENTITY_CATEGORY_CONFIG,
        default_restore_mode="RESTORE_DEFAULT_OFF",
    ),
    cv.Optional(CONF_SPA_LOCK): switch.switch_schema(
        SpaLockSwitch,
        device_class=DEVICE_CLASS_SWITCH,
        icon="mdi:lock",
        entity_category=ENTITY_CATEGORY_CONFIG,
        default_restore_mode="RESTORE_DEFAULT_OFF",
    ),
    cv.Optional(CONF_TEMP_LOCK): switch.switch_schema(
        TempLockSwitch,
        device_class=DEVICE_CLASS_SWITCH,
        icon="mdi:thermometer-lock",
        entity_category=ENTITY_CATEGORY_CONFIG,
        default_restore_mode="RESTORE_DEFAULT_OFF",
    ),
    # 24-hour boost cycle on the salt module. Unlike the 0B switches this one
    # goes direct to the module, so it only works once the module has been
    # seen on the bus.
    cv.Optional(CONF_SWG_BOOST): switch.switch_schema(
        SWGBoostSwitch,
        device_class=DEVICE_CLASS_SWITCH,
        icon="mdi:rocket-launch-outline",
        default_restore_mode="RESTORE_DEFAULT_OFF",
    ),
}


async def to_code(config):
    iq2020_component = await cg.get_variable(config[CONF_IQ2020_ID])
    if cfg := config.get(CONF_CLEAN_MODE):
        s = await switch.new_switch(cfg)
        await cg.register_parented(s, config[CONF_IQ2020_ID])
        cg.add(iq2020_component.set_clean_mode_switch(s))
    for key in (CONF_JETS1, CONF_JETS2, CONF_JETS3, CONF_BLOWER,
                CONF_SUMMER_TIMER, CONF_SPA_LOCK, CONF_TEMP_LOCK, CONF_SWG_BOOST):
        if cfg := config.get(key):
            s = await switch.new_switch(cfg)
            await cg.register_parented(s, config[CONF_IQ2020_ID])
            cg.add(getattr(iq2020_component, f"set_{key}_switch")(s))
