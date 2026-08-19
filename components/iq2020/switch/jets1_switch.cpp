#include "jets1_switch.h"

namespace esphome {
namespace iq2020 {

static const char *TAG = "iq2020.switch.Jets1";

void Jets1Switch::write_state(bool state) {
  ESP_LOGI(TAG, "write_state %s", state ? "True" : "False");
  this->parent_->sendCmdSetJets(0x01, state);
}

}  // namespace iq2020
}  // namespace esphome
