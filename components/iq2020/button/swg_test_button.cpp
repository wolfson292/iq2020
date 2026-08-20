#include "swg_test_button.h"

namespace esphome {
namespace iq2020 {

static const char *TAG = "iq2020.button.SWGTest";

void SWGTestButton::press_action() {
  ESP_LOGI(TAG, "press_action - starting SWG water test");
  this->parent_->sendSWGTestCmd();
}

}  // namespace iq2020
}  // namespace esphome
