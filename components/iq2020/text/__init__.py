import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text
from esphome.const import (
    CONF_ENTITY_CATEGORY,
    CONF_ICON,
    CONF_MODE,
    CONF_SOURCE_ID,
    CONF_OUTPUT_ID,
)
from esphome.core.entity_helpers import inherit_property_from

from .. import CONF_IQ2020_ID, IQ2020Component, iq2020_ns

IQ2020Text = iq2020_ns.class_("IQ2020Text", text.Text, cg.Component)

CONF_TYPE = "type"


CONFIG_SCHEMA = text.text_schema(IQ2020Text).extend(
    {
        cv.GenerateID(CONF_IQ2020_ID): cv.use_id(IQ2020Component),
        cv.Required(CONF_TYPE): cv.string,
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    iq2020_component = await cg.get_variable(config[CONF_IQ2020_ID])
    var = await text.new_text(config)
    await cg.register_component(var, config)
    cg.add(var.set_type(config[CONF_TYPE]))
    await cg.register_parented(var, config[CONF_IQ2020_ID])
