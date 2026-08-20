#pragma once
#include "esphome/core/defines.h"
#include "esphome/core/component.h"
#include "esphome/components/text/text.h"
#include "esphome/components/button/button.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/number/number.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/automation.h"
#include "esphome/core/helpers.h"

#include <map>
#include <queue>
#include <tuple>

#include <inttypes.h>

namespace esphome {
namespace iq2020 {

#define GETBIT8(a, b) ((a) & ((uint8_t) 1 << (b)))

// Silence that means "whatever was part-received is abandoned". The controller
// applies the same rule to its own receive path.
static const uint32_t IQ2020_INTERFRAME_GAP_MS = 100;

class IQ2020Device;

class IQ2020Component : public PollingComponent, public uart::UARTDevice {
  SUB_TEXT_SENSOR(iq2020_debug)
  SUB_TEXT_SENSOR(iq2020_version_controller)
  SUB_TEXT_SENSOR(iq2020_version_display)
  SUB_TEXT_SENSOR(iq2020_version_other_a)
  SUB_TEXT_SENSOR(iq2020_version_other_b)
  SUB_TEXT_SENSOR(spa_state)
  SUB_TEXT_SENSOR(light_state)
  SUB_TEXT_SENSOR(rtc_status)
  SUB_TEXT_SENSOR(rtc)
  SUB_TEXT_SENSOR(swg_status)
  SUB_TEXT_SENSOR(swg_type)
  
  SUB_BUTTON(swg_test)

  SUB_BINARY_SENSOR(summer_timer)
  SUB_BINARY_SENSOR(spa_lock)
  SUB_BINARY_SENSOR(temp_lock)
  SUB_BINARY_SENSOR(clean_lock)
  SUB_BINARY_SENSOR(pump)
  SUB_BINARY_SENSOR(swg_generating)
  SUB_BINARY_SENSOR(swg_boost)
  SUB_BINARY_SENSOR(swg_cartridge_due)
  SUB_BINARY_SENSOR(swg_cartridge_present)
  SUB_BINARY_SENSOR(swg_level_locked)
  SUB_BINARY_SENSOR(econ_mode)
  SUB_BINARY_SENSOR(circulation)
  
  SUB_SENSOR(jets1_timeout)
  SUB_SENSOR(jets2_timeout)
  SUB_SENSOR(jets3_timeout)
  SUB_SENSOR(blower_timeout)
  SUB_SENSOR(lights_timeout)
  SUB_SENSOR(jets1_speed)
  SUB_SENSOR(jets2_speed)
  SUB_SENSOR(jets3_speed)
  SUB_SENSOR(blower_speed)
  SUB_SENSOR(high_limit_temp)
  SUB_SENSOR(heater_seconds)
  SUB_SENSOR(jet1_seconds)
  SUB_SENSOR(lifetime_seconds)
  SUB_SENSOR(lost_lines)
  SUB_SENSOR(jet2_seconds)
  SUB_SENSOR(jet3_seconds)
  SUB_SENSOR(blower_seconds)
  SUB_SENSOR(lights_seconds)
  SUB_SENSOR(pump_seconds)
  SUB_SENSOR(jet1_low_seconds)
  SUB_SENSOR(jet2_low_seconds)
  SUB_SENSOR(temp_set)
  SUB_SENSOR(water_temp)
  SUB_SENSOR(l1_voltage)
  SUB_SENSOR(heater_voltage)
  SUB_SENSOR(l2_voltage)
  SUB_SENSOR(jets3_voltage)
  SUB_SENSOR(l1_current)
  SUB_SENSOR(heater_current)
  SUB_SENSOR(l2_current)
  SUB_SENSOR(jets3_current)
  SUB_SENSOR(l1_power)
  SUB_SENSOR(l2_power)
  SUB_SENSOR(jets3_power)
  SUB_SENSOR(heater_power)
  SUB_SENSOR(filter1_time)
  SUB_SENSOR(filter2_time)
  SUB_SENSOR(pcb_temp)
  SUB_SENSOR(periph_current)
  SUB_SENSOR(rtc_seconds)
  SUB_SENSOR(rtc_minutes)
  SUB_SENSOR(rtc_hours)
  SUB_SENSOR(rtc_days)
  SUB_SENSOR(rtc_months)
  SUB_SENSOR(rtc_years)
  SUB_SENSOR(daily_clean_cycle)
  SUB_SENSOR(swg_age)
  SUB_SENSOR(swg_boost_mode)
  SUB_SENSOR(swg_error)
  SUB_SENSOR(swg_salinity)
  SUB_SENSOR(swg_salinity_index)
  SUB_SENSOR(swg_cell_runtime)
  SUB_SENSOR(swg_spa_size)
  SUB_SENSOR(swg_output_level)
  SUB_SENSOR(swg_salt_test)
  SUB_SENSOR(swg_cell_state)
  
