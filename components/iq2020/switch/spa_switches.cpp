#include "spa_switches.h"

namespace esphome {
namespace iq2020 {

static const char *TAG = "iq2020.switch";

void Jets2Switch::write_state(bool state) {
  ESP_LOGI(TAG, "Jets2 write_state %s", state ? "True" : "False");
  this->parent_->sendCmdSetJets(0x02, state);
}

void Jets3Switch::write_state(bool state) {
  ESP_LOGI(TAG, "Jets3 write_state %s", state ? "True" : "False");
  this->parent_->sendCmdSetJets(0x03, state);
}

void BlowerSwitch::write_state(bool state) {
  ESP_LOGI(TAG, "Blower write_state %s", state ? "True" : "False");
  this->parent_->sendCmdSetBlower(state);
}

void SummerTimerSwitch::write_state(bool state) {
  ESP_LOGI(TAG, "SummerTimer write_state %s", state ? "True" : "False");
  this->parent_->sendCmdSetSummerTimer(state);
}

void SpaLockSwitch::write_state(bool state) {
  ESP_LOGI(TAG, "SpaLock write_state %s", state ? "True" : "False");
  this->parent_->sendCmdSetSpaLock(state);
}

void TempLockSwitch::write_state(bool state) {
  ESP_LOGI(TAG, "TempLock write_state %s", state ? "True" : "False");
  this->parent_->sendCmdSetTempLock(state);
}

}  // namespace iq2020
}  // namespace esphome
