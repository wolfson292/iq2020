#pragma once

#include "esphome/components/switch/switch.h"
#include "../iq2020.h"

namespace esphome {
namespace iq2020 {

// The 0x0B control group is uniform enough that these differ only in which
// command they send. Each one publishes optimistically and is then corrected
// by the controller's reply, which reports the state it actually settled on.

class Jets2Switch : public switch_::Switch, public Parented<IQ2020Component> {
 public:
  Jets2Switch() = default;
 protected:
  void write_state(bool state) override;
};

class Jets3Switch : public switch_::Switch, public Parented<IQ2020Component> {
 public:
  Jets3Switch() = default;
 protected:
  void write_state(bool state) override;
};

class BlowerSwitch : public switch_::Switch, public Parented<IQ2020Component> {
 public:
  BlowerSwitch() = default;
 protected:
  void write_state(bool state) override;
};

class SummerTimerSwitch : public switch_::Switch, public Parented<IQ2020Component> {
 public:
  SummerTimerSwitch() = default;
 protected:
  void write_state(bool state) override;
};

class SpaLockSwitch : public switch_::Switch, public Parented<IQ2020Component> {
 public:
  SpaLockSwitch() = default;
 protected:
  void write_state(bool state) override;
};

class TempLockSwitch : public switch_::Switch, public Parented<IQ2020Component> {
 public:
  TempLockSwitch() = default;
 protected:
  void write_state(bool state) override;
};

}  // namespace iq2020
}  // namespace esphome