  SUB_SWITCH(clean_mode)
  SUB_SWITCH(jets1)
  SUB_SWITCH(jets2)
  SUB_SWITCH(jets3)
  SUB_SWITCH(blower)
  SUB_SWITCH(summer_timer)
  SUB_SWITCH(spa_lock)
  SUB_SWITCH(temp_lock)
  SUB_SWITCH(swg_boost)
  SUB_SWITCH(capture)
  
  SUB_NUMBER(swg_level)

 public:
  void setup() override;
  void dump_config() override;
  void loop() override;
  void read_all_info();
  void update() override;
  void sendSWGBoostCmd(bool enableBoost);
  void sendSWGTestCmd();
  void sendSWGDefault();
  void sendCMDGetVersions();
  void sendCMDGetTimeStamps();
  void sendCMDGetData();
  void sendCMDGetDataExtended();
  void sendCMDGetLightStatus(bool force = false);
  void sendCMDGetSWG();
  void sendCmdSetCleanMode(bool state);
  void sendCmdSetTemp(float temp_c);
  // jet is 1..3, speed 0 = off. Two-speed pumps accept 1 and 2; single-speed
  // pumps are coerced to 2 by the controller.
  void sendCmdSetJets(uint8_t jet, uint8_t speed);
  void sendCmdSetJets(uint8_t jet, bool on) { this->sendCmdSetJets(jet, (uint8_t)(on ? 2 : 0)); }
  void sendCmdQueryJets(uint8_t jet);
  void sendCmdSetBlower(bool on);
  void sendCmdSetSummerTimer(bool on);
  void sendCmdSetSpaLock(bool on);
  void sendCmdSetTempLock(bool on);
  void sendCMDGetFilterConfig();
  void sendCmdSetSWG(float value);
  void sendCmdSetLightColorUp(uint8_t lightNum);
  void sendCmdSetLightColorDown(uint8_t lightNum);
  void sendCmdSetLightBrightUp(uint8_t lightNum);
  void sendCmdSetLightBrightDown(uint8_t lightNum);
  void sendCmdSetLightAllOn();
  void sendCmdSetLightAllOff();
  void sendCmdSetLightCycleOn(uint8_t lightNum);
  void sendCmdSetLightCycleOff(uint8_t lightNum);
  void sendCmdSetLightSpeedUp(uint8_t lightNum);
  void sendCmdSetLightSpeedDown(uint8_t lightNum);
  void set_light_cycle(uint8_t lightNum, bool cycling);
  void set_light_speed(uint8_t lightNum, uint8_t speed, bool skip_status_update = false);
  void sendCmdReset();
  void sendCmdTransmitNudge();
  // 0 disables the automatic nudge; otherwise the quiet period after which one
  // is sent, and the minimum gap between them.
  void set_nudge_timeout(uint32_t ms) { nudge_timeout_ms_ = ms; }
  // Bus silence required before transmitting. Must clear the controller's
  // request/reply pairs, which run 8-12 ms apart.
  void set_bus_idle_time(uint32_t ms) { bus_idle_ms_ = ms; }
  void sendCmdSetDateTime(uint8_t seconds, uint8_t minutes, uint8_t hours, uint8_t days, uint8_t months, uint16_t years);
  void sendResponseEmulateAudio();
  void sendResponseEmulateAudioTitle();
  void sendResponseEmulateAudioArtist();
  void sendCmdGetAudio();
  void sendCmdSetAudioSource(uint8_t source);
  void set_light_color(uint8_t lightNum, uint8_t color_num, bool skip_status_update = false);
  void set_light_brightness(uint8_t lightNum, bool On, uint8_t brightness, bool skip_status_update = false);

