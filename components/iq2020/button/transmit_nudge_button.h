#pragma once

#include "esphome/components/button/button.h"
#include "../iq2020.h"

namespace esphome {
namespace iq2020 {

// Sends 02/50, telling the controller it may transmit whatever it has queued and
// clearing its retry backoff. Changes no setting, so it is safe to press when
// the bus looks stuck.
class TransmitNudgeButton : public button::Button, public Parented<IQ2020Component> {
 public:
  TransmitNudgeButton() = default;

 protected:
  void press_action() override;
};

}  // namespace iq2020
}  // namespace esphome
