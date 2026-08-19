#pragma once

#include "esphome/components/number/number.h"
#include "esphome/components/light/light_output.h"
#include "../iq2020.h"

namespace esphome {
namespace iq2020 {

static const char *TAG = "iq2020.light";

class SpaLight : public Component, public light::LightOutput, public IQ2020Device, public Parented<IQ2020Component> {
 public:
  void setup() override;
  light::LightTraits get_traits() override {
    auto traits = light::LightTraits();
    traits.set_supported_color_modes({light::ColorMode::BRIGHTNESS});
    return traits;
  }
  void setup_state(light::LightState *state) override { this->light_state_ = state; }
  void write_state(light::LightState *state) override;
  void set_light_num(uint8_t light_num) {
    light_num_ = light_num;
  }
  void on_water_temp(float temp_c) override;
  void on_set_temp(float temp_c) override;
  void on_heating(bool heating) override;
  void on_light(uint8_t light_num, uint8_t brightness, uint8_t color) override;
  void set_color(int8_t color_num);
  uint8_t light_num() { return light_num_; }

 protected:
    light::LightState *light_state_{nullptr};
    uint8_t light_num_;
    uint8_t last_on_ = 255;
    float last_brightness_ = 100;
    uint8_t last_color_update_ = 255;
    uint8_t last_brightnessLevel_update_ = 255;
};

class SpaLightColor : public light::LightEffect {
  public:
    explicit SpaLightColor(const char *name, uint8_t color_num) : LightEffect(name) {
      this->color_num_ = color_num;
    }
    void apply() override { }
    void start() override { 
      auto light = this->get_spa_light_();
      if(light != nullptr) {
        ESP_LOGI(TAG, "Effect SpaLightColor Start Light:%s Effect:%s Color:%s", light->get_parent()->decodeLightNumber_(light->light_num()).c_str(), this->get_name().c_str(), light->get_parent()->decodeLightColor_(this->color_num_).c_str());
        light->set_color(this->color_num_);
      }
      this->stop();
    }
  protected:
    SpaLight *get_spa_light_() const {
      if(this->state_ != nullptr)
      {
        auto output = this->state_->get_output();
        if(output != nullptr)
        {
          return (SpaLight *)  output;
        }
      }
      return nullptr;
    }
    uint8_t color_num_;
};

}  // namespace iq2020
}  // namespace esphome
