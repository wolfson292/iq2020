#pragma once

#include "esphome/components/switch/switch.h"
#include "iq2020_switch.h"
#include "../iq2020.h"

namespace esphome {
namespace iq2020 {

class Jets1Switch : public IQ2020SwitchBase, public Parented<IQ2020Component> {
 public:
  Jets1Switch() = default;

 protected:
  void write_state(bool state) override;
};

}  // namespace iq2020
}  // namespace esphome
