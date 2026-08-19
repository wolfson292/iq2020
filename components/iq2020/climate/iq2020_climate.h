#pragma once

#include <map>
#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "../iq2020.h"

namespace esphome {
namespace iq2020 {

	class IQ2020Climate : public climate::Climate, public Component, public IQ2020Device, public Parented<IQ2020Component> {
	public:
		void setup() override;
		void dump_config() override;
		void on_water_temp(float temp_c) override;
		void on_set_temp(float temp_c) override;
		void on_heating(bool heating) override;
		void on_light(uint8_t light_num, uint8_t brightness, uint8_t color) override;

	protected:
		void control(const climate::ClimateCall &call) override;
		climate::ClimateTraits traits() override;
	};

} //namespace iq2020
} //namespace esphome