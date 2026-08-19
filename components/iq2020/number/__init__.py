import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_TIMEOUT,
    DEVICE_CLASS_DISTANCE,
    DEVICE_CLASS_SIGNAL_STRENGTH,
    DEVICE_CLASS_POWER,
    UNIT_SECOND,
    UNIT_PERCENT,
    ENTITY_CATEGORY_NONE,
    ICON_MOTION_SENSOR,
    ICON_TIMELAPSE,
    ICON_GAS_CYLINDER,
)
from .. import CONF_IQ2020_ID, IQ2020Component, iq2020_ns

SWGLevelNumber = iq2020_ns.class_("SWGLevelNumber", number.Number)


CONF_SWG_LEVEL = "swg_level"


CONFIG_SCHEMA = cv.Schema(
     {
         cv.GenerateID(CONF_IQ2020_ID): cv.use_id(IQ2020Component),
         cv.Optional(CONF_SWG_LEVEL): number.number_schema(
             SWGLevelNumber,
             device_class=DEVICE_CLASS_POWER,
             entity_category=ENTITY_CATEGORY_NONE,
             icon=ICON_GAS_CYLINDER,
         )
     }
 )



async def to_code(config):
    iq2020_component = await cg.get_variable(config[CONF_IQ2020_ID])
    if swg_percnt_config := config.get(CONF_SWG_LEVEL):
        n = await number.new_number(
            swg_percnt_config, min_value=0, max_value=10, step=1
        )
        await cg.register_parented(n, config[CONF_IQ2020_ID])
        cg.add(iq2020_component.set_swg_level_number(n))
