import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID
from esphome import automation
from esphome.automation import maybe_simple_id
from esphome.cpp_helpers import gpio_pin_expression
from esphome.const import (
    CONF_FLOW_CONTROL_PIN,
    CONF_TRIGGER_ID,
)
from esphome import pins

DEPENDENCIES = ["uart"]

# iq2020.h includes each of these platform headers unconditionally, so the base
# component pulls them in whether or not the user configured that platform.
# Without this, a config that omits e.g. `text:` fails to compile on a missing
# esphome/components/text/text.h.
AUTO_LOAD = [
    "binary_sensor",
    "button",
    "number",
    "sensor",
    "switch",
    "text",
    "text_sensor",
]
CODEOWNERS = ["@wolfson292"]
MULTI_CONF = True

iq2020_ns = cg.esphome_ns.namespace("iq2020")
IQ2020Component = iq2020_ns.class_("IQ2020Component", cg.PollingComponent, uart.UARTDevice)

CONF_IQ2020_ID = "iq2020_id"
CONF_SIMULATE_MUSIC = "simulate_music"
CONF_NUDGE_TIMEOUT = "nudge_timeout"
CONF_ON_MUSIC_VOLUME_UP = "on_music_volume_up"

# Triggers
VolumeUpTrigger = iq2020_ns.class_(
    "VolumeUpTrigger", automation.Trigger.template()
)


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(IQ2020Component),
        cv.Optional(CONF_SIMULATE_MUSIC, default = 'false'): cv.boolean,
        # How long the controller may stay silent before a 02/50 transmit nudge
        # is sent to un-stick it. Defaults to off: the nudge is harmless, but
        # putting extra traffic on the bus should be an explicit choice.
        cv.Optional(CONF_NUDGE_TIMEOUT, default = '0s'): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_ON_MUSIC_VOLUME_UP): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(VolumeUpTrigger),
            }
        ),
    }
)

CONFIG_SCHEMA = cv.All(
    CONFIG_SCHEMA.extend(uart.UART_DEVICE_SCHEMA).extend(cv.polling_component_schema('60s'))
)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "iq2020",
    require_tx=True,
    require_rx=True,
    parity="NONE",
    stop_bits=1,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    cg.add(var.set_simulate_music(config[CONF_SIMULATE_MUSIC]))
    cg.add(var.set_nudge_timeout(config[CONF_NUDGE_TIMEOUT]))
    
    for conf in config.get(CONF_ON_MUSIC_VOLUME_UP, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)


CALIBRATION_ACTION_SCHEMA = maybe_simple_id(
    {
        cv.Required(CONF_ID): cv.use_id(IQ2020Component),
    }
)
