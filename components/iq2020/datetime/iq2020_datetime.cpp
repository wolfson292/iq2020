#include "iq2020_datetime.h"


static const char *const TAG = "IQ2020.button";

namespace esphome {
namespace iq2020 {

    static const char *TAG = "iq2020.datetime";

void IQ2020DateTime::control(const datetime::DateTimeCall &call) {
    
    ESP_LOGI(TAG, "control HasYear:%d HasMonth:%d HasDay:%d HasHour:%d HasMinute:%d HasSecond:%d", 
        call.get_year().has_value(),
        call.get_month().has_value(),
        call.get_day().has_value(),
        call.get_hour().has_value(),
        call.get_minute().has_value(),
        call.get_second().has_value()
    );
    
    bool has_year = call.get_year().has_value();
    bool has_month = call.get_month().has_value();
    bool has_day = call.get_day().has_value();
    bool has_hour = call.get_hour().has_value();
    bool has_minute = call.get_minute().has_value();
    bool has_second = call.get_second().has_value();

    ESP_LOGI(TAG, "Year:%u", call.get_year().value_or(0));
    ESP_LOGI(TAG, "Month:%u", call.get_month().value_or(0));
    ESP_LOGI(TAG, "Day:%u", call.get_day().value_or(0));
    ESP_LOGI(TAG, "Hour:%u", call.get_hour().value_or(0));
    ESP_LOGI(TAG, "Minute:%u", call.get_minute().value_or(0));
    ESP_LOGI(TAG, "Second:%u", call.get_second().value_or(0));


    if(has_year && has_month && has_day && has_hour && has_minute && has_second)
    {
        
        if(this->parent_ != nullptr) {
            ESP_LOGI(TAG, "Setting IQ2020 datetime");
            this->parent_->sendCmdSetDateTime(call.get_second().value(), call.get_minute().value(), call.get_hour().value(), call.get_day().value(), call.get_month().value(), call.get_year().value());
        } else {
            ESP_LOGI(TAG, "parent null");
        }
    }
    if (has_year)
        this->year_ = *call.get_year();
    if (has_month)
        this->month_ = *call.get_month();
    if (has_day)
        this->day_ = *call.get_day();
    if (has_hour)
        this->hour_ = *call.get_hour();
    if (has_minute)
        this->minute_ = *call.get_minute();
    if (has_second)
        this->second_ = *call.get_second();
    this->publish_state();
}
     


}  // namespace iq2020
}  // namespace esphome
