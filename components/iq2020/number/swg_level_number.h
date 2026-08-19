#pragma once

#include "esphome/components/number/number.h"
#include "../iq2020.h"

namespace esphome {
namespace iq2020 {

class SWGLevelNumber : public number::Number, public Parented<IQ2020Component> {
 public:
  SWGLevelNumber() = default;

 protected:
  void control(float value) override;
};

}  // namespace iq2020
}  // namespace esphome
