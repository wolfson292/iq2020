#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "iq2020.h"

namespace esphome {
namespace iq2020 {

class VolumeUpTrigger : public Trigger<> {
 public:
  explicit VolumeUpTrigger(IQ2020Component *parent) {
    parent->add_on_music_volume_up_callback([this]() { this->trigger(); });
  }
};

}  // namespace number
}  // namespace iq2020
