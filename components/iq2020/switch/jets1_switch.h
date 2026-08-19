#pragma once

#include "esphome/components/switch/switch.h"
#include "../iq2020.h"

namespace esphome {
namespace iq2020 {

class Jets1Switch : public switch_::Switch, public Parented<IQ2020Component> {
 public:
  Jets1Switch() = default;

 protected:
  void write_state(bool state) override;
};

}  // namespace iq2020
}  // namespace esphome
