#pragma once

#include "esphome/components/button/button.h"
#include "../iq2020.h"

namespace esphome {
namespace iq2020 {

// Starts the salt module's water test - the same action as the test button on
// the panel. The module reports progress through the salt status; the reading it
// produces lands in swg_salt_test.
class SWGTestButton : public button::Button, public Parented<IQ2020Component> {
 public:
  SWGTestButton() = default;

 protected:
  void press_action() override;
};

}  // namespace iq2020
}  // namespace esphome
