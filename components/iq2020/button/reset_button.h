#pragma once

#include "esphome/components/button/button.h"
#include "../iq2020.h"

namespace esphome {
namespace iq2020 {

class IQ2020ResetButton : public button::Button, public Parented<IQ2020Component> {
 public:
  IQ2020ResetButton() = default;

 protected:
  void press_action() override;
};

;

}  // namespace iq2020
}  // namespace esphome
