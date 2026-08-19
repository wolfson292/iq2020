from esphome import automation
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import datetime
from esphome.const import (
    ENTITY_CATEGORY_NONE
)
from .. import CONF_IQ2020_ID, IQ2020Component, iq2020_ns

DEPENDENCIES = ["iq2020"]

CONF_RTC = "rtc"

IQ2020DateTime = iq2020_ns.class_("IQ2020DateTime", datetime.DateTimeEntity)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_IQ2020_ID): cv.use_id(IQ2020Component),
        cv.Optional(CONF_RTC): datetime.datetime_schema(IQ2020DateTime),
    }
)
async def to_code(config):
    iq2020_component = await cg.get_variable(config[CONF_IQ2020_ID])
    if datetime_config := config.get(CONF_RTC):
        sens = await datetime.new_datetime(datetime_config)
        await cg.register_parented(sens, config[CONF_IQ2020_ID])

    