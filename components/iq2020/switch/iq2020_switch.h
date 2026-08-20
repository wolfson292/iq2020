#pragma once

#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"

namespace esphome {
namespace iq2020 {

// Common base for this component's switches.
//
// switch_::Switch stores a restore mode but does nothing with it - applying it
// is the platform's job, and it only happens if the switch is a Component with a
// setup() that asks for the initial state. Without this, `default_restore_mode`
// is silently ignored: the switch always comes up off no matter what the config
// says. That is exactly what happened to ALWAYS_ON capture.
class IQ2020SwitchBase : public switch_::Switch, public Component {
 public:
  void setup() override {
    optional<bool> initial = this->get_initial_state_with_restore_mode();
    if (initial.has_value()) {
      if (initial.value()) {
        this->turn_on();
      } else {
        this->turn_off();
      }
    }
  }
};

}  // namespace iq2020
}  // namespace esphome
