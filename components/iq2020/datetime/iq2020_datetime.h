#pragma once

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/components/datetime/datetime_entity.h"
#include "../iq2020.h"

namespace esphome {
namespace iq2020 {

class IQ2020DateTime : public datetime::DateTimeEntity, public Parented<IQ2020Component> {
 public:
  IQ2020DateTime() = default;

 protected:
  void control(const datetime::DateTimeCall &call) override;
};

;

}  // namespace iq2020
}  // namespace esphome
