import esphome.codegen as cg
from esphome.components import climate
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_SWITCH,
    ICON_BLUETOOTH,
    ENTITY_CATEGORY_CONFIG,
    ICON_PULSE,
)
from .. import CONF_IQ2020_ID, IQ2020Component, iq2020_ns

IQ2020Climate = iq2020_ns.class_("IQ2020Climate", climate.Climate, cg.Component)

CONFIG_SCHEMA = climate.climate_schema(IQ2020Climate).extend(
    {
        cv.GenerateID(CONF_IQ2020_ID): cv.use_id(IQ2020Component),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    iq2020_component = await cg.get_variable(config[CONF_IQ2020_ID])
    server = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(server, config)
    await climate.register_climate(server, config)
    await cg.register_parented(server, config[CONF_IQ2020_ID])
    cg.add(iq2020_component.set_device(server))