  void music_set_artist(std::string value) { audio_artist_ = value; }
  void music_set_song(std::string value) { audio_title_ = value; }

  std::string decodeLightColor_(uint8_t raw);
  std::string decodeLightIntensity_(uint8_t raw);
  std::string decodeLightNumber_(uint8_t raw);
  std::string decodeLightOperation_(uint8_t raw);
  std::string decodeAddr_(uint8_t raw);
  std::string decodeLightSpeed_(uint8_t raw);
  std::string decodeSWGStatus_(uint8_t raw);
  std::vector<uint8_t> buildSWGCommand_();
  uint8_t swgAddr_();

  // Salinity index -> position along the panel's salt scale, 0-100%.
  static float swg_salinity_from_index(uint8_t index);
  

  void set_simulate_music(bool simulate) { simulate_music_ = simulate; }

  void set_device(IQ2020Device *device) { 
    this->devices_.push_back(device);
  };

  void add_on_music_volume_up_callback(std::function<void()> &&callback) {
    this->music_volume_up_callback_.add(std::move(callback));
  }
  void add_on_music_volume_down_callback(std::function<void()> &&callback) {
    this->music_volume_down_callback_.add(std::move(callback));
  }
  void add_on_music_play_callback(std::function<void()> &&callback) {
    this->music_play_callback_.add(std::move(callback));
  }
  void add_on_music_pause_callback(std::function<void()> &&callback) {
    this->music_pause_callback_.add(std::move(callback));
  }
  void add_on_music_next_callback(std::function<void()> &&callback) {
    this->music_next_callback_.add(std::move(callback));
  }
  void add_on_music_back_callback(std::function<void()> &&callback) {
    this->music_back_callback_.add(std::move(callback));
  }
  void add_on_music_off_callback(std::function<void()> &&callback) {
    this->music_off_callback_.add(std::move(callback));
  }
  void add_on_music_on_callback(std::function<void()> &&callback) {
    this->music_on_callback_.add(std::move(callback));
  }


 protected:
  void restart_();
  bool readline_(int readch, uint8_t *buffer, int len);
  void sendCmd_(uint8_t source, uint8_t dest, std::vector<uint8_t> data, bool request = true);
  void sendCmd_(uint8_t source, uint8_t dest, uint8_t retries, std::vector<uint8_t> data, bool request = true);

  uint32_t read_uint32(std::vector<uint8_t> data, uint8_t offset) {
    return data[offset] | data[offset + 1] << 8 | data[offset + 2] << 16 | data[offset + 3] << 24;
  }

  uint16_t read_uint16(std::vector<uint8_t> data,  uint8_t offset) {
    return data[offset] | data[offset + 1] << 8;
  }

  std::string buffer_to_string_(std::vector<uint8_t> data) {
    std::string res;
    char buf[8];
    for (int i = 0; i < data.size(); i++) {
        if (i > 0) {
            res += ':';
        }
        sprintf(buf, "%02X", data[i]);
        res += buf;
    }
    return res;
  }

  CallbackManager<void()> music_volume_up_callback_{};
  CallbackManager<void()> music_volume_down_callback_{};
  CallbackManager<void()> music_play_callback_{};
  CallbackManager<void()> music_pause_callback_{};
  CallbackManager<void()> music_next_callback_{};
  CallbackManager<void()> music_back_callback_{};
  CallbackManager<void()> music_off_callback_{};
  CallbackManager<void()> music_on_callback_{};



  std::vector<IQ2020Device *> devices_;



