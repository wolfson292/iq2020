import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import (
    DEVICE_CLASS_SWITCH,
    ICON_RADIOACTIVE,
    ICON_FAN
)
from .. import CONF_IQ2020_ID, IQ2020Component, iq2020_ns

CleanModeSwitch = iq2020_ns.class_("CleanModeSwitch", switch.Switch)
Jets1Switch = iq2020_ns.class_("Jets1Switch", switch.Switch)

CONF_CLEAN_MODE = "clean_mode"
CONF_JETS1 = "jets1"

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
}


async def to_code(config):
    iq2020_component = await cg.get_variable(config[CONF_IQ2020_ID])
    if cfg := config.get(CONF_CLEAN_MODE):
        s = await switch.new_switch(cfg)
        await cg.register_parented(s, config[CONF_IQ2020_ID])
        cg.add(iq2020_component.set_clean_mode_switch(s))
    if cfg := config.get(CONF_JETS1):
        s = await switch.new_switch(cfg)
        await cg.register_parented(s, config[CONF_IQ2020_ID])
        cg.add(iq2020_component.set_jets1_switch(s))
