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

CleanModeSwitch = iq2020_ns.class_("CleanModeSwitch", switch.Switch, cg.Component)
Jets1Switch = iq2020_ns.class_("Jets1Switch", switch.Switch, cg.Component)
Jets2Switch = iq2020_ns.class_("Jets2Switch", switch.Switch, cg.Component)
Jets3Switch = iq2020_ns.class_("Jets3Switch", switch.Switch, cg.Component)
BlowerSwitch = iq2020_ns.class_("BlowerSwitch", switch.Switch, cg.Component)
SummerTimerSwitch = iq2020_ns.class_("SummerTimerSwitch", switch.Switch, cg.Component)
SpaLockSwitch = iq2020_ns.class_("SpaLockSwitch", switch.Switch, cg.Component)
TempLockSwitch = iq2020_ns.class_("TempLockSwitch", switch.Switch, cg.Component)
SWGBoostSwitch = iq2020_ns.class_("SWGBoostSwitch", switch.Switch, cg.Component)
CaptureSwitch = iq2020_ns.class_("CaptureSwitch", switch.Switch, cg.Component)

CONF_CLEAN_MODE = "clean_mode"
CONF_JETS1 = "jets1"
CONF_JETS2 = "jets2"
CONF_JETS3 = "jets3"
CONF_BLOWER = "blower"
CONF_SUMMER_TIMER = "summer_timer"
CONF_SPA_LOCK = "spa_lock"
CONF_TEMP_LOCK = "temp_lock"
CONF_SWG_BOOST = "swg_boost"
CONF_CAPTURE = "capture"

def _switch(cls, **kwargs):
    # COMPONENT_SCHEMA is required now that these are Components; without it
    # register_component has no validated config to work from.
    return switch.switch_schema(cls, **kwargs).extend(cv.COMPONENT_SCHEMA)


CONFIG_SCHEMA = {
    cv.GenerateID(CONF_IQ2020_ID): cv.use_id(IQ2020Component),
    cv.Optional(CONF_CLEAN_MODE): _switch(
        CleanModeSwitch,
        device_class=DEVICE_CLASS_SWITCH,
        icon=ICON_RADIOACTIVE,
        default_restore_mode="RESTORE_DEFAULT_OFF",
    ),
    cv.Optional(CONF_JETS1): _switch(
        Jets1Switch,
        device_class=DEVICE_CLASS_SWITCH,
        icon=ICON_FAN,
        default_restore_mode="RESTORE_DEFAULT_OFF",
    ),
    # Jets 2 and 3 are present on larger spas. On a spa that does not have
    # them the controller answers 0 whatever is sent, so the switch simply
    # falls back to off rather than misreporting.
    cv.Optional(CONF_JETS2): _switch(
        Jets2Switch,
        device_class=DEVICE_CLASS_SWITCH,
        icon=ICON_FAN,
        default_restore_mode="RESTORE_DEFAULT_OFF",
    ),
    cv.Optional(CONF_JETS3): _switch(
        Jets3Switch,
        device_class=DEVICE_CLASS_SWITCH,
        icon=ICON_FAN,
        default_restore_mode="RESTORE_DEFAULT_OFF",
    ),
    cv.Optional(CONF_BLOWER): _switch(
        BlowerSwitch,
        device_class=DEVICE_CLASS_SWITCH,
        icon=ICON_FAN,
        default_restore_mode="RESTORE_DEFAULT_OFF",
    ),
    cv.Optional(CONF_SUMMER_TIMER): _switch(
        SummerTimerSwitch,
        device_class=DEVICE_CLASS_SWITCH,
        icon="mdi:weather-sunny",
        entity_category=ENTITY_CATEGORY_CONFIG,
        default_restore_mode="RESTORE_DEFAULT_OFF",
    ),
    cv.Optional(CONF_SPA_LOCK): _switch(
        SpaLockSwitch,
        device_class=DEVICE_CLASS_SWITCH,
        icon="mdi:lock",
        entity_category=ENTITY_CATEGORY_CONFIG,
        default_restore_mode="RESTORE_DEFAULT_OFF",
    ),
    cv.Optional(CONF_TEMP_LOCK): _switch(
        TempLockSwitch,
        device_class=DEVICE_CLASS_SWITCH,
        icon="mdi:thermometer-lock",
        entity_category=ENTITY_CATEGORY_CONFIG,
        default_restore_mode="RESTORE_DEFAULT_OFF",
    ),
    # 24-hour boost cycle on the salt module. Unlike the 0B switches this one
    # goes direct to the module, so it only works once the module has been
    # seen on the bus.
    cv.Optional(CONF_SWG_BOOST): _switch(
        SWGBoostSwitch,
        device_class=DEVICE_CLASS_SWITCH,
        icon="mdi:rocket-launch-outline",
        default_restore_mode="RESTORE_DEFAULT_OFF",
    ),
    # Raw frame capture. ALWAYS_ON while the protocol work is ongoing: a reflash
    # would otherwise silently drop capture, which is exactly when a comparison
    # of before and after is wanted. Revert to ALWAYS_OFF once that settles, so
    # a reboot cannot leave a device logging every frame indefinitely.
    cv.Optional(CONF_CAPTURE): _switch(
        CaptureSwitch,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:record-rec",
        default_restore_mode="ALWAYS_ON",
    ),
}


async def to_code(config):
    iq2020_component = await cg.get_variable(config[CONF_IQ2020_ID])
    if cfg := config.get(CONF_CLEAN_MODE):
        s = await switch.new_switch(cfg)
        # register_component is what makes setup() run, and setup() is what
        # applies default_restore_mode. Without it the mode is silently ignored.
        await cg.register_component(s, cfg)
        await cg.register_parented(s, config[CONF_IQ2020_ID])
        cg.add(iq2020_component.set_clean_mode_switch(s))
    for key in (CONF_JETS1, CONF_JETS2, CONF_JETS3, CONF_BLOWER,
                CONF_SUMMER_TIMER, CONF_SPA_LOCK, CONF_TEMP_LOCK, CONF_SWG_BOOST, CONF_CAPTURE):
        if cfg := config.get(key):
            s = await switch.new_switch(cfg)
            await cg.register_component(s, cfg)
            await cg.register_parented(s, config[CONF_IQ2020_ID])
            cg.add(getattr(iq2020_component, f"set_{key}_switch")(s))
