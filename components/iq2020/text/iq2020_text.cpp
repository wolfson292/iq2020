#include "iq2020_text.h"
#include "esphome/core/log.h"

namespace esphome {
namespace iq2020 {

static const char *const TAG = "iq2020.text";

void IQ2020Text::setup() {
    if(this->type_ == "artist") {
        this->publish_state("Wolf Family");
    } else if(this->type_ == "song") {
        this->publish_state("Happy Family");
    }
}

void IQ2020Text::dump_config() {
    ESP_LOGCONFIG("", "IQ2020 Text Type:%s", this, type_.c_str()); 
}

void IQ2020Text::control(const std::string &value) {
    if(this->type_ == "artist") {
        this->parent_->music_set_artist(value);
    } else if(this->type_ == "song") {
        this->parent_->music_set_song(value);
    }
}

}  // namespace copy
}  // namespace iq2020
