#include "iq2020.h"
#include "esphome/core/log.h"
#include <cinttypes>
#include <cstdlib>
#include <format>

namespace esphome {
namespace iq2020 {

static const char *TAG = "iq2020.component";
static const char *AUDIO_TAG = "iq2020.audio";

void IQ2020Component::setup() {
    ESP_LOGCONFIG(TAG, "Setting up IQ2020...");
    this->last_recv_timestamp_ = millis();
    this->startup_timestamp_ = millis();

#ifdef USE_SENSOR    
    if (this->water_temp_sensor_ != nullptr) {
        this->water_temp_sensor_->set_force_update(true);
    }

    if (this->temp_set_sensor_!= nullptr) {
        this->temp_set_sensor_->set_force_update(true);
    }

    if (this->high_limit_temp_sensor_ != nullptr) {
        this->high_limit_temp_sensor_->set_force_update(true);
    }
#endif
}

void IQ2020Component::dump_config() {
    ESP_LOGCONFIG(TAG, "Intellichlor RS485 Component");
}

void IQ2020Component::loop() {
    
    auto since_startup = millis() - this->startup_timestamp_;
    if(this->startup_delay_passed_ == false && since_startup<= 5000) {
        ESP_LOGI(TAG, "Delaying until %" PRIu32 " >= 5 seconds...", since_startup);
        return;
    } else if (this->startup_delay_passed_ == false) {
        this->startup_delay_passed_ = true;
        ESP_LOGI(TAG, "Starting processing loop %f seconds after setup", since_startup / 1000.0);
    }
    
    const int max_line_length = 512;
    static uint8_t buffer[max_line_length];

    auto since_last = millis() - this->last_send_timestamp_;
    auto since_last_recv = millis() - this->last_recv_timestamp_;

    // read bytes off the wire first
    while (available()) {
        this->readline_(read(), buffer, max_line_length);
    }

    // Opt-in recovery: if the controller has gone quiet, nudge it into
    // transmitting rather than waiting indefinitely. Disabled unless a timeout
    // is configured, because it puts traffic on the bus on everyone's spa and
    // that should be a deliberate choice. Rate-limited to one nudge per timeout
    // so a genuinely dead controller is not hammered.
    if (this->nudge_timeout_ms_ > 0 && since_last_recv > this->nudge_timeout_ms_ &&
        (millis() - this->last_nudge_timestamp_) > this->nudge_timeout_ms_) {
        ESP_LOGW(TAG, "No frame received for %" PRIu32 " ms - sending transmit nudge",
                 since_last_recv);
        this->sendCmdTransmitNudge();
    }

    // Wait for the bus to be quiet before transmitting.
    //
    // Previously the only gate was 300 ms since our own last send, which takes
    // no account of anyone else. The controller polls the salt module in a tight
    // request/reply pair - measured at 8-12 ms apart - and transmitting into
    // that window corrupts it. A capture bore this out: every corrupted frame
    // over half an hour involved the salt conversation, at roughly the rate a
    // 12 ms window and a 300 ms send interval predict, while our own 2300-frame
    // exchange with the controller was untouched.
    //
    // The controller waits for silence before it transmits; so should we.
    uint32_t since_last_byte = millis() - this->last_rx_byte_ms_;
    bool bus_idle = (this->rx_pos_ == 0) && (since_last_byte >= this->bus_idle_ms_);

    if(since_last > 300 && !bus_idle) {
        this->sends_deferred_++;
        ESP_LOGV(TAG, "deferring send, bus busy (pos %d, %" PRIu32 " ms since last byte)",
                 this->rx_pos_, since_last_byte);
    }

    if(since_last > 300 && bus_idle)
    {
        if (!this->send_queue_.empty()) {
            std::tuple<uint8_t, uint8_t, uint8_t, uint8_t, std::vector<uint8_t>, uint32_t, bool> &packet = this->send_queue_.front();
            auto retries = std::get<0>(packet);
            auto attempts = std::get<1>(packet);
            auto source = std::get<2>(packet);
            auto dest = std::get<3>(packet);
            auto data = std::get<4>(packet);
            auto id = std::get<5>(packet);
            bool request = std::get<6>(packet);

            
            std::get<1>(packet) = std::get<1>(packet) + 1;
            
            ESP_LOGD(TAG, "Process Queue Attempt:%i of Retries:%i - Id:%" PRIu32, attempts, retries, id);
            
            

           

            if(attempts > retries)
            {
                ESP_LOGD(TAG, "Queue Remove Attempts:%i > Retries:%i - Id:%" PRIu32, attempts, retries, id);
                this->send_queue_.pop();
            } else {
                std::vector<uint8_t> packet;
                packet.reserve(data.size()+6);

                packet.push_back(0x1C);
                packet.push_back(dest);
                packet.push_back(source);
                packet.push_back(data.size());
                packet.push_back(request ? 0x40 : 0x80);

                for (int i = 0; i < data.size(); i++)
                {
                    packet.push_back(data[i]);
                }
                
                uint8_t checksum = 0;
                for (int i = 1; i < (data.size() + 5); i++)
                {
                    checksum += packet[i]; 
                }
                packet.push_back(checksum ^ 0xff);
                this->write_array(packet);
                this->flush();

                std::string res;
                char buf[256];
                sprintf(buf, "sendCmd_ %s -> %s Length:%02X Operation:%02X Data:", 
                    decodeAddr_(packet[2]).c_str(),
                    decodeAddr_(packet[1]).c_str(),
                    packet[3],
                    packet[4]
                );
                res += buf;

                for (int i = 0; i < data.size(); i++)
                {
                    if (i > 0) {
                        res += ':';
                    }
                    sprintf(buf, "%02X", data[i]);
                    res += buf;
                }

                sprintf(buf, " Checksum:%02X", 
                    checksum
                );
                res += buf;

                ESP_LOGI(TAG, "%s", res.c_str());

                /*
                if(data[0] == 0x17 && data[1] == 0x02)
                {
                    // light set operation
                    ESP_LOGI(TAG, "sendCmd_ sendCmdSetLight Light:%s Operation:%s Retries:%d Attempts:%d Id:%" PRIu32 "", 
                        this->decodeLightNumber_(data[2]).c_str(), 
                        this->decodeLightOperation_(data[3]).c_str(),
                        retries,
                        attempts,
                        id
                    );
                }
                */
                
                this->last_send_timestamp_ = millis();
            }
        }
    }
}

void IQ2020Component::update() {
    if (this->send_queue_.empty()) {
        ESP_LOGI(TAG, "update");
        this->sendCMDGetVersions(); 
        this->sendCMDGetTimeStamps();
        this->sendCMDGetDataExtended();
        this->sendCMDGetLightStatus();
        this->sendCMDGetSWG();
        this->sendCMDGetFilterConfig();
        if (this->bus_resyncs_sensor_ != nullptr) {
            this->bus_resyncs_sensor_->publish_state(this->bus_resyncs_);
        }
        if (this->sends_deferred_sensor_ != nullptr) {
            this->sends_deferred_sensor_->publish_state(this->sends_deferred_);
        }
        this->sendCmdGetAudio();
        sent_get_lights_since_last_cycle_ = false;
    } else {
        ESP_LOGI(TAG, "send queue not empty, skipping update");
    }
}

void IQ2020Component::sendCMDGetLightStatus(bool force) {
    if(!sent_get_lights_since_last_cycle_ || force)
    {
        if(force) {
            ESP_LOGI(TAG, "forcing sendCMDGetLightStatus");
        } else {
            ESP_LOGI(TAG, "sendCMDGetLightStatus");
        }
        std::vector<uint8_t> data = {0x17, 0x05 };
        this->sendCmd_(remote_addr_, 0x01, data);
    } else {
        ESP_LOGI(TAG, "skipping sendCMDGetLightStatus");
    }
}

void IQ2020Component::sendCMDGetVersions() {
    std::vector<uint8_t> data = {0x01, 0x00 };
    this->sendCmd_(remote_addr_, 0x01, data);
}

void IQ2020Component::sendCMDGetSWG() {
    std::vector<uint8_t> data = {0x1e, 0x03 };
    this->sendCmd_(remote_addr_, 0x01, data);
}

void IQ2020Component::sendCMDGetTimeStamps() {
    std::vector<uint8_t> data = {0x02, 0x4c, 0xFF };
    this->sendCmd_(remote_addr_, 0x01, data);
}


void IQ2020Component::sendCMDGetData() {
    std::vector<uint8_t> data = {0x02, 0x55};
    this->sendCmd_(remote_addr_, 0x01, data);
}

void IQ2020Component::sendCMDGetDataExtended() {
    std::vector<uint8_t> data = {0x02, 0x56};
    this->sendCmd_(remote_addr_, 0x01, data);
}

// The whole 0x0B control group shares one encoding: the payload byte is the
// desired state + 1, so 1 = off and 2 = on. Jets extend that to speeds, where
// the byte is speed + 1. A payload of 0 is a query - the controller changes
// nothing and just answers with the current value.
//
// Every 0x0B command answers with a single byte carrying the resulting state,
// so the reply is worth parsing: it reports what the controller actually did,
// which is not always what was asked. A single-speed pump silently promotes
// speed 1 to speed 2, and an unconfigured jet reports 0 whatever you send.
void IQ2020Component::sendCmdSetJets(uint8_t jet, uint8_t speed)
{
    if(jet < 1 || jet > 3) {
        ESP_LOGW(TAG, "sendCmdSetJets ignoring out-of-range jet:%d", jet);
        return;
    }
    if(speed > 2) {
        ESP_LOGW(TAG, "sendCmdSetJets jet:%d clamping speed:%d to 2", jet, speed);
        speed = 2;
    }
    // 0x0B/0x02, 0x03, 0x04 for jets 1, 2, 3.
    std::vector<uint8_t> data = {0x0b, (uint8_t)(0x01 + jet), (uint8_t)(speed + 1)};
    ESP_LOGI(TAG, "sendCmdSetJets jet:%d speed:%d", jet, speed);
    this->sendCmd_(remote_addr_, 0x01, data);
    this->sendCMDGetDataExtended();
}

void IQ2020Component::sendCmdQueryJets(uint8_t jet)
{
    if(jet < 1 || jet > 3) {
        return;
    }
    std::vector<uint8_t> data = {0x0b, (uint8_t)(0x01 + jet), 0x00};
    this->sendCmd_(remote_addr_, 0x01, data);
}

void IQ2020Component::sendCmdSetBlower(bool on)
{
    ESP_LOGI(TAG, "sendCmdSetBlower %s", on ? "On" : "Off");
    std::vector<uint8_t> data = {0x0b, 0x07, (uint8_t)(on ? 0x02 : 0x01)};
    this->sendCmd_(remote_addr_, 0x01, data);
    this->sendCMDGetDataExtended();
}

void IQ2020Component::sendCmdSetSummerTimer(bool on)
{
    ESP_LOGI(TAG, "sendCmdSetSummerTimer %s", on ? "On" : "Off");
    std::vector<uint8_t> data = {0x0b, 0x1c, (uint8_t)(on ? 0x02 : 0x01)};
    this->sendCmd_(remote_addr_, 0x01, data);
    this->sendCMDGetDataExtended();
}

void IQ2020Component::sendCmdSetSpaLock(bool on)
{
    ESP_LOGI(TAG, "sendCmdSetSpaLock %s", on ? "Locked" : "Unlocked");
    std::vector<uint8_t> data = {0x0b, 0x1d, (uint8_t)(on ? 0x02 : 0x01)};
    this->sendCmd_(remote_addr_, 0x01, data);
    this->sendCMDGetDataExtended();
}

void IQ2020Component::sendCmdSetTempLock(bool on)
{
    ESP_LOGI(TAG, "sendCmdSetTempLock %s", on ? "Locked" : "Unlocked");
    std::vector<uint8_t> data = {0x0b, 0x1e, (uint8_t)(on ? 0x02 : 0x01)};
    this->sendCmd_(remote_addr_, 0x01, data);
    this->sendCMDGetDataExtended();
}

// 0x02/0x41 reads and writes the two filter cycle times plus the econ flag.
// The trailing flags byte gates the write: bit 7 commits the times, bit 6
// commits econ from bit 0. Sent all-zero it is a pure read.
void IQ2020Component::sendCMDGetFilterConfig()
{
    std::vector<uint8_t> data = {0x02, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00};
    this->sendCmd_(remote_addr_, 0x01, data);
}

void IQ2020Component::sendCmdSetCleanMode(bool state) {
    std::vector<uint8_t> data = {0x0b, 0x1f, 0x01};
    if(state == true)
    {
        data[2] = 0x02;
    }
    this->sendCmd_(remote_addr_, 0x01, data);
    this->sendCMDGetDataExtended();
}

void IQ2020Component::sendCmdSetDateTime(uint8_t seconds, uint8_t minutes, uint8_t hours, uint8_t days, uint8_t months, uint16_t years) {
    std::vector<uint8_t> data = {0x02, 0x4c, seconds, minutes, hours, days, months, (uint8_t)years, (uint8_t)(years >> 8) };
    this->sendCmd_(remote_addr_, 0x01, data);
    this->sendCMDGetDataExtended();
}

void IQ2020Component::sendCmdSetSWG(float value)
{
    uint8_t iLevel = std::round(value);
    if(iLevel <= 10)
    {
        ESP_LOGI(TAG, "sendCmdSetSWG %d", iLevel);
        std::vector<uint8_t> data = {0x1E, 0x02, 0x01, iLevel, 0x00};
        this->sendCmd_(remote_addr_, 0x01, data);
        this->sendCMDGetDataExtended();
    }
}

// Remote reset. The three payload bytes are a magic constant the controller
// checks before acting - on a match it enters an unconditional infinite loop and
// the watchdog resets it a moment later. Nothing else guards it.
//
// The spa keeps running while the controller reboots, but everything it drives
// stops responding until it comes back, so this is not a routine operation.
void IQ2020Component::sendCmdReset()
{
    ESP_LOGW(TAG, "sendCmdReset - resetting the IQ2020 controller via watchdog");
    std::vector<uint8_t> data = {0x02, 0x73, 0x34, 0x87, 0xe5};
    this->sendCmd_(remote_addr_, 0x01, data);
}

// Transmit nudge. Tells the controller it may send whatever it has queued once
// ~300 ms have passed, instead of waiting for its usual bus-idle condition, and
// resets its retry counter at the same time.
//
// It changes no setting - it is purely bus arbitration, which is what makes it
// usable as a recovery poke when the controller has stopped answering because it
// keeps losing arbitration.
void IQ2020Component::sendCmdTransmitNudge()
{
    ESP_LOGI(TAG, "sendCmdTransmitNudge");
    std::vector<uint8_t> data = {0x02, 0x50, 0x06};
    this->sendCmd_(remote_addr_, 0x01, data, false);
    this->last_nudge_timestamp_ = millis();
}

void IQ2020Component::sendCmdSetTemp(float temp_c) {
    if(last_set_temp_c_ != 0) {
        // we have a temp to compare against
        
        if(this->temp_in_c_) {
            auto temp_diff = temp_c - last_set_temp_c_;
            ESP_LOGI(TAG, "sendCmdSetTemp CurrentC:%f TargetC:%f Diff:%f", last_set_temp_c_, temp_c, temp_diff);
        } else {
            auto temp_f = celsius_to_fahrenheit(temp_c);
            int temp_diff = std::round(temp_f - last_set_temp_f_);
            char temp_offset = (char)temp_diff;
            ESP_LOGI(TAG, "sendCmdSetTemp CurrentF:%f TargetF:%f Diff:%d Offset:%02x", last_set_temp_f_, temp_f, temp_diff, temp_offset);

            if(temp_diff != 0 && temp_diff < 10 && temp_diff > -10)
            {
                std::vector<uint8_t> data = {0x01, 0x09, 0xff, (uint8_t)temp_offset};
                this->sendCmd_(remote_addr_, 0x01, data);
                this->sendCMDGetDataExtended();
            }
        }
        
        
    }
}

void IQ2020Component::sendCmdSetLightColorUp(uint8_t lightNum) {
    ESP_LOGI(TAG, "sendCmdSetLightColorUp Light:%s", this->decodeLightNumber_(lightNum).c_str());
    std::vector<uint8_t> data = {0x17, 0x02, lightNum, 0x05, 0x00};
    this->sendCmd_(remote_addr_, 0x01, 0x01, data);
}

void IQ2020Component::sendCmdSetLightColorDown(uint8_t lightNum) {
    ESP_LOGI(TAG, "sendCmdSetLightColorDown Light:%s", this->decodeLightNumber_(lightNum).c_str());
    std::vector<uint8_t> data = {0x17, 0x02, lightNum, 0x04, 0x00};
    this->sendCmd_(remote_addr_, 0x01, 0x01, data);
}

void IQ2020Component::sendCmdSetLightBrightUp(uint8_t lightNum) {
    ESP_LOGI(TAG, "sendCmdSetLightBrightUp Light:%s", this->decodeLightNumber_(lightNum).c_str());
    std::vector<uint8_t> data = {0x17, 0x02, lightNum, 0x03, 0x00};
    this->sendCmd_(remote_addr_, 0x01, 0x01, data);
}

void IQ2020Component::sendCmdSetLightBrightDown(uint8_t lightNum) {
    ESP_LOGI(TAG, "sendCmdSetLightBrightDown Light:%s", this->decodeLightNumber_(lightNum).c_str());
    std::vector<uint8_t> data = {0x17, 0x02, lightNum, 0x02, 0x00};
    this->sendCmd_(remote_addr_, 0x01, 0x01, data);
}

void IQ2020Component::sendCmdSetLightAllOn() {
    ESP_LOGI(TAG, "sendCmdSetLightAllOn Light:%s", this->decodeLightNumber_(4).c_str());
    std::vector<uint8_t> data = {0x17, 0x02, 0x04, 0x11, 0x00};
    this->sendCmd_(remote_addr_, 0x01, data);
    this->sendCMDGetLightStatus();
}

void IQ2020Component::sendCmdSetLightAllOff() {
    ESP_LOGI(TAG, "sendCmdSetLightAllOff Light:%s", this->decodeLightNumber_(4).c_str());
    std::vector<uint8_t> data = {0x17, 0x02, 0x04, 0x10, 0x00};
    this->sendCmd_(remote_addr_, 0x01, data);
    this->sendCMDGetLightStatus();
}

// Colour-cycle control. Wire command 8 starts the cycle and 9 stops it.
void IQ2020Component::sendCmdSetLightCycleOn(uint8_t lightNum) {
    ESP_LOGI(TAG, "sendCmdSetLightCycleOn Light:%s", this->decodeLightNumber_(lightNum).c_str());
    std::vector<uint8_t> data = {0x17, 0x02, lightNum, 0x08, 0x00};
    this->sendCmd_(remote_addr_, 0x01, data);
}

void IQ2020Component::sendCmdSetLightCycleOff(uint8_t lightNum) {
    ESP_LOGI(TAG, "sendCmdSetLightCycleOff Light:%s", this->decodeLightNumber_(lightNum).c_str());
    std::vector<uint8_t> data = {0x17, 0x02, lightNum, 0x09, 0x00};
    this->sendCmd_(remote_addr_, 0x01, data);
}

// Cycle speed steps one level per command and saturates at 0 and 3 in the
// controller, so repeating past the end is harmless.
void IQ2020Component::sendCmdSetLightSpeedUp(uint8_t lightNum) {
    ESP_LOGI(TAG, "sendCmdSetLightSpeedUp Light:%s", this->decodeLightNumber_(lightNum).c_str());
    std::vector<uint8_t> data = {0x17, 0x02, lightNum, 0x07, 0x00};
    this->sendCmd_(remote_addr_, 0x01, data);
}

void IQ2020Component::sendCmdSetLightSpeedDown(uint8_t lightNum) {
    ESP_LOGI(TAG, "sendCmdSetLightSpeedDown Light:%s", this->decodeLightNumber_(lightNum).c_str());
    std::vector<uint8_t> data = {0x17, 0x02, lightNum, 0x06, 0x00};
    this->sendCmd_(remote_addr_, 0x01, data);
}

void IQ2020Component::set_light_cycle(uint8_t lightNum, bool cycling) {
    if(lightNum == 4) {
        for(int i = 0; i < 4; i++) {
            this->set_light_cycle(i, cycling);
        }
        this->sendCMDGetLightStatus();
        return;
    }
    if(lightNum > 3) {
        return;
    }
    ESP_LOGI(TAG, "set_light_cycle Light:%s Cycling:%d", this->decodeLightNumber_(lightNum).c_str(), cycling);
    if(cycling) {
        this->sendCmdSetLightCycleOn(lightNum);
    } else {
        this->sendCmdSetLightCycleOff(lightNum);
    }
    this->sendCMDGetLightStatus();
}

void IQ2020Component::set_light_speed(uint8_t lightNum, uint8_t speed, bool skip_status_update) {
    if(lightNum == 4) {
        for(int i = 0; i < 4; i++) {
            this->set_light_speed(i, speed, true);
        }
        this->sendCMDGetLightStatus();
        return;
    }
    if(lightNum > 3) {
        return;
    }
    if(speed > 3) {
        ESP_LOGW(TAG, "set_light_speed Light:%s Ignoring out-of-range Speed:%d", this->decodeLightNumber_(lightNum).c_str(), speed);
        return;
    }
    if(light_speed_[lightNum] == 255) {
        ESP_LOGW(TAG, "set_light_speed Light:%s Ignoring CurrentSpeed:Unknown", this->decodeLightNumber_(lightNum).c_str());
        return;
    }
    ESP_LOGI(TAG, "set_light_speed Light:%s CurrentSpeed:%s WantSpeed:%s",
        this->decodeLightNumber_(lightNum).c_str(),
        this->decodeLightSpeed_(light_speed_[lightNum]).c_str(),
        this->decodeLightSpeed_(speed).c_str());
    if(speed > light_speed_[lightNum]) {
        for(int i = 0, steps = speed - light_speed_[lightNum]; i < steps; i++) {
            this->sendCmdSetLightSpeedUp(lightNum);
        }
    } else if(speed < light_speed_[lightNum]) {
        for(int i = 0, steps = light_speed_[lightNum] - speed; i < steps; i++) {
            this->sendCmdSetLightSpeedDown(lightNum);
        }
    }
    if(!skip_status_update) {
        this->sendCMDGetLightStatus();
    }
}

void IQ2020Component::set_light_color(uint8_t lightNum, uint8_t color_num, bool skip_status_update) {
    if(lightNum == 4) {
        ESP_LOGI(TAG, "set_light_color All Color:%s SkipUpdate:%d", this->decodeLightColor_(color_num).c_str(), skip_status_update);
        for(int i = 0; i < 4; i++) {
            this->set_light_color(i, color_num, true);
        }
        this->sendCMDGetLightStatus();
    } else if(lightNum <= 3) {
        if(light_color_[lightNum] == 255) {
            ESP_LOGW(TAG, "set_light_color Light:%s Ignoring CurrentColor:Unknown WantColor:%s", this->decodeLightNumber_(lightNum).c_str(), this->decodeLightColor_(color_num).c_str());
            return;
        }
        // The controller cycles colours over 1..7 and wraps 7 -> 1 / 1 -> 7.
        // Colour 0 is not reachable, so a request for it would step down forever
        // and never converge. Clamp into the real range; within 1..7 the linear
        // stepping below always arrives.
        if(color_num < 1 || color_num > 7) {
            ESP_LOGW(TAG, "set_light_color Light:%s Clamping out-of-range Color:%d into 1..7", this->decodeLightNumber_(lightNum).c_str(), color_num);
            color_num = (color_num < 1) ? 1 : 7;
        }
        ESP_LOGI(TAG, "set_light_color Light:%s CurrentColor:%s WantColor:%s SkipUpdate:%d", this->decodeLightNumber_(lightNum).c_str(), this->decodeLightColor_(light_color_[lightNum]).c_str(), this->decodeLightColor_(color_num).c_str(), skip_status_update);
        if(color_num > light_color_[lightNum]) {
            //increment
            auto steps = color_num - light_color_[lightNum];
            ESP_LOGI(TAG, "set_light_color Light:%s Increment:%d", this->decodeLightNumber_(lightNum).c_str(), steps);
            this->stabalize_counter_[lightNum] = 3;
            for(int i = 0; i < steps; i++) {
                this->sendCmdSetLightColorUp(lightNum);
            }
        } else if (color_num < light_color_[lightNum]) {
            //decrement
            auto steps = light_color_[lightNum] - color_num;
            ESP_LOGI(TAG, "set_light_color Light:%s Decrement:%d", this->decodeLightNumber_(lightNum).c_str(), steps);
            this->stabalize_counter_[lightNum] = 3;
            for(int i = 0; i < steps; i++) {
                this->sendCmdSetLightColorDown(lightNum);
            }
        }
        if(!skip_status_update) {
            this->sendCMDGetLightStatus();
        }
    }
    
}
void IQ2020Component::set_light_brightness(uint8_t lightNum, bool On, uint8_t brightness, bool skip_status_update) {
    if(lightNum == 4) {
        ESP_LOGI(TAG, "set_light_brightness All On:%d Brightness:%s SkipUpdate:%d", On, this->decodeLightIntensity_(brightness).c_str(), skip_status_update);
        for(int i = 0; i < 4; i++) {
            this->set_light_brightness(i, On, brightness, true);
        }
        this->sendCMDGetLightStatus();

    } else if(lightNum <= 3) {
        if(light_brightness_[lightNum] == 255) {
            ESP_LOGW(TAG, "set_light_brightness Ignoring Light:%s CurrentBrightness:Unknown WantBrightness:%s", this->decodeLightNumber_(lightNum).c_str(), this->decodeLightIntensity_(brightness).c_str());
            return;
        }
        ESP_LOGI(TAG, "set_light_brightness Light:%s CurrentBrightness:%s WantOn:%d WantBrightness:%s SkipUpdate:%d", this->decodeLightNumber_(lightNum).c_str(), this->decodeLightIntensity_(light_brightness_[lightNum]).c_str(), On, this->decodeLightIntensity_(brightness).c_str(), skip_status_update);
        if(brightness > light_brightness_[lightNum]) {
            //increment
            auto steps = brightness - light_brightness_[lightNum];
            ESP_LOGI(TAG, "set_light_brightness Light:%s Increment:%d", this->decodeLightNumber_(lightNum).c_str(), steps);
            this->stabalize_counter_[lightNum] = 3;
            for(int i = 0; i < steps; i++) {
                this->sendCmdSetLightBrightUp(lightNum);
            }
        } else if (brightness < light_brightness_[lightNum]) {
            //decrement
            auto steps = light_brightness_[lightNum] - brightness;
            ESP_LOGI(TAG, "set_light_brightness Light:%s Decrement:%d", this->decodeLightNumber_(lightNum).c_str(), steps);
            this->stabalize_counter_[lightNum] = 3;
            for(int i = 0; i < steps; i++) {
                this->sendCmdSetLightBrightDown(lightNum);
            }
        }
        if(!skip_status_update) {
            this->sendCMDGetLightStatus();
        }
    }
}


// Direct commands to the salt module, bypassing the controller.
//
// These reproduce the poll the controller itself sends: a 13-byte payload
// prefilled with 0xFF, where 0xFF means "no change". Two fields are NOT
// no-change and must carry live values every time:
//
//   payload[0]  output level  - the module takes this as the level unconditionally
//   payload[1]  spa size
//
// Getting payload[0] wrong silently reprograms the output level as a side effect
// of whatever else the command was for.
std::vector<uint8_t> IQ2020Component::buildSWGCommand_() {
    // Fall back to the module's last reported level rather than inventing one.
    uint8_t level = (this->swg_level_reported_ <= 10) ? this->swg_level_reported_ : 0;
    uint8_t spa_size = (this->swg_spa_size_ != 0xFF) ? this->swg_spa_size_ : 0x01;
    return { 0x1E, 0x01,
             level, spa_size, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
}

uint8_t IQ2020Component::swgAddr_() {
    // 0x29 (FreshWater) unless the module has actually been seen at 0x24 (ACE).
    return this->swg_seen_ ? this->swg_addr_ : 0x29;
}

// payload[4]: 1 starts the 24-hour boost cycle, 2 stops it. Sent once and then
// reverted to 0xFF, because the module acts on the transition - leaving the byte
// set would re-issue the command on every poll.
void IQ2020Component::sendSWGBoostCmd(bool enableBoost) {
    if(!this->swg_seen_) {
        ESP_LOGW(TAG, "sendSWGBoostCmd: no SWG seen on the bus yet, skipping");
        return;
    }
    ESP_LOGI(TAG, "sendSWGBoostCmd %s", enableBoost ? "Start" : "Stop");
    std::vector<uint8_t> data = this->buildSWGCommand_();
    uint8_t addr = this->swgAddr_();
    data[0x06] = enableBoost ? 0x01 : 0x02;   // payload[4]
    this->sendCmd_(remote_addr_, addr, data);
    data[0x06] = 0xff;
    this->sendCmd_(remote_addr_, addr, data);
}

void IQ2020Component::sendSWGDefault() {
    if(!this->swg_seen_) {
        return;
    }
    std::vector<uint8_t> data = this->buildSWGCommand_();
    this->sendCmd_(remote_addr_, this->swgAddr_(), data);
}

// payload[9] = 1 starts the water test - the same action as the panel's test
// button. Also one-shot.
void IQ2020Component::sendSWGTestCmd() {
    if(!this->swg_seen_) {
        ESP_LOGW(TAG, "sendSWGTestCmd: no SWG seen on the bus yet, skipping");
        return;
    }
    ESP_LOGI(TAG, "sendSWGTestCmd start water test");
    std::vector<uint8_t> data = this->buildSWGCommand_();
    uint8_t addr = this->swgAddr_();
    data[0x0B] = 0x01;   // payload[9]
    this->sendCmd_(remote_addr_, addr, data);
    data[0x0B] = 0xff;
    this->sendCmd_(remote_addr_, addr, data);
}

void IQ2020Component::sendResponseEmulateAudio() {
    if(this->simulate_music_) {
        ESP_LOGI(AUDIO_TAG, "Emulating Audio Device");
        //std::vector<uint8_t> data = {0x19, 0x01, 0x01, 0x19, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x04, 0x01, 0x00, 0x00};
        uint8_t* buffer = static_cast<uint8_t*>(static_cast<void*>(&audio_status_));
        auto data = std::vector<uint8_t>(buffer, buffer + 13);
        this->sendCmd_(music_addr_, iq2020_addr_, 0, data, false);
    } else {
        ESP_LOGI(AUDIO_TAG, "Not emulating Audio Device");
    }
}

void IQ2020Component::sendResponseEmulateAudioTitle() {
    ESP_LOGI(AUDIO_TAG, "Emulating Audio Title:%s", audio_title_.c_str());
    std::vector<char> title(audio_title_.begin(), audio_title_.end());
    std::vector<uint8_t> data;
    data.reserve(2 + title.size());
    data.push_back(0x19);
    data.push_back(0x06);
    for(int i = 0; i < title.size(); i++) {
        data.push_back(title[i]);
    }
    this->sendCmd_(music_addr_, iq2020_addr_, 0, data, false);
}
void IQ2020Component::sendResponseEmulateAudioArtist() {
    ESP_LOGI(AUDIO_TAG, "Emulating Audio Artist:%s", audio_artist_.c_str());
    std::vector<char> title(audio_artist_.begin(), audio_artist_.end());
    std::vector<uint8_t> data;
    data.reserve(2 + title.size());
    data.push_back(0x19);
    data.push_back(0x07);
    for(int i = 0; i < title.size(); i++) {
        data.push_back(title[i]);
    }
    this->sendCmd_(music_addr_, iq2020_addr_, 0, data, false);
}

void IQ2020Component::sendCmdGetAudio() {
    ESP_LOGI(AUDIO_TAG, "Send Get Audio Status");
    std::vector<uint8_t> data = {0x19, 0x01 };
    this->sendCmd_(remote_addr_, 0x01, data);
}

void IQ2020Component::sendCmdSetAudioSource(uint8_t source) {
    ESP_LOGI(AUDIO_TAG, "Send Change Audio Source:0x%02X", source);
    std::vector<uint8_t> data = {0x19, 0x00, 0x03, source, 0x00 };
    this->sendCmd_(remote_addr_, 0x01, data);
}

void IQ2020Component::sendCmd_(uint8_t source, uint8_t dest, std::vector<uint8_t> data, bool request) {

    if(startup_delay_passed_) {
        auto size = this->send_queue_.size();
        auto id = random_uint32();
        ESP_LOGI(TAG, "sendCmd_ queue packet QueueSize:%i Dest:%02X Retries:3-default Id:%" PRIu32 "",
            size, dest, id);
        this->send_queue_.push(std::make_tuple(3, 0, source, dest, data, id, request));
    } else {
        ESP_LOGI(TAG, "ignoring sendCmd_ until startup delay passed Dest:%02X", dest);
    }
}

void IQ2020Component::sendCmd_(uint8_t source, uint8_t dest, uint8_t retries, std::vector<uint8_t> data, bool request) {

    auto size = this->send_queue_.size();
    ESP_LOGV(TAG, "sendCmd_ queue packet QueueSize:%i Dest:%02X Retries:%d Id:%" PRIu32 "", size, dest, retries, id);
    auto id = random_uint32();
    this->send_queue_.push(std::make_tuple(retries, 0, source, dest, data, id, request));
}

bool IQ2020Component::readline_(int readch, uint8_t *buffer, int len) {
    // Kept as a member rather than a local static so the transmit path can tell
    // whether a frame is currently in flight.
    int &pos = this->rx_pos_;

    // Inter-frame gap resync. The controller uses the same rule: a long enough
    // silence means whatever was part-received is abandoned. Without this, one
    // corrupted length byte swallows everything that follows it - a capture
    // caught a mangled frame claiming 253 bytes eat four good frames behind it.
    uint32_t now = millis();
    if (pos != 0 && (now - this->last_rx_byte_ms_) > IQ2020_INTERFRAME_GAP_MS) {
        ESP_LOGW(TAG, "readline_ %" PRIu32 " ms gap mid-frame at pos %d - resyncing",
                 now - this->last_rx_byte_ms_, pos);
        this->bus_resyncs_++;
        pos = 0;
    }
    this->last_rx_byte_ms_ = now;

    ESP_LOGV(TAG, "readline_ POS: %02X", pos);

    if (pos == 0 && readch == 0x1c) {
        ESP_LOGV(TAG, "readline_ Good header1");
        buffer[pos] = readch;
        pos++;

    } else if (pos == 0) {
        ESP_LOGW(TAG, "readline_ BAD header1, dropping byte: %02X", readch);

    } else if (pos == 1) {
        buffer[pos] = readch;
        pos++;
        ESP_LOGV(TAG, "readline_ Dest: %02X", readch);

    } else if (pos == 2) {
        buffer[pos] = readch;
        pos++;
        ESP_LOGV(TAG, "readline_ Source: %02X", readch);

    } else if (pos == 3) {
        // The controller rejects len < 2, and anything that will not fit is a
        // corrupted byte rather than a real frame. Catching it here means one
        // bad byte costs one frame instead of the next few hundred bytes.
        if (readch < 2 || readch > len - 7) {
            ESP_LOGW(TAG, "readline_ implausible length %02X - resyncing", readch);
            this->bus_resyncs_++;
            pos = 0;
            return true;
        }
        buffer[pos] = readch;
        pos++;
        ESP_LOGV(TAG, "readline_ Length: %02X", readch);

    } else if (pos == 4) {
        buffer[pos] = readch;
        pos++;
        ESP_LOGV(TAG, "readline_ Operation: %02X", readch);

    } else if (pos >= 5 && pos < len - 1) {
        buffer[pos] = readch;
        ESP_LOGV(TAG, "readline_ Data %02X: %02X", pos - 5, readch);

        if (pos == buffer[3] + 5) {
            
            auto src = buffer[2];
            auto dest = buffer[1];
            auto length = buffer[3];
            auto operation = buffer[4];

            bool request = GETBIT8(operation, 6);
            bool response = GETBIT8(operation, 7);

            std::vector<uint8_t> data;
            data.reserve(len+6);
            for (int i = 5; i < pos; i++) {
                data.push_back(buffer[i]);
            }

            uint8_t checksum = 0;
            
            for (int i = 1; i < pos; i++)
            { 
                checksum += buffer[i]; 
            }
            checksum = checksum ^ 0xff;

            // Capture mode: one lossless line per frame, emitted before the
            // checksum verdict so malformed frames are recorded too - those are
            // otherwise dropped without their payload, which is exactly the
            // traffic worth looking at. Format is deliberately rigid so it can
            // be parsed months later:
            //
            //   IQCAP <millis> <OK|BAD> <whole frame as hex, 0x1C..checksum>
            //
            // Everything else is derivable from the raw bytes, so nothing is
            // pre-interpreted here.
            if(this->capture_switch_ != nullptr && this->capture_switch_->state) {
                std::string cap;
                char cb[4];
                for (int i = 0; i <= pos; i++) {
                    sprintf(cb, "%02X", buffer[i]);
                    cap += cb;
                }
                ESP_LOGI(TAG, "IQCAP %u %s %s", (unsigned) millis(),
                         (buffer[pos] == checksum) ? "OK" : "BAD", cap.c_str());
            }

            if(checksum != buffer[pos])
            {
                // Resync only. Draining the UART here used to discard whatever
                // had already arrived behind the bad frame, turning one
                // corrupted byte into several lost frames; the next 0x1C starts
                // a new frame on its own.
                ESP_LOGE(TAG, "readline_ Invalid Checksum %02X should be %02X - resyncing", buffer[pos], checksum);
                this->bus_resyncs_++;
                pos = 0;
                return true;
            }

            std::string res;
            char buf[64];
            sprintf(buf, "readline_ Full Packet 0x%02X -> 0x%02X Length:%d Operation:0x%02X ", 
                src,
                dest,
                length,
                operation
            );
            res += buf;

            if(request && !response)
            {
                res += "REQ  ";
            } else if (response && !request)
            {
                res += "RESP ";
            } else {
                res += "UNK  ";
            }
            
            sprintf(buf, "Data:");
            res += buf;

            for (int i = 5; i < pos; i++) {
                if (i > 5) {
                    res += ':';
                }
                sprintf(buf, "%02X", buffer[i]);
                res += buf;
            }

            sprintf(buf, " Checksum:%s %02X%s%02X", 
                buffer[pos] == checksum ? "Valid" : "Bad",
                buffer[pos],
                buffer[pos] == checksum ? "=" : "!=",
                checksum
            );
            res += buf;

            ESP_LOGD(TAG, "%s", res.c_str());

            res = "";
            std::tuple<uint8_t,uint8_t> mapKey = std::make_tuple(src,dest);
            if(src == 0x01 && dest == 0x1f)
            {
                mapKey = std::make_tuple(data[0],data[1]);
            }
            if(last_type_recv_timestamp_.find(mapKey) != last_type_recv_timestamp_.end())
            {
                auto mapValue = last_type_recv_timestamp_.at(mapKey);
                auto secondsSince = (millis() - std::get<0>(mapValue)) / 1000.0;
                auto lastData = std::get<1>(mapValue);
                sprintf(buf, "   SinceLast:%02.1fs", 
                    secondsSince
                );
                res += buf;

                if(data.size() == lastData.size())
                {
                    for(int i = 0; i < data.size(); i++)
                    {
                        if(data[i] != lastData[i] && i != 43 && i != 44 && i != 45 && i != 46 && i != 126 && i != 127 && i != 128 && i != 129 && i != 130 && i != 131 && i != 132 && i != 73
                            && i != 35 && i != 36 && i != 37 && i != 38)
                        {
                            sprintf(buf, " [%02d]0x%02X->0x%02X", 
                                i,
                                lastData[i],
                                data[i]
                            );
                            res += buf;
                        }
                    }
                }
                ESP_LOGD(TAG, "%s", res.c_str());
            }
            
            std::tuple<uint32_t,std::vector<uint8_t>> newMapValue = std::make_tuple(millis(), data);
            last_type_recv_timestamp_[mapKey] = newMapValue;

            res = "";
            sprintf(buf, "   DataS:");
            res += buf;
            for (int i = 5; i < pos; i++) {
                
                if(buffer[i] >= 32 && buffer[i] <= 126 )
                {
                    sprintf(buf, "%c", buffer[i]);
                    res += buf;
                } else 
                {
                    res += ".";
                }
                
            }
            ESP_LOGD(TAG, "%s", res.c_str());
            this->last_recv_timestamp_ = millis();

            std::vector<uint8_t> origData;
            if(dest == remote_addr_) {
                if (!this->send_queue_.empty())
                {
                    auto packet = this->send_queue_.front();
                    auto retries = std::get<0>(packet);
                    auto attempts = std::get<1>(packet);
                    auto queueSrc = std::get<2>(packet);
                    auto queueDest = std::get<3>(packet);
                    auto queueData = std::get<4>(packet);
                    if(src == 0x01 && src == queueDest && data[0] == queueData[0] && data[1] == queueData[1]) {
                        ESP_LOGD(TAG, "send_queue_ RECV Got CMD response, removing from send queue Retries:%i Attempts:%i Data[0]=0x%02X Data[1]=0x%02X", retries, attempts, data[0], data[1]);
                        this->send_queue_.pop();
                        origData = queueData;    
                    }
                    else if(src == queueDest && src != 0x01)
                    {
                        ESP_LOGD(TAG, "send_queue_ RECV Got Generic response, removing from send queue Retries:%i Attempts:%i Src:0x%02X", retries, attempts, src);
                        this->send_queue_.pop();
                    } else {
                        ESP_LOGW(TAG, "send_queue_ RECV wrong packet");
                    }
                }
            }

            

            // Get Timers
            if(src == 0x01 && dest == 0x1f && operation == 0x80 && length == 0x0A && data[0] == 0x02 && data[1] == 0x4c)
            {

            }

            // A well-formed 02/55 is 117 data bytes and a 02/56 is 134.
            // Anything shorter passed the checksum but is not a status block,
            // and the decode below indexes fixed offsets that would run off the
            // end of it.
            else if(src == 0x01 && dest == 0x1f && operation == 0x80 && data[0] == 0x02
                    && (data[1] == 0x55 || data[1] == 0x56) && data.size() < 117)
            {
                ESP_LOGW(TAG, "Status block too short (%u bytes); ignoring",
                         (unsigned) data.size());
            }

            // Get Data
            else if(src == 0x01 && dest == 0x1f && operation == 0x80 && data[0] == 0x02 && (data[1] == 0x55 || data[1] == 0x56))
            {
//Clean Off
//[11:16:37][I][iq2020.component:697]: DataExtended Length:134 Offset:00:01:02:03:04:05:06:07:08:09:0A:0B:0C:0D:0E:0F:10:11:12:13:14:15:16:17:18:19:1A:1B:1C:1D:1E:1F:20:21:22:23:24:25:26:27:28:29:2A:2B:2C:2D:2E:2F:30:31:32:33:34:35:36:37:38:39:3A:3B:3C:3D:3E:3F:40:41:42:43:44:45:46:47:48:49:4A:4B:4C:4D:4E:4F:50:51:52:53:54:55:56:57:58:59:5A:5B:5C:5D:5E:5F:60:61:62:63:64:65:66:67:68:69:6A:6B:6C:6D:6E:6F:70:71:72:73:74:75:76:77:78:79:7A:7B:7C:7D:7E:7F:80:81:82:83:84:85
//[11:16:37][I][iq2020.component:698]: DataExtended Length:134 Data:  02:56:00:08:40:04:00:00:06:06:00:0A:06:20:72:13:00:20:1C:20:1C:20:1C:84:03:60:54:00:00:00:00:31:30:32:46:AD:86:09:00:94:B6:06:00:00:C9:08:03:1F:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:E8:A8:08:00:02:00:A1:C7:08:03:00:00:00:00:00:00:00:00:31:30:30:46:31:30:32:46:76:00:01:01:01:01:00:00:00:00:00:00:00:00:00:00:00:00:38:00:00:00:00:00:00:3C:00:1E:00:00:6E:62:01:25:11:0B:03:07:E8:07:01

//Clean On
//[11:18:02][I][iq2020.component:697]: DataExtended Length:134 Offset:00:01:02:03:04:05:06:07:08:09:0A:0B:0C:0D:0E:0F:10:11:12:13:14:15:16:17:18:19:1A:1B:1C:1D:1E:1F:20:21:22:23:24:25:26:27:28:29:2A:2B:2C:2D:2E:2F:30:31:32:33:34:35:36:37:38:39:3A:3B:3C:3D:3E:3F:40:41:42:43:44:45:46:47:48:49:4A:4B:4C:4D:4E:4F:50:51:52:53:54:55:56:57:58:59:5A:5B:5C:5D:5E:5F:60:61:62:63:64:65:66:67:68:69:6A:6B:6C:6D:6E:6F:70:71:72:73:74:75:76:77:78:79:7A:7B:7C:7D:7E:7F:80:81:82:83:84:85
//[11:18:03][I][iq2020.component:698]: DataExtended Length:134 Data:  02:56:00:08:54:04:00:00:06:06:00:0A:06:20:72:13:00:20:1C:20:1C:20:1C:84:03:60:54:02:00:00:00:31:30:32:46:AD:86:09:00:AF:B6:06:00:55:C9:08:03:1F:00:00:00:00:00:00:01:00:00:00:00:00:00:00:00:00:00:00:00:E8:A8:08:00:02:00:F6:C7:08:03:00:00:00:00:00:00:00:00:31:30:30:46:31:30:32:46:75:00:FE:00:FE:00:00:00:09:00:00:00:00:00:00:00:F8:08:33:00:00:00:00:00:00:3C:00:1E:00:00:6E:38:01:02:13:0B:03:07:E8:07:01

//Clean back off
//[11:48:57][I][iq2020.component:697]: DataExtended Length:134 Offset:00:01:02:03:04:05:06:07:08:09:0A:0B:0C:0D:0E:0F:10:11:12:13:14:15:16:17:18:19:1A:1B:1C:1D:1E:1F:20:21:22:23:24:25:26:27:28:29:2A:2B:2C:2D:2E:2F:30:31:32:33:34:35:36:37:38:39:3A:3B:3C:3D:3E:3F:40:41:42:43:44:45:46:47:48:49:4A:4B:4C:4D:4E:4F:50:51:52:53:54:55:56:57:58:59:5A:5B:5C:5D:5E:5F:60:61:62:63:64:65:66:67:68:69:6A:6B:6C:6D:6E:6F:70:71:72:73:74:75:76:77:78:79:7A:7B:7C:7D:7E:7F:80:81:82:83:84:85
//[11:48:57][I][iq2020.component:698]: DataExtended Length:134 Data:  02:56:00:08:40:04:00:00:06:06:00:02:06:20:72:13:00:20:1C:20:1C:20:1C:84:03:60:54:00:00:00:00:31:30:33:46:AD:86:09:00:EB:B8:06:00:94:D0:08:03:1F:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:E8:A8:08:00:02:00:35:CF:08:03:00:00:00:00:00:00:00:00:31:30:30:46:31:30:33:46:74:00:FD:00:FD:00:00:00:00:00:00:00:00:00:00:00:00:00:38:00:00:00:00:00:00:3C:00:1E:00:00:71:38:01:39:31:0B:03:07:E8:07:01
                
                    std::vector<uint8_t> data_num;
                    for(int i = 0; i<data.size(); i++) {
                        data_num.push_back(i);
                    }
                    ESP_LOGI(TAG, "DataExtended Length:%d Offset:%s", data.size(), buffer_to_string_(data_num).c_str());
                    ESP_LOGI(TAG, "DataExtended Length:%d Data:  %s", data.size(), buffer_to_string_(data).c_str());

                if(this->jets1_timeout_sensor_ != nullptr) {
                    this->jets1_timeout_sensor_->publish_state(read_uint16(data, 17));
                }
                if(this->jets2_timeout_sensor_ != nullptr) {
                    this->jets2_timeout_sensor_->publish_state(read_uint16(data, 19));
                }
                if(this->jets3_timeout_sensor_ != nullptr) {
                    this->jets3_timeout_sensor_->publish_state(read_uint16(data, 21));
                }
                if(this->blower_timeout_sensor_ != nullptr) {
                    this->blower_timeout_sensor_->publish_state(read_uint16(data, 23));
                }
                if(this->lights_timeout_sensor_ != nullptr) {
                    this->lights_timeout_sensor_->publish_state(read_uint16(data, 25));
                }



                float heaterSeconds = read_uint32(data, 35);
                float jet1Seconds = read_uint32(data, 39);
                
                float timeInServiceSeconds = read_uint32(data, 43);
                float lostLineCounter = read_uint32(data, 47);

                float jet2Seconds = read_uint32(data, 55);
                float jet3Seconds = read_uint32(data, 59);

                float blowerSeconds = read_uint32(data, 63);
                float lightsSeconds = read_uint32(data, 67);

                // These two used to read offsets 71 and 72, which carry a
                // constant 2 and a zero pad - they reported "2" and "0" forever.
                // Offset 11 is the circulation-pump scheduler's state code, and
                // the light intensity lives in bits 4-6 of offset 5.
                if(this->spa_state_text_sensor_ != nullptr) {
                    this->spa_state_text_sensor_->publish_state(std::to_string(data[11]));
                }
                if(this->light_state_text_sensor_ != nullptr) {
                    this->light_state_text_sensor_->publish_state(std::to_string((data[5] >> 4) & 0x07));
                }


                float circPumpSeconds = read_uint32(data, 73);
                
                float jet1LowSeconds = read_uint32(data, 77);
                float jet2LowSeconds = read_uint32(data, 81);

                

                if (this->heater_seconds_sensor_ != nullptr) {
                    this->heater_seconds_sensor_->publish_state(heaterSeconds);
                }
                if (this->jet1_seconds_sensor_ != nullptr) {
                    this->jet1_seconds_sensor_->publish_state(jet1Seconds);
                }
                if (this->jet2_seconds_sensor_ != nullptr) {
                    this->jet2_seconds_sensor_->publish_state(jet2Seconds);
                }
                if (this->jet3_seconds_sensor_ != nullptr) {
                    this->jet3_seconds_sensor_->publish_state(jet3Seconds);
                }
                if (this->blower_seconds_sensor_ != nullptr) {
                    this->blower_seconds_sensor_->publish_state(blowerSeconds);
                }
                if (this->lights_seconds_sensor_ != nullptr) {
                    this->lights_seconds_sensor_->publish_state(lightsSeconds);
                }
                if (this->pump_seconds_sensor_ != nullptr) {
                    this->pump_seconds_sensor_->publish_state(circPumpSeconds);
                }
                if (this->jet1_low_seconds_sensor_ != nullptr) {
                    this->jet1_low_seconds_sensor_->publish_state(jet1LowSeconds);
                }
                if (this->jet2_low_seconds_sensor_ != nullptr) {
                    this->jet2_low_seconds_sensor_->publish_state(jet2LowSeconds);
                }
                if (this->lifetime_seconds_sensor_ != nullptr) {
                    this->lifetime_seconds_sensor_->publish_state(timeInServiceSeconds);
                }
                if (this->lost_lines_sensor_ != nullptr) {
                    this->lost_lines_sensor_->publish_state(lostLineCounter);
                }
                
                
                auto jets1speed = data[27];
                auto jets2speed = data[28];
                auto jets3speed = data[29];
                auto blowerSpeed = data[30];

                if(this->jets1_speed_sensor_ != nullptr) {
                    this->jets1_speed_sensor_->publish_state(jets1speed);
                }
                if(this->jets2_speed_sensor_ != nullptr) {
                    this->jets2_speed_sensor_->publish_state(jets2speed);
                }
                if(this->jets3_speed_sensor_ != nullptr) {
                    this->jets3_speed_sensor_->publish_state(jets3speed);
                }
                if(this->blower_speed_sensor_ != nullptr) {
                    this->blower_speed_sensor_->publish_state(blowerSpeed);
                }

                ESP_LOGI(TAG, "Jets Speed 1:%d 2:%d 3:%d", jets1speed, jets2speed, jets3speed);
                auto jets1on = jets1speed == 0x03 || jets1speed == 0x02 || jets1speed == 0x01;
                auto jets2on = jets2speed == 0x03 || jets2speed == 0x02 || jets2speed == 0x01;
                auto jets3on = jets3speed == 0x03 || jets3speed == 0x02 || jets3speed == 0x01;
                ESP_LOGI(TAG, "Jets On 1:%d 2:%d 3:%d",
                    jets1on,
                    jets2on,
                    jets3on
                );

                if (this->jets1_switch_ != nullptr) {
                    ESP_LOGI(TAG, "jets1_switch_ publish_state:%s", jets1on ? "True" : "False");
                    this->jets1_switch_->publish_state(jets1on);
                }

                bool summer_timer = data[51] != 0x00;
                bool spa_lock = data[52] != 0x00;
                auto temp_lock = data[53] != 0x00;
                bool clean_lock = data[54] != 0x00;

                if(this->summer_timer_binary_sensor_ != nullptr) {
                    this->summer_timer_binary_sensor_->publish_state(summer_timer);
                }
                if(this->spa_lock_binary_sensor_ != nullptr) {
                    this->summer_timer_binary_sensor_->publish_state(spa_lock);
                }
                if(this->temp_lock_binary_sensor_ != nullptr) {
                    this->temp_lock_binary_sensor_->publish_state(temp_lock);
                }
                if(this->clean_lock_binary_sensor_ != nullptr) {
                    this->clean_lock_binary_sensor_->publish_state(clean_lock);
                }

                ESP_LOGI(TAG, "SpaLock:%s TempLock:%s", spa_lock ? "True" : "False", temp_lock ? "True" : "False");

                // Electrical channels. Each named channel is one voltage, one
                // current and the power the controller derives from that exact
                // pair - the packet does not list the three in the same order,
                // so the offsets below are deliberately not sequential. See
                // docs/status-packet-0255.md.
                //
                //   baseline    : the always-on load, latched whenever no jets
                //                 and no blower are running - the circ pump
                //   jets_blower : whatever is drawn above that baseline
                //   heater      : the heating element. Identified from a week of
                //                 capture: its current is 0 or 23 A and the 23 A
                //                 frames are exactly the frames where offset 6
                //                 reads 5, with no exceptions either way. It is
                //                 also the one channel with no power-factor
                //                 term, which is what a resistive element wants.
                //   aux         : a fourth channel, unpopulated here - zero
                //                 across every frame, heating or not
                //
                // The aux channel's three fields are emitted defectively by the
                // controller: it stores the high byte into both byte positions
                // and drops the low byte. Only the first byte carries
                // information, so recover the value at 256-unit resolution.
                // Nothing has ever been seen in it, so that reconstruction is
                // still untested against a real reading.
                auto aux_field = [&](size_t off) -> uint32_t {
                    return (uint32_t) data[off] * 256u;
                };

                if(this->baseline_voltage_sensor_ != nullptr) {
                    this->baseline_voltage_sensor_->publish_state(read_uint16(data, 93));
                }
                if(this->jets_blower_voltage_sensor_ != nullptr) {
                    this->jets_blower_voltage_sensor_->publish_state(read_uint16(data, 95));
                }
                if(this->heater_voltage_sensor_ != nullptr) {
                    this->heater_voltage_sensor_->publish_state(read_uint16(data, 97));
                }
                if(this->aux_voltage_sensor_ != nullptr) {
                    this->aux_voltage_sensor_->publish_state(aux_field(99));
                }

                // Currents are floats truncated to integers by the controller,
                // so anything under 1 A reports as 0 even while its power
                // reading is correct. Do not treat a 0 here as "off".
                if(this->jets_blower_current_sensor_ != nullptr) {
                    this->jets_blower_current_sensor_->publish_state(read_uint16(data, 101));
                }
                if(this->baseline_current_sensor_ != nullptr) {
                    this->baseline_current_sensor_->publish_state(read_uint16(data, 103));
                }
                if(this->heater_current_sensor_ != nullptr) {
                    this->heater_current_sensor_->publish_state(read_uint16(data, 105));
                }
                if(this->aux_current_sensor_ != nullptr) {
                    this->aux_current_sensor_->publish_state(aux_field(107));
                }

                if(this->jets_blower_power_sensor_ != nullptr) {
                    this->jets_blower_power_sensor_->publish_state(read_uint16(data, 109));
                }
                if(this->baseline_power_sensor_ != nullptr) {
                    this->baseline_power_sensor_->publish_state(read_uint16(data, 111));
                }
                if(this->heater_power_sensor_ != nullptr) {
                    this->heater_power_sensor_->publish_state(read_uint16(data, 113));
                }
                if(this->aux_power_sensor_ != nullptr) {
                    this->aux_power_sensor_->publish_state(aux_field(115));
                }

                // Offsets 117 and up exist only in the 02/56 form: a 02/55
                // reply is 117 bytes and stops at offset 116. Check the size
                // rather than the command byte, so a truncated frame that
                // happened to pass the checksum cannot walk off the end either.
                if(data.size() < 134) {
                    ESP_LOGD(TAG, "Status block is %u bytes; skipping the 02/56 tail",
                             (unsigned) data.size());
                } else {
                if(this->daily_clean_cycle_sensor_ != nullptr) {
                    this->daily_clean_cycle_sensor_->publish_state(data[117]);
                }

                if(this->filter1_time_sensor_ != nullptr) {
                    this->filter1_time_sensor_->publish_state(read_uint16(data, 118));
                }
                if(this->filter2_time_sensor_ != nullptr) {
                    this->filter2_time_sensor_->publish_state(read_uint16(data, 120));
                }

                if(this->pcb_temp_sensor_ != nullptr) {
                    this->pcb_temp_sensor_->publish_state(data[123]);
                }

                if(this->periph_current_sensor_ != nullptr) {
                    this->periph_current_sensor_->publish_state(read_uint16(data, 124) / 1000.0);
                }

                if(data[1] == 0x56)
                {
                    auto seconds = data[126];
                    auto minutes = data[127];
                    auto hours = data[128];
                    auto days = data[129];
                    auto months = data[130];
                    uint16_t year = read_uint16(data, 131);

                    if(this->rtc_text_sensor_ != nullptr) {
                        char buf[64];
                        std::sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", year, months, days, hours, minutes, seconds);
                        this->rtc_text_sensor_->publish_state(std::string(buf));
                    }

                    if(this->rtc_seconds_sensor_ != nullptr)
                    {
                        this->rtc_seconds_sensor_->publish_state(seconds);
                    }
                    if(this->rtc_minutes_sensor_ != nullptr)
                    {
                        this->rtc_minutes_sensor_->publish_state(minutes);
                    }
                    if(this->rtc_hours_sensor_ != nullptr)
                    {
                        this->rtc_hours_sensor_->publish_state(hours);
                    }
                    if(this->rtc_days_sensor_ != nullptr)
                    {
                        this->rtc_days_sensor_->publish_state(days);
                    }
                    if(this->rtc_months_sensor_ != nullptr)
                    {
                        this->rtc_months_sensor_->publish_state(months);
                    }
                    if(this->rtc_years_sensor_ != nullptr)
                    {
                        this->rtc_years_sensor_->publish_state(year);
                    }

                    ESP_LOGI(TAG, "Timer %04d-%02d-%02d %02d:%02d:%02d", year, months, days, hours, minutes, seconds);
                }
                }   // end of the 02/56-only tail

                bool pump_on = GETBIT8(data[5], 2);
                
                if (this->pump_binary_sensor_ != nullptr) {
                    this->pump_binary_sensor_->publish_state(pump_on);
                }

                // The heater byte carries 5 when heating and 0 when not - it is
                // not a 0/1 flag.
                if (this->heater_binary_sensor_ != nullptr) {
                    this->heater_binary_sensor_->publish_state(data[6] == 5);
                }
                // Flow switch. Debounced by the controller over 15 seconds, so
                // it lags the pump by up to that long.
                if (this->flow_switch_binary_sensor_ != nullptr) {
                    this->flow_switch_binary_sensor_->publish_state(GETBIT8(data[3], 3));
                }
                // Set when the controller cannot trust its water temperature
                // reading; the temperature strings are meaningless while it is.
                if (this->water_temp_fault_binary_sensor_ != nullptr) {
                    this->water_temp_fault_binary_sensor_->publish_state(GETBIT8(data[3], 6));
                }
                if (this->panel_type_sensor_ != nullptr) {
                    this->panel_type_sensor_->publish_state(data[12]);
                }
                // Offset 8 carries two presence latches. They record what has
                // ever answered on the bus, not what answered most recently, so
                // they stay set once a device has been seen. The CoolZone one
                // also widens the setpoint floor from 80F to 50F.
                if (this->swg_present_binary_sensor_ != nullptr) {
                    this->swg_present_binary_sensor_->publish_state(GETBIT8(data[8], 1));
                }
                if (this->coolzone_present_binary_sensor_ != nullptr) {
                    this->coolzone_present_binary_sensor_->publish_state(GETBIT8(data[8], 6));
                }

                // Offset 4 bit 4 is the clean cycle. Bit 2 - which this used
                // to read - is jets 1 at high speed, so the switch never
                // followed the clean cycle at all.
                auto clean_mode = GETBIT8(data[4], 4);
                ESP_LOGI(TAG, "Clean Mode:%s", clean_mode ? "On" : "Off");
                if (this->clean_mode_switch_ != nullptr) {
                    ESP_LOGI(TAG, "clean_mode_switch_ publish_state:%s", clean_mode ? "True" : "False");
                    this->clean_mode_switch_->publish_state(clean_mode);
                }
                
                bool temp_in_c = GETBIT8(data[15], 6);
                this->temp_in_c_ = temp_in_c;
                ESP_LOGI(TAG, "Temp In C: %s", temp_in_c ? "True" : "false");

                if(!temp_in_c)
                {
                    std::string temp_a(data.begin() + 31, data.begin() + 34);
                    int iTempAF = std::stoi(temp_a);
                    ESP_LOGI(TAG, "TempA:%s %d", temp_a.c_str(), iTempAF);
                    if (this->high_limit_temp_sensor_ != nullptr) {
                        this->high_limit_temp_sensor_->publish_state(iTempAF);
                    }

                    std::string temp_set(data.begin() + 85, data.begin() + 88);
                    int iTempSetF = std::stoi(temp_set);
                    ESP_LOGI(TAG, "Set Temp:%s %d", temp_set.c_str(), iTempSetF);
                    if (this->temp_set_sensor_ != nullptr) {
                        this->temp_set_sensor_->publish_state(iTempSetF);
                    }

                    std::string water_temp(data.begin() + 89, data.begin() + 92);
                    int iWaterTempF = std::stoi(water_temp);
                    ESP_LOGI(TAG, "TempB:%s %d", water_temp.c_str(), iWaterTempF);
                    if (this->water_temp_sensor_ != nullptr) {
                        this->water_temp_sensor_->publish_state(iWaterTempF);
                    }

                    this->last_set_temp_c_ = fahrenheit_to_celsius(iTempSetF);
                    this->last_water_temp_c_ = fahrenheit_to_celsius(iWaterTempF);

                    this->last_set_temp_f_ = iTempSetF;
                    this->last_water_temp_f_ = iWaterTempF;

                    for (auto *device : this->devices_) {
                        device->on_water_temp(fahrenheit_to_celsius(iWaterTempF));
                        device->on_set_temp(fahrenheit_to_celsius(iTempSetF));
                    }
                    
                }
                else
                {
                    // Celsius. The controller formats these with "%4.1f" rather
                    // than "%3iF", so they arrive as "38.5" and parse as a
                    // float. Without this branch nothing here updated at all in
                    // Celsius mode - not the sensors, and not the climate
                    // entity, which never received a temperature.
                    auto parse_c = [&](size_t off) -> float {
                        std::string t(data.begin() + off, data.begin() + off + 4);
                        return std::strtof(t.c_str(), nullptr);
                    };
                    float highLimitC = parse_c(31);
                    float tempSetC   = parse_c(85);
                    float waterTempC = parse_c(89);
                    ESP_LOGI(TAG, "Celsius HighLimit:%.1f Set:%.1f Water:%.1f",
                             highLimitC, tempSetC, waterTempC);

                    if (this->high_limit_temp_sensor_ != nullptr) {
                        this->high_limit_temp_sensor_->publish_state(highLimitC);
                    }
                    if (this->temp_set_sensor_ != nullptr) {
                        this->temp_set_sensor_->publish_state(tempSetC);
                    }
                    if (this->water_temp_sensor_ != nullptr) {
                        this->water_temp_sensor_->publish_state(waterTempC);
                    }

                    this->last_set_temp_c_ = tempSetC;
                    this->last_water_temp_c_ = waterTempC;
                    this->last_set_temp_f_ = celsius_to_fahrenheit(tempSetC);
                    this->last_water_temp_f_ = celsius_to_fahrenheit(waterTempC);

                    for (auto *device : this->devices_) {
                        device->on_water_temp(waterTempC);
                        device->on_set_temp(tempSetC);
                    }
                }

                auto isHeating = data[6] != 0;
                ESP_LOGI(TAG, "Heater:%d", isHeating);
                for (auto *device : this->devices_) {
                    device->on_heating(isHeating);
                }

                

            }

            else if(src == 0x01 && dest == 0x1f && operation == 0x80 && data[0] == 0x17 && data[1] == 0x02)
            {
                
                if (origData.size() > 0) {
                    ESP_LOGI(TAG, "sendCmdSetLight Response:0x%02X Light:%s Operation:%s", data[2], this->decodeLightNumber_(origData[2]).c_str(), this->decodeLightOperation_(origData[3]).c_str());
                } else {
                    ESP_LOGI(TAG, "sendCmdSetLight Response:0x%02X", data[2]);
                }
            }

            else if(src == 0x01 && dest == 0x1f && operation == 0x80 && data[0] == 0x0b && data[1] == 0x27)
            {
                //sendCmdSetLight
                ESP_LOGI(TAG, "Unimplemented Command Response:0x%02X", data[2]);
            }

            // Get Lights
            else if(src == 0x01 && dest == 0x1f && operation == 0x80 && length == 0x16 && data[0] == 0x17 && data[1] == 0x05)
            {
                ESP_LOGI(TAG, "sendCMDGetLightStatus Response");

                // 17/05 response, 20-byte payload:
                //
                //   payload[0]      master lights on/off
                //   payload[1..4]   intensity,   lights 0..3   (data[3..6])
                //   payload[5..8]   per-light on/off flag      (data[7..10])
                //   payload[9..12]  colour-cycle speed, 0..3   (data[11..14])
                //   payload[13..16] colour,      lights 0..3   (data[15..18])
                //   payload[17]     any light at non-zero intensity (data[19])
                //   payload[18,19]  always zero
                //
                // payload[5..8] is derived from the cycle speed being non-zero,
                // so those bytes report "colour cycling", not "lit".
                bool lights_on = data[19] != 0;
                bool master_on = data[2] != 0;

                ESP_LOGI(TAG, "sendCMDGetLightStatus Master:%d AnyOn:%d", master_on, lights_on);

                for(int i = 0; i < 4; i++) {
                    
                    auto new_brightness = data[03 + i];
                    auto new_color = data[15 + i];
                    auto new_cycling = data[7 + i];
                    auto new_speed = data[11 + i];

                    if(new_speed != light_speed_[i] || new_cycling != light_cycling_[i]) {
                        ESP_LOGI(TAG, "sendCMDGetLightStatus Response Cycle Change Light:%s Cycling:%d -> %d Speed:%s -> %s",
                            this->decodeLightNumber_(i).c_str(),
                            light_cycling_[i], new_cycling,
                            this->decodeLightSpeed_(light_speed_[i]).c_str(),
                            this->decodeLightSpeed_(new_speed).c_str()
                        );
                        light_speed_[i] = new_speed;
                        light_cycling_[i] = new_cycling;
                        for (auto *device : this->devices_) {
                            device->on_light_cycle(i, new_cycling != 0, new_speed);
                        }
                    }

                    
                    if(stabalize_counter_[i] > 0 && false) {
                        ESP_LOGI(TAG, "sendCMDGetLightStatus Response Waiting To Stabalize Light:%s StabalizeCounter:%d Color: %s -> %s Brightness: %s -> %s",
                            this->decodeLightNumber_(i).c_str(),
                            stabalize_counter_[i],
                            this->decodeLightColor_(light_color_[i]).c_str(),
                            this->decodeLightColor_(new_color).c_str(),
                            this->decodeLightIntensity_(light_brightness_[i]).c_str(),
                            this->decodeLightIntensity_(new_brightness).c_str()
                        );
                        stabalize_counter_[i]--;
                    }
                    else if(new_brightness != light_brightness_[i] || new_color != light_color_[i]) {
                        ESP_LOGI(TAG, "sendCMDGetLightStatus Response Change Light:%s StabalizeCounter:%d Color: %s -> %s Brightness: %s -> %s",
                            this->decodeLightNumber_(i).c_str(),
                            stabalize_counter_[i],
                            this->decodeLightColor_(light_color_[i]).c_str(),
                            this->decodeLightColor_(new_color).c_str(),
                            this->decodeLightIntensity_(light_brightness_[i]).c_str(),
                            this->decodeLightIntensity_(new_brightness).c_str()
                        );

                        light_brightness_[i] = new_brightness;
                        light_color_[i] =      new_color;
                       
                        for (auto *device : this->devices_) {
                            device->on_light(i, new_brightness, new_color);
                        }

                    } else {
                        ESP_LOGI(TAG, "sendCMDGetLightStatus Response No Change Light:%s StabalizeCounter:%d Color:%s Brightness:%s",
                            this->decodeLightNumber_(i).c_str(),
                            stabalize_counter_[i],
                            this->decodeLightColor_(light_color_[i]).c_str(),
                            this->decodeLightIntensity_(light_brightness_[i]).c_str()
                        );
                    }

                    
                }

                auto max_brightness = *std::max_element(light_brightness_.begin(), light_brightness_.end());
                if(lights_max_brightness_ != max_brightness) {
                    for (auto *device : this->devices_) {
                        device->on_light(4, max_brightness, 0);
                    }
                    lights_max_brightness_ = max_brightness;
                }

            }

            // Get SWG
            else if(src == 0x01 && dest == 0x1f && operation == 0x80 && data.size() >= 20 && data[0] == 0x1e && data[1] == 0x03)
            {

                ESP_LOGI(TAG, "SWG Packet Src:%s Dest:%s Operation:0x%02X Data:%s Length:%d", 
                    decodeAddr_(src).c_str(), 
                    decodeAddr_(dest).c_str(), 
                    operation, buffer_to_string_(data).c_str(), data.size());
                
                // 1E/03 is the controller's own summary of the SWG. It has
                // THREE layouts, selected by the address the SWG module transmits
                // from (0x24 -> B, 0x29 -> C). The variant is not carried in the
                // frame, but it is knowable: watch which address the module uses.
                //
                // Unused offsets stay at their 0xFF fill. Do not trust them.
                const uint8_t *q = &data[2];
                bool have_variant = this->swg_seen_;
                bool variant_c = have_variant && this->swg_addr_ == 0x29;

                uint8_t swgLevel = q[0];
                if (this->swg_level_number_ != nullptr) {
                    this->swg_level_number_->publish_state(swgLevel);
                }

                // Spa size is only written in the A and C layouts.
                if (q[1] != 0xFF) {
                    this->swg_spa_size_ = q[1];
                }
                if (this->swg_spa_size_sensor_ != nullptr && q[1] != 0xFF) {
                    this->swg_spa_size_sensor_->publish_state(q[1]);
                }

                // Offset 10 packs two fields: salinity index in the high 6 bits,
                // status class in the low 2. Reading it as a scalar is wrong.
                uint8_t rel_salinity = q[10] >> 2;
                uint8_t rel_class    = q[10] & 0x03;

                // Offsets 14-16 are one 24-bit little-endian value; 17 is separate.
                uint32_t rel_runtime = (uint32_t) q[14] | ((uint32_t) q[15] << 8) | ((uint32_t) q[16] << 16);

                ESP_LOGI(TAG, "SWG 1E/03 variant:%s Level:%d SpaSize:%d Salinity idx:%d Class:%d Flags:0x%02X Err:%d Runtime:%u",
                    have_variant ? (variant_c ? "C (FreshWater)" : "B (ACE)") : "unknown",
                    swgLevel, q[1], rel_salinity, rel_class, q[4], q[6] & 7, (unsigned) rel_runtime);

                // Only fall back to this frame's SWG fields when the module's own
                // 1E/01 traffic has not been seen - that frame is richer and its
                // layout is unambiguous.
                if (!this->swg_seen_) {
                    if (this->swg_salinity_index_sensor_ != nullptr) {
                        this->swg_salinity_index_sensor_->publish_state(rel_salinity);
                    }
                    if (this->swg_salinity_sensor_ != nullptr) {
                        this->swg_salinity_sensor_->publish_state(swg_salinity_from_index(rel_salinity));
                    }
                    if (this->swg_output_level_sensor_ != nullptr) {
                        this->swg_output_level_sensor_->publish_state(swgLevel);
                    }
                    if (this->swg_error_sensor_ != nullptr) {
                        this->swg_error_sensor_->publish_state(q[6] & 7);
                    }
                    if (this->swg_cell_runtime_sensor_ != nullptr) {
                        this->swg_cell_runtime_sensor_->publish_state(rel_runtime);
                    }
                    if (this->swg_generating_binary_sensor_ != nullptr) {
                        this->swg_generating_binary_sensor_->publish_state((q[4] & 0x01) != 0);
                    }
                    if (this->swg_boost_binary_sensor_ != nullptr) {
                        this->swg_boost_binary_sensor_->publish_state((q[4] & 0x04) != 0);
                    }
                    // Cartridge age is only present in the C layout.
                    if (variant_c && this->swg_age_sensor_ != nullptr && q[12] != 0xFF) {
                        this->swg_age_sensor_->publish_state(q[12]);
                    }
                }
            }

            // 0x0B control group replies. Every one is a single byte holding the
            // state the controller settled on, which can differ from what was
            // requested - a single-speed pump promotes speed 1 to 2, and an
            // unconfigured jet or blower answers 0 regardless.
            else if(src == 0x01 && dest == 0x1f && operation == 0x80 && data.size() >= 3 && data[0] == 0x0b &&
                    (data[1] == 0x02 || data[1] == 0x03 || data[1] == 0x04))
            {
                uint8_t jet = data[1] - 0x01;   // 0x02..0x04 -> 1..3
                uint8_t speed = data[2];
                ESP_LOGI(TAG, "Jets %d Speed:%d", jet, speed);
                switch_::Switch *sw = (jet == 1) ? this->jets1_switch_
                                    : (jet == 2) ? this->jets2_switch_
                                                 : this->jets3_switch_;
                if(sw != nullptr) {
                    sw->publish_state(speed != 0);
                }
            }

            else if(src == 0x01 && dest == 0x1f && operation == 0x80 && data.size() >= 3 && data[0] == 0x0b && data[1] == 0x07)
            {
                ESP_LOGI(TAG, "Blower Speed:%d", data[2]);
                if(this->blower_switch_ != nullptr) {
                    this->blower_switch_->publish_state(data[2] != 0);
                }
            }

            else if(src == 0x01 && dest == 0x1f && operation == 0x80 && data.size() >= 3 && data[0] == 0x0b &&
                    (data[1] == 0x1c || data[1] == 0x1d || data[1] == 0x1e))
            {
                bool on = data[2] != 0;
                switch_::Switch *sw = (data[1] == 0x1c) ? this->summer_timer_switch_
                                    : (data[1] == 0x1d) ? this->spa_lock_switch_
                                                        : this->temp_lock_switch_;
                ESP_LOGI(TAG, "0B/%02X State:%d", data[1], on);
                if(sw != nullptr) {
                    sw->publish_state(on);
                }
            }

            // 0x02/0x41 - filter cycle times and the econ / circulation flags.
            // The two times also appear in the 02/55 status block; this reply is
            // the only place the econ and circulation bits are exposed.
            else if(src == 0x01 && dest == 0x1f && operation == 0x80 && data.size() >= 7 && data[0] == 0x02 && data[1] == 0x41)
            {
                uint16_t filter1 = read_uint16(data, 2);
                uint16_t filter2 = read_uint16(data, 4);
                bool econ = (data[6] & 0x01) != 0;
                bool circ = (data[6] & 0x02) != 0;
                ESP_LOGI(TAG, "Filter Config Filter1:%d Filter2:%d Econ:%d Circ:%d", filter1, filter2, econ, circ);

                if(this->filter1_time_sensor_ != nullptr) {
                    this->filter1_time_sensor_->publish_state(filter1);
                }
                if(this->filter2_time_sensor_ != nullptr) {
                    this->filter2_time_sensor_->publish_state(filter2);
                }
                if(this->econ_mode_binary_sensor_ != nullptr) {
                    this->econ_mode_binary_sensor_->publish_state(econ);
                }
                if(this->circulation_binary_sensor_ != nullptr) {
                    this->circulation_binary_sensor_->publish_state(circ);
                }
            }

            // Get Versions
            else if(src == 0x01 && dest == 0x1f && operation == 0x80 && length == 0x17 && data[0] == 0x01 && data[1] == 0x00)
            {
                std::string version_controller(data.begin() + 2, data.begin() + 8);
                std::string version_other_a(data.begin() + 8, data.begin() + 12);
                std::string version_other_b(data.begin() + 12, data.begin() + 16);
                std::string version_display(data.begin() + 16, data.begin() + 22);

                if (this->iq2020_version_controller_text_sensor_ != nullptr) {
                    this->iq2020_version_controller_text_sensor_->publish_state(version_controller);
                }

                if (this->iq2020_version_other_a_text_sensor_ != nullptr) {
                    this->iq2020_version_other_a_text_sensor_->publish_state(version_other_a);
                }

                if (this->iq2020_version_other_b_text_sensor_ != nullptr) {
                    this->iq2020_version_other_b_text_sensor_->publish_state(version_other_b);
                }

                if (this->iq2020_version_display_text_sensor_ != nullptr) {
                    this->iq2020_version_display_text_sensor_->publish_state(version_display);
                }

            }

            else if(src == 0x01 && dest == 0x1f && operation == 0x80 && length == 0x13 && data[0] == 0x19 && data[1] == 0x01)
            {
                //19:01:00:19:00:00:00:0B:00:04:01:00:00

                // 2 00       - Power (0 = Off, 1 = On)
                // 3 19       - Volume
                // 4 00       - Treble
                // 5 00       - Bass
                // 6 00       - Balance
                // 7 0B       - Subwoofer volume
                // 8 00       - Play/pause status
                // 9 04       - Source selection (2 = Wireless, 3 = Aux, 4 = Bluetooth)
                //10 01       - Wireless channel
                //11 00       - Radio signal strength
                //12 00       - Bluetooth pairing

            }

            

            else if(src == 0x01 && dest == 0x29)
            {
                ESP_LOGI(TAG, "RAW SWG Packet Src:%s Dest:%s Operation:0x%02X Data:%s Length:%d", 
                    decodeAddr_(src).c_str(), decodeAddr_(dest).c_str(), operation, buffer_to_string_(data).c_str(), data.size());
                if(operation == 0x40 && length == 0x0f)
                {
                    //0x02 = SWG Set 01-10
                    //0x08 = Set 0x01=New 0xFF=Existing
                    auto swgSet = buffer[5 + 0x02];
                    auto newValue = buffer[5 + 0x08] == 0x01;
                    ESP_LOGI(TAG, "SWG Set:%d New:%d", swgSet, newValue);
                }
            }

            else if(src == 0x01 && dest == 0x33 && operation == 0x40 && data[0] == 0x19 && data[1] == 0x00 && length == 0x08) {
                // Get Audio Status
                ESP_LOGI(AUDIO_TAG, "Request Audio Status Sync");


                if(data[2] != audio_status_.power) {
                    ESP_LOGI(AUDIO_TAG, "Power Change 0x%02X -> 0x%02X", audio_status_.power, data[2]);
                    audio_status_.power = data[2];
                }
                
                if(data[3] != audio_status_.volume) {
                    ESP_LOGI(AUDIO_TAG, "Volume Change 0x%02X -> 0x%02X", audio_status_.volume, data[3]);
                    if(data[3] > audio_status_.volume)
                    {
                        this->music_volume_up_callback_();
                    }
                    audio_status_.volume = data[3];
                }
                
                if(data[4] != audio_status_.treble) {
                    ESP_LOGI(AUDIO_TAG, "Treble Change 0x%02X -> 0x%02X", audio_status_.treble, data[4]);
                    audio_status_.treble = data[4];
                }
                
                if(data[5] != audio_status_.bass) {
                    ESP_LOGI(AUDIO_TAG, "Bass Change 0x%02X -> 0x%02X", audio_status_.bass, data[5]);
                    audio_status_.bass = data[5];
                }
                
                if(data[6] != audio_status_.balance) {
                    ESP_LOGI(AUDIO_TAG, "Balance Change 0x%02X -> 0x%02X", audio_status_.balance, data[6]);
                    audio_status_.balance = data[6];
                }
                
                if(data[7] != audio_status_.sub_volume) {
                    ESP_LOGI(AUDIO_TAG, "Sub Volume Change 0x%02X -> 0x%02X", audio_status_.sub_volume, data[7]);
                    audio_status_.sub_volume = data[7];
                }

                /*
                
                         19:00:01:1A:00:00:00:0B
                33 01 40 19:00:01:13:0B:00:08:00
                                  VV TT CC BB SS

                VV = Volume (0x0F to 0x18)
                TT = Tremble (0xFB to 0x05)
                CC = Bass (0xFB to 0x05)
                BB = Balance (0xFB to 0x05)
                SS = Subwoofer (0x00 to 0x0B)
                
                */



                sendResponseEmulateAudio();
            }
            else if(src == 0x01 && dest == 0x33 && operation == 0x40 && data[0] == 0x19 && data[1] == 0x01 && data[2] == 0x00) {
                // Get Audio Status
                ESP_LOGI(AUDIO_TAG, "Request Audio Status");
                sendResponseEmulateAudio();
            }
            else if(src == 0x01 && dest == 0x33 && operation == 0x40 && data[0] == 0x19 && data[1] == 0x01 && data[2] == 0x01) {
                // Audio Play
                ESP_LOGI(AUDIO_TAG, "Request Audio Play");
                audio_status_.play_pause = 0x01;
                sendResponseEmulateAudio();
            }
            else if(src == 0x01 && dest == 0x33 && operation == 0x40 && data[0] == 0x19 && data[1] == 0x01 && data[2] == 0x02) {
                // Audio Pause
                ESP_LOGI(AUDIO_TAG, "Request Audio Pause");
                audio_status_.play_pause = 0x02;
                sendResponseEmulateAudio();
            }
            else if(src == 0x01 && dest == 0x33 && operation == 0x40 && data[0] == 0x19 && data[1] == 0x01 && data[2] == 0x03) {
                // Audio Next
                ESP_LOGI(AUDIO_TAG, "Request Audio Next");
                sendResponseEmulateAudio();
            }
            else if(src == 0x01 && dest == 0x33 && operation == 0x40 && data[0] == 0x19 && data[1] == 0x01 && data[2] == 0x04) {
                // Audio Prev
                ESP_LOGI(AUDIO_TAG, "Request Audio Prev");
                sendResponseEmulateAudio();
            }
            
            else if(src == 0x01 && dest == 0x33 && operation == 0x40 && data[0] == 0x19 && data[1] == 0x03) {
                // Audio Change Source
                ESP_LOGI(AUDIO_TAG, "Request Audio Change Source:0x%02X", data[2]);
                if(data[2] != audio_status_.source) {
                    ESP_LOGI(AUDIO_TAG, "Source Change 0x%02X -> 0x%02X", audio_status_.source, data[2]);
                    audio_status_.source = data[2];
                }
                sendResponseEmulateAudio();
            }

            else if(src == 0x01 && dest == 0x33 && operation == 0x40 && data[0] == 0x19 && data[1] == 0x06 && data[2] == 0x00) {
                // Audio Request Song Name
                ESP_LOGI(AUDIO_TAG, "Request Audio Song Name");
                sendResponseEmulateAudioTitle();
            }
            else if(src == 0x01 && dest == 0x33 && operation == 0x40 && data[0] == 0x19 && data[1] == 0x07 && data[2] == 0x00) {
                // Audio Request Artist Name
                ESP_LOGI(AUDIO_TAG, "Request Audio Artist Name");
                sendResponseEmulateAudioArtist();
            }

            // Salt Water Generator status, straight from the SWG module.
            //
            // The module answers the controller's poll with 1E/01 and a 13-byte
            // payload. The field map below is confirmed against the captures in
            // docs/captures, including every offset that should be unused.
            //
            // The source address picks the layout variant: 0x29 is the FreshWater
            // ("legacy") module and 0x24 the ACE module. This is the same test the
            // controller makes, and it is what resolves which of the three 1E/03
            // layouts the controller will subsequently emit.
            // Deliberately NOT filtered on destination. The module normally
            // answers the controller at 0x01, but it also emits frames addressed
            // to 0x99 - seen clustered around water-test activity in
            // docs/captures, with valid checksums and a well-formed payload. The
            // controller drops those (its destination check only accepts its own
            // address or broadcast), but a sniffer has no reason to: the state
            // they carry is just as good, and filtering them out loses updates.
            else if((src == 0x29 || src == 0x24) && operation == 0x80
                    && data.size() >= 15 && data[0] == 0x1e && data[1] == 0x01) {
                ESP_LOGI(TAG, "RAW SWG Packet Src:%s Dest:%s Operation:0x%02X Data:%s Length:%d", decodeAddr_(src).c_str(), decodeAddr_(dest).c_str(), operation, buffer_to_string_(data).c_str(), data.size());

                const uint8_t *p = &data[2];   // payload[] after the 1E 01 prefix
                bool is_freshwater = (src == 0x29);

                uint8_t level      = p[0];          // SWG output level, 0-10
                uint8_t test_val   = p[1];          // salt-test reading
                uint8_t salinity_i = p[2] >> 2;     // salinity index, 0-63
                uint8_t status_cls = p[2] & 0x03;   // 1 = summer timer, 3 = low salt
                uint8_t cell_days  = p[3];          // cartridge age in days
                uint8_t flags      = p[5];          // b0 generating, b1 active, b2 boost, b3 self-check
                uint8_t error_code = p[6];
                uint32_t runtime   = (uint32_t) p[8] | ((uint32_t) p[9] << 8) | ((uint32_t) p[10] << 16);
                // Low nibble of payload[12] is cartridge presence. The controller's
                // replace wizard waits on it: it steps forward when this reads 0
                // ("Remove Cartridge Now" satisfied) and again when it reads 1
                // ("Insert New Cartridge" satisfied).
                uint8_t cartridge  = p[12] & 0x0F;
                // p[4] is relayed untouched by the controller, but it is NOT
                // static: captures show it moving between 0, 2, 6 and 8, and
                // stepping during a water test. Exposed raw so it can be
                // correlated against the spa; its meaning is not established.
                uint8_t cell_state = p[4];

                // A salinity index of zero forces the "low" class, before
                // anything else looks at it.
                if(salinity_i == 0) {
                    status_cls = 3;
                }

                this->swg_addr_ = src;
                this->swg_level_reported_ = level;
                this->swg_test_value_ = test_val;
                this->swg_status_class_ = status_cls;
                this->swg_salinity_idx_ = salinity_i;
                this->swg_cell_days_ = cell_days;
                this->swg_flags_ = flags;
                this->swg_error_code_ = error_code;
                this->swg_runtime_ = runtime;
                this->swg_seen_ = true;

                bool generating = (flags & 0x01) != 0;
                bool boost      = (flags & 0x04) != 0;
                // bit 3 is set for the duration of the module's self-check and
                // clears when it finishes. It is not a lockout: level adjustment
                // is gated on the salt test reading, not on this bit.
                bool testing    = (flags & 0x08) != 0;
                bool cartridge_due = cell_days >= 120;   // the 4-month replace prompt
                // A salt test reading above 9 locks level adjustment. Worth
                // surfacing on its own - otherwise the level simply stops
                // responding with nothing to explain why.
                bool level_locked = test_val > 9;

                ESP_LOGI(TAG, "SWG %s Level:%d Salinity:%.0f%%(idx %d) Class:%d CellDays:%d Flags:0x%02X Error:%d Runtime:%u",
                    is_freshwater ? "FreshWater" : "ACE",
                    level, swg_salinity_from_index(salinity_i), salinity_i,
                    status_cls, cell_days, flags, error_code, (unsigned) runtime);

                // Reproduce the controller's status logic as far as bus-visible
                // state allows. Its pump/flow checks and the panel's local "user
                // acknowledged" flags are not on the wire, so the no-circulation
                // and post-acknowledge states never appear here.
                uint8_t status;
                bool inactive_flag = is_freshwater && ((flags & 0x02) == 0);
                bool service = (error_code == 1 || error_code == 2 ||
                                error_code == 4 || error_code == 5);
                if(level == 0 || inactive_flag) {
                    status = 1;
                } else if(service) {
                    status = 19;
                } else if(cartridge_due) {
                    status = 18;
                } else if(test_val >= 20) {
                    status = 5;
                } else if(test_val >= 15) {
                    status = 4;
                } else if(test_val >= 10) {
                    // The controller treats 10..14 as its own state and shows no
                    // panel message for it. What matters practically is the side
                    // effect: a reading above 9 locks the output level against
                    // adjustment, so the panel's +/- stops responding.
                    status = 3;
                } else if(!generating) {
                    status = 1;
                } else if(status_cls == 1) {
                    status = 9;
                } else if(status_cls == 3) {
                    status = 12;
                } else if(salinity_i >= 24) {
                    status = boost ? 10 : 11;
                } else {
                    status = boost ? 2 : 0;
                }

                if(this->swg_age_sensor_ != nullptr) {
                    this->swg_age_sensor_->publish_state(cell_days);
                }
                // Historically this published the whole flags byte. Boost is bit 2.
                if(this->swg_boost_mode_sensor_ != nullptr) {
                    this->swg_boost_mode_sensor_->publish_state(boost ? 1 : 0);
                }
                if(this->swg_error_sensor_ != nullptr) {
                    this->swg_error_sensor_->publish_state(error_code);
                }
                if(this->swg_output_level_sensor_ != nullptr) {
                    this->swg_output_level_sensor_->publish_state(level);
                }
                if(this->swg_salinity_sensor_ != nullptr) {
                    this->swg_salinity_sensor_->publish_state(swg_salinity_from_index(salinity_i));
                }
                if(this->swg_salinity_index_sensor_ != nullptr) {
                    this->swg_salinity_index_sensor_->publish_state(salinity_i);
                }
                if(this->swg_cell_runtime_sensor_ != nullptr) {
                    this->swg_cell_runtime_sensor_->publish_state(runtime);
                }
                if(this->swg_generating_binary_sensor_ != nullptr) {
                    this->swg_generating_binary_sensor_->publish_state(generating);
                }
                if(this->swg_boost_binary_sensor_ != nullptr) {
                    this->swg_boost_binary_sensor_->publish_state(boost);
                }
                if(this->swg_boost_switch_ != nullptr) {
                    this->swg_boost_switch_->publish_state(boost);
                }
                if(this->swg_cartridge_due_binary_sensor_ != nullptr) {
                    this->swg_cartridge_due_binary_sensor_->publish_state(cartridge_due);
                }
                if(this->swg_level_locked_binary_sensor_ != nullptr) {
                    this->swg_level_locked_binary_sensor_->publish_state(level_locked);
                }
                if(this->swg_testing_binary_sensor_ != nullptr) {
                    this->swg_testing_binary_sensor_->publish_state(testing);
                }
                if(this->swg_cartridge_present_binary_sensor_ != nullptr) {
                    this->swg_cartridge_present_binary_sensor_->publish_state(cartridge == 1);
                }
                // Salt test reading. Drives the panel's "Test Water & Confirm
                // Level" prompt at 15 and "Level Set To 3" at 20, and locks level
                // adjustment above 9. Cleared when the level changes or the
                // prompt is acknowledged.
                if(this->swg_salt_test_sensor_ != nullptr) {
                    this->swg_salt_test_sensor_->publish_state(test_val);
                }
                if(this->swg_cell_state_sensor_ != nullptr) {
                    this->swg_cell_state_sensor_->publish_state(cell_state);
                }
                if(this->swg_status_text_sensor_ != nullptr) {
                    // While the self-check runs the panel replaces the salt
                    // status with "Testing", but not unconditionally: the two
                    // states that mean the system is not running at all win over
                    // it, and among the prompt/wizard states only "Service
                    // Required" is displaced.
                    bool steady = (status == 0 || status == 2 ||
                                   status == 9 || status == 10 ||
                                   status == 11 || status == 12);
                    bool show_testing = testing && (steady || status == 19);
                    this->swg_status_text_sensor_->publish_state(
                        show_testing ? std::string("Testing")
                                     : this->decodeSWGStatus_(status));
                }
                if(this->swg_type_text_sensor_ != nullptr) {
                    this->swg_type_text_sensor_->publish_state(is_freshwater ? "FreshWater (0x29)" : "ACE (0x24)");
                }
            }

            //else if(src == 0x01 && dest == 0x33) {
            //    ESP_LOGI(TAG, "Unknown Audio Packet Src:%s Dest:%s Operation:0x%02X Data:%s Length:%d", decodeAddr_(src).c_str(), decodeAddr_(dest).c_str(), operation, buffer_to_string_(data).c_str(), data.size());
            //}

            else {
                ESP_LOGI(TAG, "Unknown %s Src:%s Dest:%s Operation:0x%02X Data:%s Length:%d", operation == 0x40 ? "Request" : "Response", decodeAddr_(src).c_str(), decodeAddr_(dest).c_str(), operation, buffer_to_string_(data).c_str(), data.size());
            }

           

            pos=0;
            for(int i = 0; i < len; i++)
            {
                buffer[i] = 0x00;
            }
            return true;

        } else {
            pos++;
        }
    } else {
        ESP_LOGW(TAG, "Clearing Buffer after error");
        pos=0;
        for(int i = 0; i < len; i++)
        {
            buffer[i] = 0x00;
        }
    }
    return false;
}

std::string IQ2020Component::decodeLightColor_(uint8_t raw) {
    // The controller steps colour over 1..7, wrapping 7 -> 1 and 1 -> 7. 0 only
    // ever appears as an uninitialised value, so this table is 1-based.
    //
    // 8 is reachable too, but only from the panel: its palette has eight
    // swatches and the eighth is the colour cycle. Stepping never produces it.
    //
    // "Rainbow" is NOT a colour: the colour cycle is a separate per-light flag
    // driven by wire commands 8/9 and reported in payload[5..8] of 17/05.
    if (raw == 0x00) {
        return "0-Unset";
    } else if (raw == 0x01) {
        return "1-Violet";
    } else if (raw == 0x02) {
        return "2-Blue";
    } else if (raw == 0x03) {
        return "3-Cyan";
    } else if (raw == 0x04) {
        return "4-Green";
    } else if (raw == 0x05) {
        return "5-White";
    } else if (raw == 0x06) {
        return "6-Yellow";
    } else if (raw == 0x07) {
        return "7-Red";
    } else if (raw == 0x08) {
        // The eighth palette swatch. Selecting it sets colour 8 and turns the
        // cycle flag on at the same instant - captured live - so Rainbow and
        // "cycling" are the same thing seen from two fields. The stepping
        // commands never reach it (they wrap 7 -> 1), so 8 arrives only from a
        // direct panel selection.
        return "8-Rainbow";
    } else if (raw == 255) {
        return "None";
    } else {
        return "Unknown";
    }
}

std::string IQ2020Component::decodeLightIntensity_(uint8_t raw) {
    // Intensity 0x05=High 0x01=Low 0x00=Off
    if(raw == 0x00) {
        return "0-Off";
    } else if (raw == 0x01) {
        return "Level_1";
    } else if (raw == 0x02) {
        return "Level_2";
    } else if (raw == 0x03) {
        return "Level_3";
    } else if (raw == 0x04) {
        return "Level_4";
    } else if (raw == 0x05) {
        return "Level_5";
    } else if (raw == 255) {
        return "None";
    } else {
        return "Unknown";
    }
}

std::string IQ2020Component::decodeLightSpeed_(uint8_t raw) {
    // Intensity 0x05=High 0x01=Low 0x00=Off
    if(raw == 0x00) {
        return "0:Pause";
    } else if (raw == 0x01) {
        return "1:Slow";
    } else if (raw == 0x02) {
        return "2:Normal";
    } else if (raw == 0x03) {
        return "3:Fast";
    } else {
        return "Unknown";
    }
}

// Salinity index (payload[2] >> 2) -> the value the controller puts on the panel.
// Salinity index -> position along the panel's salt scale, as a percentage.
//
// The panel does not show a salt concentration anywhere - it draws a marker on a
// bar. The controller converts the index through this table to get how far along
// that bar the marker sits, then adds a fixed origin to turn it into a screen
// coordinate. The origin is display geometry, not measurement, so it is left out
// here: what is reported is the position on the scale, 0-100%, which is exactly
// what the bar shows.
//
// Indices of 32 or more read as 0. The table is deliberately non-linear - about
// 5 units of travel per index at the bottom, 3 through the middle, and 6-7 at
// the top - so the marker barely moves while salt is in range and swings hard at
// the extremes.
float IQ2020Component::swg_salinity_from_index(uint8_t index) {
    static const uint8_t table[32] = {
          0,   5,  10,  15,  20,  25,  30,  35,
         40,  45,  50,  53,  56,  60,  63,  66,
         69,  73,  76,  79,  82,  85,  89,  92,
         95, 100, 106, 113, 119, 126, 132, 139,
    };
    uint8_t pos = (index < 32) ? table[index] : 0;
    return (float) pos * 100.0f / 139.0f;
}

// Human-readable SWG status. This mirrors the controller's own logic, restricted
// to the inputs visible on the bus - it also consults panel-local acknowledge
// flags and the pump/flow state, which a bus observer cannot see, so a few of
// its states are unreachable here.
std::string IQ2020Component::decodeSWGStatus_(uint8_t raw) {
    switch (raw) {
        case 0:  return "Okay";
        case 1:  return "Inactive - System Off";
        case 2:  return "24-Hour Boost Cycle On";
        // The controller shows no message here; the meaningful part is that the
        // salt test reading has locked level adjustment.
        case 3:  return "Level Locked - Confirm Salt Level";
        case 4:  return "Test Water & Confirm Level";
        case 5:  return "Level Set To 3 - Test & Adjust";
        case 6:  return "Level Set To 1 - Test & Adjust";
        case 7:  return "Restarting";
        case 8:  return "Inactive - No Circulation";
        case 9:  return "Inactive - Summer Timer On";
        case 10: return "Inactive - High Salt";
        case 11: return "High Salt";
        case 12: return "Inactive - Low Salt";
        case 18: return "Cartridge Reached 4 Months - Replace";
        case 19: return "Service Required - Contact Dealer";
        case 21: return "Timeout Error - Check Salt Cartridge";
        default: return "Unknown";
    }
}

std::string IQ2020Component::decodeLightNumber_(uint8_t raw) {
    if(raw == 0x00) {
        return "0:Underwater";
    } else if (raw == 0x01) {
        return "1:Bartop";
    } else if (raw == 0x02) {
        return "2:Pillow";
    } else if (raw == 0x03) {
        return "3:Exterior";
    } else if (raw == 0x04) {
        return "4:All";
    } else {
        return "Unknown";
    }
}

std::string IQ2020Component::decodeLightOperation_(uint8_t raw) {
    if(raw == 0x05) {
        return "ColorUp";
    } else if (raw == 0x04) {
        return "ColorDown";
    } else if (raw == 0x03) {
        return "BrightUp";
    } else if (raw == 0x02) {
        return "BrightDown";
    } else if (raw == 0x11) {
        return "AllOn";
    } else if (raw == 0x10) {
        return "AllOff";
    } else {
        return "Unknown";
    }
}

std::string IQ2020Component::decodeAddr_(uint8_t raw) {
    if(raw == 0x01) {
        return "0x0-IQ2020";
    } else if (raw == 0x21) {
        return "0x21-CoolZone";
    } else if (raw == 0x24) {
        return "0x24-ACE-SWG";
    } else if (raw == 0x29) {
        return "0x29-FW-SWG";
    } else if (raw == 0x33) {
        return "0x33-Music";
    } else if (raw == 0x1f) {
        return "0x1f-Remote";
    } else {
        //return "Unknown";
        return std::format("{:X}-Unk", raw);
    }
}


}  // namespace iq2020
}  //