#include "spa_light.h"

namespace esphome {
namespace iq2020 {

  void SpaLight::setup() {
    light_state_->set_default_transition_length(0);
    // Colour indices are 1..7 on the wire - the controller wraps 7 -> 1 and never
    // produces 0. Effects are 1-based in the order they are added, so effect N
    // lines up with colour N and on_light() can pass the raw value straight in.
    //
    // The old list started at 0 and carried an eighth "Rainbow" entry; the colour
    // cycle turned out to be a separate per-light flag, not a colour, so it is
    // gone from here and exposed through set_light_cycle() instead.
    auto *violet = new SpaLightColor("Violet", 0x01);
    auto *blue = new SpaLightColor("Blue", 0x02);
    auto *cyan = new SpaLightColor("Cyan", 0x03);
    auto *green = new SpaLightColor("Green", 0x04);
    auto *white = new SpaLightColor("White", 0x05);
    auto *yellow = new SpaLightColor("Yellow", 0x06);
    auto *red = new SpaLightColor("Red", 0x07);
    auto *cycle = new SpaLightCycle("Color Cycle");
    light_state_->add_effects({violet, blue, cyan, green, white, yellow, red, cycle});
    violet->init_internal(this->light_state_);
    blue->init_internal(this->light_state_);
    cyan->init_internal(this->light_state_);
    green->init_internal(this->light_state_);
    white->init_internal(this->light_state_);
    yellow->init_internal(this->light_state_);
    red->init_internal(this->light_state_);
    cycle->init_internal(this->light_state_);
	}
 
  void SpaLight::write_state(light::LightState *state) {
    auto brightness_float = state->current_values.get_brightness();
    auto state_float = state->current_values.get_state();
    
    if(light_num_ == 4 && state_float == 0)
    {
      brightness_float = 0;
    }
    
    auto binary = brightness_float > 0 && state_float > 0;

    ESP_LOGI(TAG, "write_state Light:%s CurrentBrightness:%f CurrentState:%f RemoteBrightness:%f RemoteState:%f Bright:%f Binary:%d", 
          this->parent_->decodeLightNumber_(this->light_num_).c_str(), 
          state->current_values.get_brightness(),
          state->current_values.get_state(),
          state->remote_values.get_brightness(),
          state->remote_values.get_state(),
          brightness_float, binary
    );
    
    uint8_t brightnessLevel = 0;
    if(brightness_float > 0 && brightness_float <= .2f) {
      brightnessLevel = 1;
    
    } else if (brightness_float > .2f && brightness_float <= .4f) {
      brightnessLevel = 2;
    
    } else if (brightness_float > .4f && brightness_float <= .6f) {
      brightnessLevel = 3;
    
    } else if (brightness_float > .6f && brightness_float <= .8f) {
      brightnessLevel = 4;
    
    } else if (brightness_float > .8f) {
      brightnessLevel = 5;
    } else {
      ESP_LOGW(TAG, "write_state Light:%s Unable To Convert Brightness:%f to BrightnessLevel", 
          this->parent_->decodeLightNumber_(this->light_num_).c_str(),
          brightness_float
      );
    }

    ESP_LOGI(TAG, "write_state Light:%s Convert Brightness:%f to BrightnessLevel:%s", 
          this->parent_->decodeLightNumber_(this->light_num_).c_str(),
          brightness_float,
          this->parent_->decodeLightIntensity_(brightnessLevel).c_str()
    );


    ESP_LOGI(TAG, "write_state Light:%s Binary:%d LastOn:%d Bright:%f LastBright:%f", 
          this->parent_->decodeLightNumber_(this->light_num_).c_str(), 
          binary, this->last_on_,
          brightness_float, this->last_brightness_
    );


    if(binary != this->last_on_ || brightness_float != this->last_brightness_ || light_num_ == 4)
    {
     
      if(light_num_ == 4 && brightnessLevel == last_brightnessLevel_update_) {
        ESP_LOGI(TAG, "write_state  set_light_brightness Ignoring All Round Trip Light:%s - BrightnessLevel:%s == LastBrightnessLevelUpdate:%s", 
          this->parent_->decodeLightNumber_(this->light_num_).c_str(), 
          this->parent_->decodeLightIntensity_(brightnessLevel).c_str(), 
          this->parent_->decodeLightIntensity_(last_brightnessLevel_update_).c_str()
        );
        last_brightnessLevel_update_ = 255;
        return;
      }
      
      ESP_LOGI(TAG, "write_state  set_light_brightness Light:%s - BrightnessLevel:%s != LastBrightnessLevelUpdate:%s - Last Brightness:%f On:%d - New Brightness:%f On:%d", 
        this->parent_->decodeLightNumber_(this->light_num_).c_str(), 
        this->parent_->decodeLightIntensity_(brightnessLevel).c_str(), 
        this->parent_->decodeLightIntensity_(last_brightnessLevel_update_).c_str(), 
        this->last_brightness_, this->last_on_,
        brightness_float, binary
      );
      this->parent_->set_light_brightness(this->light_num_, binary, brightnessLevel);
    } 
  }
  

