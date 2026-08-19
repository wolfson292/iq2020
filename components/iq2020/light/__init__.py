import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.components import light, output
from esphome.const import (
    CONF_ID,
    CONF_TIMEOUT,
    DEVICE_CLASS_DISTANCE,
    DEVICE_CLASS_SIGNAL_STRENGTH,
    DEVICE_CLASS_ILLUMINANCE,
    UNIT_SECOND,
    UNIT_PERCENT,
    ENTITY_CATEGORY_CONFIG,
    ICON_MOTION_SENSOR,
    ICON_TIMELAPSE,
    ICON_LIGHTBULB,
    CONF_OUTPUT_ID, CONF_OUTPUT
)
from .. import CONF_IQ2020_ID, IQ2020Component, iq2020_ns

SpaLight = iq2020_ns.class_("SpaLight", light.LightOutput, cg.Component)


CONF_SPA = "spa"
CONFIG_LIGHT_NUM = "light_num"


CONFIG_SCHEMA = light.BRIGHTNESS_ONLY_LIGHT_SCHEMA.extend(
    {
        cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(SpaLight),
        cv.GenerateID(CONF_IQ2020_ID): cv.use_id(IQ2020Component),
        cv.Required(CONFIG_LIGHT_NUM): cv.uint8_t,
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    iq2020_component = await cg.get_variable(config[CONF_IQ2020_ID])
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_component(var, config)
    await light.register_light(var, config)
    cg.add(var.set_light_num(config[CONFIG_LIGHT_NUM]))
    await cg.register_parented(var, config[CONF_IQ2020_ID])
    cg.add(iq2020_component.set_device(var))

