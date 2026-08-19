#pragma once

#include "esphome/core/component.h"
#include "esphome/components/text/text.h"
#include "../iq2020.h"

namespace esphome {
namespace iq2020 {

class IQ2020Text : public text::Text, public Component, public Parented<IQ2020Component> {
 public:
  void setup() override;
  void dump_config() override;
  void set_type(std::string type) {
    type_ = type;
  }

 protected:
  void control(const std::string &value) override;

  std::string type_;

};

}  // namespace iq2020
}  // namespace esphome