  void SpaLight::on_water_temp(float temp_c) {
	}

	void SpaLight::on_heating(bool heating) {
	}

	void SpaLight::on_set_temp(float temp_c) {
	}

  void SpaLight::on_light(uint8_t light_num, uint8_t brightLevel, uint8_t color) {
    // status from iq2020 to remote
    if(light_num == light_num_) {
      auto state_on = brightLevel > 0;
      float brightness = 0.0f;
      if(brightLevel == 0) {
        brightness = 0.0f;
      } else if(brightLevel == 1) {
        brightness = 0.2f;
      } else if(brightLevel == 2) {
        brightness = 0.4f;
      } else if(brightLevel == 3) {
        brightness = 0.6f;
      } else if(brightLevel == 4) {
        brightness = 0.8f;
      } else if(brightLevel == 5) {
        brightness = 1.0f;
      }

      this->last_brightnessLevel_update_ = brightLevel;
      
      ESP_LOGI(TAG, "on_light (from IQ2020) Light:%s State:%s BrightnessLevel:%s Brightness:%f Brightness%%:%.0f%% Color:%s StateCurrentEqRemote:%d StateCurrentBrightness:%f StateRemoteBrightness:%f", 
        this->parent_->decodeLightNumber_(light_num).c_str(), 
        state_on ? "On" : "Off", 
        this->get_parent()->decodeLightIntensity_(brightLevel).c_str(),
          brightness, brightness * 100.0f, 
          light_num < 4 ? this->get_parent()->decodeLightColor_(color).c_str() : "Ignored",
          this->light_state_->current_values == this->light_state_->remote_values,
          this->light_state_->current_values.get_brightness(),
          this->light_state_->remote_values.get_brightness()
         );



      this->light_state_->remote_values.set_brightness(brightness);
      this->light_state_->remote_values.set_state(state_on);
      this->light_state_->current_values.set_brightness(brightness);
      this->light_state_->current_values.set_state(state_on);
      ESP_LOGI(TAG, "on_light (from IQ2020) Light:%s Publish Brightness:%f State:%d", this->parent_->decodeLightNumber_(this->light_num_).c_str(), brightness, state_on);
      this->light_state_->publish_state();

      // Effect N == colour N now that both are 1-based; colour 0 means the
      // controller has not reported one yet, so leave the effect alone.
      // Colour 8 is the panel's eighth swatch, the colour cycle, and lines up
      // with the Color Cycle effect without any special casing. While
      // the zone is cycling, effect 8 (Color Cycle) is showing instead - don't
      // stomp it with the underlying colour.
      if(state_on && light_num_ != 4 && !cycling_ && color >= 1 && color <= 8) {
        light::LightCall call2 = light_state_->make_call();
        call2.set_effect(color);
        call2.perform();
      }
      
      
    }
	}

  void SpaLight::on_light_cycle(uint8_t light_num, bool cycling, uint8_t speed) {
    if(light_num != light_num_) {
      return;
    }
    ESP_LOGI(TAG, "on_light_cycle (from IQ2020) Light:%s Cycling:%d Speed:%s",
      this->parent_->decodeLightNumber_(light_num).c_str(), cycling,
      this->parent_->decodeLightSpeed_(speed).c_str());
    if(cycling == cycling_) {
      return;
    }
    cycling_ = cycling;
    // Effect 8 is Color Cycle; 0 is "no effect". Falling back to 0 rather than
    // the colour avoids a redundant colour command - the next status poll
    // reports the colour and restores the matching effect.
    light::LightCall call = light_state_->make_call();
    call.set_effect(cycling ? 8 : 0);
    call.perform();
  }

  void SpaLight::set_cycle(bool cycling) {
    ESP_LOGI(TAG, "set_cycle Light:%s Cycling:%d", this->parent_->decodeLightNumber_(this->light_num_).c_str(), cycling);
    this->parent_->set_light_cycle(this->light_num_, cycling);
  }

  void SpaLight::set_color(int8_t color_num) {
    ESP_LOGI(TAG, "set_color Light:%s Color:%s", this->parent_->decodeLightNumber_(this->light_num_).c_str(), this->parent_->decodeLightColor_(color_num).c_str());
    this->parent_->set_light_color(this->light_num_, color_num);
  }


}  // namespace iq2020
}  // namespace esphome
