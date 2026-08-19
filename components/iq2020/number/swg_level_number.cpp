#include "swg_level_number.h"

namespace esphome {
namespace iq2020 {

void SWGLevelNumber::control(float value) {
  this->publish_state(value);
  this->parent_->sendCmdSetSWG(value);
}

}  // namespace iq2020
}  // namespace esphome