  // last send operation
  uint32_t last_recv_timestamp_;
  uint32_t last_send_timestamp_;
  uint32_t startup_timestamp_;
  uint32_t last_nudge_timestamp_ = 0;
  uint32_t nudge_timeout_ms_ = 0;
  uint32_t bus_idle_ms_ = 20;
  uint32_t last_rx_byte_ms_ = 0;
  int rx_pos_ = 0;
  bool startup_delay_passed_ = false;
  bool sent_get_lights_since_last_cycle_ = false;

  // last recv by sender/reciver
  std::map<std::tuple<uint8_t,uint8_t>,std::tuple<uint32_t,std::vector<uint8_t>>> last_type_recv_timestamp_;

  // send queue tuple<retries,current_attempt,source,dest,data,id,request>
  std::queue<std::tuple<uint8_t, uint8_t, uint8_t, uint8_t, std::vector<uint8_t>, uint32_t, bool >> send_queue_;

  uint8_t remote_addr_ = 0x1f;
  uint8_t music_addr_ = 0x33;
  uint8_t iq2020_addr_ = 0x01;

  float last_set_temp_c_ = 0;
  float last_water_temp_c_ = 0;

  float last_set_temp_f_ = 0;
  float last_water_temp_f_ = 0;

  bool temp_in_c_ = false;

  bool audio_responded_ = false;

  // Salt Water Generator state, decoded from the module's own 1E/01 frames.
  // 0x24 = ACE, 0x29 = FreshWater ("legacy"); the address selects the variant
  // the controller decodes, so we key off it the same way.
  uint8_t swg_addr_ = 0;
  uint8_t swg_level_reported_ = 0xFF;
  uint8_t swg_test_value_ = 0xFF;   // payload[1] of the module's reply
  uint8_t swg_spa_size_ = 0xFF;     // from the controller's 1E/03 summary
  uint8_t swg_status_class_ = 0xFF; // payload[2] & 3
  uint8_t swg_salinity_idx_ = 0xFF; // payload[2] >> 2
  uint8_t swg_cell_days_ = 0xFF;    // payload[3]
  uint8_t swg_flags_ = 0;           // payload[5]
  uint8_t swg_error_code_ = 0;      // payload[6]
  uint32_t swg_runtime_ = 0;        // payload[8..10], 24-bit LE
  bool swg_seen_ = false;

  std::vector<uint8_t> light_speed_ = { 255, 255, 255, 255 };
  std::vector<uint8_t> light_cycling_ = { 255, 255, 255, 255 };
  std::vector<uint8_t> light_color_ = { 255, 255, 255, 255 };
  std::vector<uint8_t> light_brightness_ = { 255, 255, 255, 255 };
  std::vector<uint8_t> stabalize_counter_ = { 0, 0, 0, 0 };
  uint8_t lights_max_brightness_ = 255;

  bool simulate_music_;
  struct audio_status {
    uint8_t header_1 = 0x19;
    uint8_t header_2 = 0x01;
    uint8_t power = 1;
    uint8_t volume = 0x19;
    uint8_t treble = 0x00;
    uint8_t bass = 0x00;
    uint8_t balance = 0x00;
    uint8_t sub_volume = 0x0B;
    uint8_t play_pause = 0x00;
    uint8_t source = 0x04;
    uint8_t channel = 0x01;
    uint8_t signal = 0x00;
    uint8_t bluetooth = 0x00;
  };

  audio_status audio_status_;
  std::string audio_title_ = "Happy Family";
  std::string audio_artist_ = "Scott Wolf";

};



class IQ2020Device {
 public:
  virtual void on_water_temp(float temp_c) = 0;
  virtual void on_set_temp(float temp_c) = 0;
  virtual void on_heating(bool heating) = 0;
  virtual void on_light(uint8_t light_num, uint8_t brightness, uint8_t color) = 0;
  // Optional: also receives the colour-cycle state the 17/05 response carries.
  virtual void on_light_cycle(uint8_t light_num, bool cycling, uint8_t speed) {}
  
 protected:
  friend IQ2020Component;
};

}  // namespace iq2020
}  // namespace esphome
