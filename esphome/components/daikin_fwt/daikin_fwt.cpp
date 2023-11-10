#include "daikin_fwt.h"
#include "esphome/components/remote_base/remote_base.h"

namespace esphome {
namespace daikin_fwt {

static const char *const TAG = "daikin_fwt.climate";

void DaikinFwtClimate::transmit_state() {
  uint8_t remote_state[DAIKINFWT_STATE_FRAME_SIZE] = { 0xF4, 0x24, 0x10, 0x02, 0x00, 0x40, 0x82, 0x16 };

  remote_state[0] = this->swing_() | this->sleep_() | DAIKINFWT_BTN_PWR;

  remote_state[1] = this->temperature_();

  if(this->preset_()){
    remote_state[6] = this->operation_mode_() | this->preset_();
  }
  else {
    remote_state[6] = this->operation_mode_() | this->fan_speed_();
  }

  // Calculate checksum
  uint8_t checksum = this->computeDaikinFWTChecksum_(remote_state);
  remote_state[0] = remote_state[0] & checksum;

  auto transmit = this->transmitter_->transmit();
  auto *data = transmit.get_data();
  data->set_carrier_frequency(DAIKINFWT_IR_FREQUENCY_HZ);

  data->mark(DAIKINFWT_HEADER_MARK_US);
  data->space(DAIKINFWT_HEADER_SPACE_US);
  data->mark(DAIKINFWT_HEADER_MARK_US);
  data->space(DAIKINFWT_HEADER_SPACE_US);
  data->mark(DAIKINFWT_HEADER2_MARK_US);
  data->space(DAIKINFWT_HEADER2_SPACE_US);

  for(uint8_t frame_idx=0; frame_idx<DAIKINFWT_STATE_FRAME_SIZE; frame_idx++)
  {
    uint8_t frame_data = remote_state[frame_idx];
    for (uint8_t bit = 0; bit < 8; bit++) {
      data->mark(DAIKINFWT_BIT_MARK_US);
      if ((frame_data >> bit ) & 1) {
        data->space(DAIKINFWT_ONE_SPACE_US);
      } else {
        data->space(DAIKINFWT_ZERO_SPACE_US);
      }
    }
  }

  data->mark(DAIKINFWT_FOOTER_MARK_US);
  data->space(DAIKINFWT_FOOTER_SPACE_US);

  transmit.perform();
}

uint8_t DaikinFwtClimate::swing_() {
  uint8_t swing = 0;
  switch (this->swing_mode.value()) {
    case climate::CLIMATE_SWING_OFF:
      swing = DAIKINFWT_SWING_OFF;
      break;
    case climate::CLIMATE_SWING_VERTICAL:
      swing = DAIKINFWT_SWING_VERTICAL;
      break;
    default:
    break;
  }

  return swing;
}

uint8_t DaikinFwtClimate::sleep_() {
  uint8_t sleep = 0;

  if(this->preset == climate::CLIMATE_PRESET_SLEEP)
  {
    sleep = DAIKINFWT_SLEEP;
  }

  return sleep;
}

uint8_t DaikinFwtClimate::operation_mode_() {
  uint8_t operating_mode = 0;
  switch (this->mode.value()) {
    case climate::CLIMATE_MODE_COOL:
      operating_mode |= DAIKINFWT_MODE_COOL;
      break;
    case climate::CLIMATE_MODE_DRY:
      operating_mode |= DAIKINFWT_MODE_DRY;
      break;
    case climate::CLIMATE_MODE_HEAT:
      operating_mode |= DAIKINFWT_MODE_HEAT;
      break;
    case climate::CLIMATE_MODE_FAN_ONLY:
      operating_mode |= DAIKINFWT_MODE_FAN;
      break;
    default:
      operating_mode = DAIKINFWT_MODE_COOL;
      break;
  }

  return operating_mode;
}

uint16_t DaikinFwtClimate::fan_speed_() {
  uint16_t fan_speed;
  switch (this->fan_mode.value()) {
    case climate::CLIMATE_FAN_LOW:
      fan_speed = DAIKINFWT_FAN_LOW;
      break;
    case climate::CLIMATE_FAN_MEDIUM:
      fan_speed = DAIKINFWT_FAN_MEDIUM;
      break;
    case climate::CLIMATE_FAN_HIGH:
      fan_speed = DAIKINFWT_FAN_HIGH;
      break;
    case climate::CLIMATE_FAN_AUTO:
    default:
      fan_speed = DAIKINFWT_FAN_AUTO;
  }

  return fan_speed;
}

uint8_t DaikinFwtClimate::preset_() {
  uint8_t preset = 0;

  switch (this->preset.value()) {
    case climate::CLIMATE_PRESET_BOOST:
      preset = DAIKINFWT_FAN_BOOST;
      break;
    case climate::CLIMATE_PRESET_COMFORT:
      preset = DAIKINFWT_FAN_QUIET;
      break;
    default:
      preset = 0;
  }
  
  return preset;

}

uint8_t DaikinFwtClimate::temperature_() {
  uint8_t temperature_round = (uint8_t) roundf(clamp<float>(this->target_temperature, DAIKINFWT_TEMP_MIN, DAIKINFWT_TEMP_MAX));
  uint8_t temperature = ((temperature_round/10) << 4) & (temperature_round%10);

  return temperature;
}

bool DaikinFwtClimate::parse_state_frame_(const uint8_t frame[]) {
  uint8_t checksum_computed;
  
  uint8_t checksum_received = ((state_frame[0] & 0xF0) >> 4);

  uint8_t btn_pwr = frame[0] & DAIKINFWT_BTN_PWR;
  uint8_t swing_mode = frame[0] & DAIKINFWT_SWING_VERTICAL;
  uint8_t sleep = frame[0] & DAIKINFWT_SLEEP;
  
  uint8_t temperature = ((frame[1] 0xF0) >> 4)*10 + (frame[1] 0x0F);

  uint8_t fan_mode = frame[6] & 0xF0;

  uint8_t mode = frame[6] & 0x0F;

  checksum_computed = computeDaikinFWTChecksum(frame);

  if( checksum_computed != checksum_received) {
    return false;
  }

  if (btn_pwr && this->mode == climate::CLIMATE_MODE_OFF) {
    switch (mode) {
      case DAIKINFWT_MODE_COOL:
        this->mode = climate::CLIMATE_MODE_COOL;
        break;
      case DAIKINFWT_MODE_DRY:
        this->mode = climate::CLIMATE_MODE_DRY;
        break;
      case DAIKINFWT_MODE_HEAT:
        this->mode = climate::CLIMATE_MODE_HEAT;
        break;
      case DAIKINFWT_MODE_FAN:
        this->mode = climate::CLIMATE_MODE_FAN_ONLY;
        break;
    }
  } else {
    this->mode = climate::CLIMATE_MODE_OFF;
  }

  if(temperature < DAIKINFWT_TEMP_MIN) {
    temperature = DAIKINFWT_TEMP_MIN;
  }

  if(temperature > DAIKINFWT_TEMP_MAX) {
    temperature = DAIKINFWT_TEMP_MAX;
  }

  this->target_temperature = temperature;

  if (swing_mode) {
    this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
  } else {
    this->swing_mode = climate::CLIMATE_SWING_OFF;
  }

  switch (fan_mode) {
    case DAIKINFWT_FAN_LOW:
      this->fan_mode = climate::CLIMATE_FAN_LOW;
      this->preset = climate::CLIMATE_PRESET_NONE;
      break;
    case DAIKINFWT_FAN_MEDIUM:
      this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
      this->preset = climate::CLIMATE_PRESET_NONE;
      break;
    case DAIKINFWT_FAN_HIGH:
      this->fan_mode = climate::CLIMATE_FAN_HIGH;
      this->preset = climate::CLIMATE_PRESET_NONE;
      break;
    case DAIKINFWT_FAN_AUTO:
      this->fan_mode = climate::CLIMATE_FAN_AUTO;
      this->preset = climate::CLIMATE_PRESET_NONE;
      break;
    case DAIKINFWT_FAN_BOOST:
      this->fan_mode = climate::CLIMATE_FAN_HIGH;
      this->preset = climate::CLIMATE_PRESET_BOOST;
      break;
    case DAIKINFWT_FAN_QUIET:
      this->fan_mode = climate::CLIMATE_FAN_QUIET;
      this->preset = climate::CLIMATE_PRESET_COMFORT;
      break;
  }

  if( sleep ) {
    this->preset = climate::CLIMATE_PRESET_SLEEP;
  }
  
  this->publish_state();
  return true;
}

bool DaikinFwtClimate::on_receive(remote_base::RemoteReceiveData data) {
  uint8_t state_frame[DAIKINFWT_STATE_FRAME_SIZE] = {};
  uint64_t state_data;
  uint8_t nbits;

  if (!data.expect_item(DAIKINFWT_HEADER_MARK_US, DAIKINFWT_HEADER_SPACE_US))
    return false;
  if (!data.expect_item(DAIKINFWT_HEADER_MARK_US, DAIKINFWT_HEADER_SPACE_US))
    return false;
  if (!data.expect_item(DAIKINFWT_HEADER2_MARK_US, DAIKINFWT_HEADER2_SPACE_US))
    return false;

  for (nbits = 0; nbits < DAIKINFWT_STATE_FRAME_SIZE*8; nbits++) {
    if (data.expect_item(DAIKINFWT_BIT_MARK_US, DAIKINFWT_ONE_SPACE_US)) {
      state_data = (state_data >> 1) | 0x8000000000000000u;
    } else if (data.expect_item(DAIKINFWT_BIT_MARK_US, DAIKINFWT_ZERO_SPACE_US)) {
      state_data = (state_data >> 1) | 0;
    } else {
      return false;
    }
  }

  for(int pos=0; pos<DAIKINFWT_STATE_FRAME_SIZE; pos++) {
    state_frame[pos] = (state_data >> pos*8) & 0xFFu;
  }
  return this->parse_state_frame_(state_frame);
}

uint8_t DaikinFwtClimate::computeDaikinFWTChecksum_(const uint8_t frame[]) {
  uint8_t result = 0;
  uint8_t data = 0;
  data = frame[0];
  result += data & 0x0F;
  for(int i=1; i<DAIKINFWT_STATE_FRAME_SIZE; i++)
  {
    data = frame[i];
    result += data & 0x0F;
    data = data >> 4;
    result += data & 0x0F;
  }

  result = result % 16u;

  return result;
}

}  // namespace daikin
}  // namespace esphome
