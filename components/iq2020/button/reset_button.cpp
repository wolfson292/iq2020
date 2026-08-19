#include "reset_button.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

static const char *const TAG = "IQ2020.button";

namespace esphome {
namespace iq2020 {

void IQ2020ResetButton::press_action() {
     this->parent_->sendCmdReset();
}


}  // namespace iq2020
}  // namespace esphome
