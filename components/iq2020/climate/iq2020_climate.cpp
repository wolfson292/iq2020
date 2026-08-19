#include "esphome/core/defines.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "esphome/components/climate/climate_traits.h"
#include "esphome/components/climate/climate_mode.h"
#include "iq2020_climate.h"


namespace esphome {
namespace iq2020 {

	static const char *TAG = "iq2020.climate";

	void IQ2020Climate::setup() {
	}

	void IQ2020Climate::control(const climate::ClimateCall &call) {
		if (call.get_target_temperature().has_value()) {
			ESP_LOGI(TAG, "Set Temp %f", call.get_target_temperature().value());
			this->parent_->sendCmdSetTemp(call.get_target_temperature().value());
		}
	}

	climate::ClimateTraits IQ2020Climate::traits() {
		auto traits = climate::ClimateTraits();
		traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
		traits.set_supported_modes({climate::CLIMATE_MODE_HEAT});
		traits.add_feature_flags(climate::CLIMATE_SUPPORTS_ACTION);
		traits.set_visual_min_temperature(fahrenheit_to_celsius(80));
		traits.set_visual_max_temperature(fahrenheit_to_celsius(104));
		traits.set_visual_target_temperature_step(1);
		traits.set_visual_current_temperature_step(1);
		return traits;
	}

	void IQ2020Climate::dump_config() {
		LOG_CLIMATE("", "IQ2020 Heater", this);
	}

	void IQ2020Climate::on_water_temp(float temp_c) {
		this->current_temperature = temp_c;
		publish_state();
	}

	void IQ2020Climate::on_heating(bool heating) {
		this->action = heating ? esphome::climate::ClimateAction::CLIMATE_ACTION_HEATING : esphome::climate::ClimateAction::CLIMATE_ACTION_IDLE;
		publish_state();
	}

	void IQ2020Climate::on_set_temp(float temp_c) {
		this->target_temperature = temp_c;
		this->mode = esphome::climate::ClimateMode::CLIMATE_MODE_HEAT;
		publish_state();
	}

	void IQ2020Climate::on_light(uint8_t light_num, uint8_t brightness, uint8_t color) {
		
	}
	

} //namespace iq2020
} //namespace esphome