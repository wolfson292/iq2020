import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.cpp_helpers import gpio_pin_expression
from esphome.const import (
    
    CONF_ID,
    CONF_VERSION,
    ICON_BUG,
    ENTITY_CATEGORY_DIAGNOSTIC,
)
from . import CONF_IQ2020_ID, IQ2020Component

DEPENDENCIES = ["iq2020"]

CONF_IQ2020_DEBUG = "iq2020_debug"
CONF_IQ2020_VERSION_CONTROLLER = "iq2020_version_controller"
CONF_IQ2020_VERSION_DISPLAY = "iq2020_version_display"
CONF_IQ2020_VERSION_OTHER_A = "iq2020_version_other_a"
CONF_IQ2020_VERSION_OTHER_B = "iq2020_version_other_b"

CONF_SPA_STATE = "spa_state"
CONF_LIGHT_STATE = "light_state"
CONF_RTC_STATUS = "rtc_status"
CONF_RTC = "rtc"
CONF_SWG_STATUS = "swg_status"
CONF_SWG_TYPE = "swg_type"

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_IQ2020_ID): cv.use_id(IQ2020Component),
    cv.Optional(CONF_IQ2020_DEBUG): text_sensor.text_sensor_schema(
        icon=ICON_BUG,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_IQ2020_VERSION_CONTROLLER): text_sensor.text_sensor_schema(
        icon=ICON_BUG,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_IQ2020_VERSION_DISPLAY): text_sensor.text_sensor_schema(
        icon=ICON_BUG,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_IQ2020_VERSION_OTHER_A): text_sensor.text_sensor_schema(
        icon=ICON_BUG,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_IQ2020_VERSION_OTHER_B): text_sensor.text_sensor_schema(
        icon=ICON_BUG,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),

    cv.Optional(CONF_SPA_STATE): text_sensor.text_sensor_schema(
        icon=ICON_BUG,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_LIGHT_STATE): text_sensor.text_sensor_schema(
        icon=ICON_BUG,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_RTC_STATUS): text_sensor.text_sensor_schema(
        icon=ICON_BUG,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_RTC): text_sensor.text_sensor_schema(
        icon=ICON_BUG,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    # Salt system status, in the controller's own wording.
    cv.Optional(CONF_SWG_STATUS): text_sensor.text_sensor_schema(),
    # Which SWG module answered: ACE (0x24) or FreshWater (0x29).
    cv.Optional(CONF_SWG_TYPE): text_sensor.text_sensor_schema(
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
}

async def to_code(config):
    iq2020_component = await cg.get_variable(config[CONF_IQ2020_ID])
    if iq2020_debug_config := config.get(CONF_IQ2020_DEBUG):
        sens = await text_sensor.new_text_sensor(iq2020_debug_config)
        cg.add(iq2020_component.set_iq2020_debug_text_sensor(sens))
    
    if iq2020_version_controller := config.get(CONF_IQ2020_VERSION_CONTROLLER):
        sens = await text_sensor.new_text_sensor(iq2020_version_controller)
        cg.add(iq2020_component.set_iq2020_version_controller_text_sensor(sens))
    
    if iq2020_version_display := config.get(CONF_IQ2020_VERSION_DISPLAY):
        sens = await text_sensor.new_text_sensor(iq2020_version_display)
        cg.add(iq2020_component.set_iq2020_version_display_text_sensor(sens))
    
    if iq2020_version_other_a := config.get(CONF_IQ2020_VERSION_OTHER_A):
        sens = await text_sensor.new_text_sensor(iq2020_version_other_a)
        cg.add(iq2020_component.set_iq2020_version_other_a_text_sensor(sens))
    
    if iq2020_version_other_b := config.get(CONF_IQ2020_VERSION_OTHER_B):
        sens = await text_sensor.new_text_sensor(iq2020_version_other_b)
        cg.add(iq2020_component.set_iq2020_version_other_b_text_sensor(sens))

    if cfg := config.get(CONF_SPA_STATE):
        sens = await text_sensor.new_text_sensor(cfg)
        cg.add(iq2020_component.set_spa_state_text_sensor(sens))
    if cfg := config.get(CONF_LIGHT_STATE):
        sens = await text_sensor.new_text_sensor(cfg)
        cg.add(iq2020_component.set_light_state_text_sensor(sens))
    if cfg := config.get(CONF_RTC_STATUS):
        sens = await text_sensor.new_text_sensor(cfg)
        cg.add(iq2020_component.set_rtc_status_text_sensor(sens))
    if cfg := config.get(CONF_RTC):
        sens = await text_sensor.new_text_sensor(cfg)
        cg.add(iq2020_component.set_rtc_text_sensor(sens))
    if cfg := config.get(CONF_SWG_STATUS):
        sens = await text_sensor.new_text_sensor(cfg)
        cg.add(iq2020_component.set_swg_status_text_sensor(sens))
    if cfg := config.get(CONF_SWG_TYPE):
        sens = await text_sensor.new_text_sensor(cfg)
        cg.add(iq2020_component.set_swg_type_text_sensor(sens))

