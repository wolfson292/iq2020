#include "transmit_nudge_button.h"

namespace esphome {
namespace iq2020 {

static const char *TAG = "iq2020.button.TransmitNudge";

void TransmitNudgeButton::press_action() {
  ESP_LOGI(TAG, "press_action - nudging the controller to transmit");
  this->parent_->sendCmdTransmitNudge();
}

}  // namespace iq2020
}  // namespace esphome
