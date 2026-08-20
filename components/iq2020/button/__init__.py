import esphome.codegen as cg
from esphome.components import button
import esphome.config_validation as cv
from esphome.const import (
    DEVICE_CLASS_RESTART,
    ENTITY_CATEGORY_DIAGNOSTIC,
    ENTITY_CATEGORY_CONFIG,
    ICON_RESTART,
    ICON_RESTART_ALERT,
    ICON_DATABASE,
)
from .. import CONF_IQ2020_ID, IQ2020Component, iq2020_ns

IQ2020ResetButton = iq2020_ns.class_("IQ2020ResetButton", button.Button)
SWGTestButton = iq2020_ns.class_("SWGTestButton", button.Button)
TransmitNudgeButton = iq2020_ns.class_("TransmitNudgeButton", button.Button)


CONF_RESET = "reset"
CONF_SWG_TEST = "swg_test"
CONF_TRANSMIT_NUDGE = "transmit_nudge"


CONFIG_SCHEMA = {
    cv.GenerateID(CONF_IQ2020_ID): cv.use_id(IQ2020Component),
    cv.Optional(CONF_RESET): button.button_schema(
        IQ2020ResetButton,
        device_class=DEVICE_CLASS_RESTART,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon=ICON_RESTART_ALERT,
    ),
    cv.Optional(CONF_SWG_TEST): button.button_schema(
        SWGTestButton,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:test-tube",
    ),
    cv.Optional(CONF_TRANSMIT_NUDGE): button.button_schema(
        TransmitNudgeButton,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        icon="mdi:bullhorn-outline",
    ),
}

async def to_code(config):
    iq2020_component = await cg.get_variable(config[CONF_IQ2020_ID])
    if reset := config.get(CONF_RESET):
        b = await button.new_button(reset)
        await cg.register_parented(b, config[CONF_IQ2020_ID])
    if cfg := config.get(CONF_SWG_TEST):
        b = await button.new_button(cfg)
        await cg.register_parented(b, config[CONF_IQ2020_ID])
        cg.add(iq2020_component.set_swg_test_button(b))
    if cfg := config.get(CONF_TRANSMIT_NUDGE):
        b = await button.new_button(cfg)
        await cg.register_parented(b, config[CONF_IQ2020_ID])
