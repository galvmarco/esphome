#pragma once

#include "esphome/components/climate_ir/climate_ir.h"

namespace esphome {
namespace daikin_fwt {

// Values for Daikin TL401 IR Controllers for FWT series fan coils
// Temperature
const uint8_t DAIKINFWT_TEMP_MIN = 16;  // Celsius
const uint8_t DAIKINFWT_TEMP_MAX = 30;  // Celsius

// Buttons
const uint8_t DAIKINFWT_BTN_PWR = 0x08;

// Modes
const uint8_t DAIKINFWT_MODE_DRY = 0x10;
const uint8_t DAIKINFWT_MODE_COOL = 0x20;
const uint8_t DAIKINFWT_MODE_FAN = 0x40;
const uint8_t DAIKINFWT_MODE_HEAT = 0x80;

// Fan Speed
const uint8_t DAIKINFWT_FAN_AUTO = 0x10;
const uint8_t DAIKINFWT_FAN_HIGH = 0x20;
const uint8_t DAIKINFWT_FAN_MEDIUM = 0x40;
const uint8_t DAIKINFWT_FAN_LOW = 0x80;
const uint8_t DAIKINFWT_FAN_BOOST = DAIKINFWT_FAN_AUTO || DAIKINFWT_FAN_HIGH;
const uint8_t DAIKINFWT_FAN_QUIET = DAIKINFWT_FAN_AUTO || DAIKINFWT_FAN_LOW;

// Sleep
const uint8_t DAIKINFWT_SLEEP = 0x02;

// Swing
const uint8_t DAIKINFWT_SWING_OFF = 0x00;
const uint8_t DAIKINFWT_SWING_VERTICAL = 0x01;


// IR Transmission
const uint32_t DAIKINFWT_IR_FREQUENCY_HZ = 38000;
const uint32_t DAIKINFWT_HEADER_MARK_US = 9900;
const uint32_t DAIKINFWT_HEADER_SPACE_US = 9900;
const uint32_t DAIKINFWT_HEADER2_MARK_US = 4600;
const uint32_t DAIKINFWT_HEADER2_SPACE_US = 2600;
const uint32_t DAIKINFWT_BIT_MARK_US = 330;
const uint32_t DAIKINFWT_ONE_SPACE_US = 980;
const uint32_t DAIKINFWT_ZERO_SPACE_US = 400;
const uint32_t DAIKINFWT_FOOTER_MARK_US = 330;
const uint32_t DAIKINFWT_FOOTER_SPACE_US = 330;


// State Frame size
const uint8_t DAIKINFWT_STATE_FRAME_SIZE = 8;

class DaikinFwtClimate : public climate_ir::ClimateIR {
 public:
  DaikinFwtClimate()
      : climate_ir::ClimateIR(DAIKINFWT_TEMP_MIN, DAIKINFWT_TEMP_MAX, 1.0f, true, true,
                              {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM,
                               climate::CLIMATE_FAN_HIGH},
                              {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL},
                              {climate::CLIMATE_PRESET_BOOST, climate::CLIMATE_PRESET_COMFORT, climate::CLIMATE_PRESET_SLEEP}) {}

 protected:
  // Transmit via IR the state of this climate controller.
  void transmit_state() override;
  uint8_t operation_mode_();
  uint16_t fan_speed_();
  uint8_t temperature_();
  uint8_t swing_();
  uint8_t sleep_();
  uint8_t preset_();
  // Handle received IR Buffer
  bool on_receive(remote_base::RemoteReceiveData data) override;
  bool parse_state_frame_(const uint8_t frame[]);

  static uint8_t computeDaikinFWTChecksum_(const uint8_t frame[]);
};

}  // namespace daikin
}  // namespace esphome
