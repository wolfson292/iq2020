#include "clean_mode_switch.h"

namespace esphome {
namespace iq2020 {

static const char *TAG = "iq2020.switch.CleanMode";

void CleanModeSwitch::write_state(bool state) {
  ESP_LOGI(TAG, "write_state %s", state ? "True" : "False");
  this->parent_->sendCmdSetCleanMode(state);
}

}  // namespace iq2020
}  // namespace esphome
