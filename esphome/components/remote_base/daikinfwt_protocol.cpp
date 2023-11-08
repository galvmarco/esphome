#include "daikinfwt_protocol.h"
#include "esphome/core/log.h"
#include <cinttypes>

namespace esphome {
namespace remote_base {

static const char *const TAG = "remote.daikinfwt";

static const uint32_t HEADER_HIGH_US = 9900;
static const uint32_t HEADER_LOW_US = 9900;
static const uint32_t HEADER2_HIGH_US = 4600;
static const uint32_t HEADER2_LOW_US = 2600;
static const uint32_t BIT_HIGH_US = 330;
static const uint32_t BIT_ZERO_LOW_US = 400;
static const uint32_t BIT_ONE_LOW_US = 980;
static const uint32_t FOOTER_HIGH_US = 330;
static const uint32_t FOOTER_LOW_US = 330;


void DaikinfwtProtocol::encode(RemoteTransmitData *dst, const DaikinfwtData &data) {
  dst->set_carrier_frequency(38000);
  dst->reserve(6 + data.nbits * 2u + 2);

  dst->item(HEADER_HIGH_US, HEADER_LOW_US);
  dst->item(HEADER_HIGH_US, HEADER_LOW_US);
  dst->item(HEADER2_HIGH_US, HEADER2_LOW_US);


  for (uint8_t bit = data.nbits; bit > 0; bit--) {
    if ((data.data >> (bit - 1)) & 1) {
      dst->item(BIT_HIGH_US, BIT_ONE_LOW_US);
    } else {
      dst->item(BIT_HIGH_US, BIT_ZERO_LOW_US);
    }
  }

  dst->item(FOOTER_HIGH_US, FOOTER_LOW_US);
}
optional<DaikinfwtData> DaikinfwtProtocol::decode(RemoteReceiveData src) {
  DaikinfwtData out{
      .data = 0,
  };
  if (!src.expect_item(HEADER_HIGH_US, HEADER_LOW_US))
    return {};
  if (!src.expect_item(HEADER_HIGH_US, HEADER_LOW_US))
    return {};
  if (!src.expect_item(HEADER2_HIGH_US, HEADER2_LOW_US))
    return {};

  for (out.nbits = 0; out.nbits < 64; out.nbits++) {
    if (src.expect_item(BIT_HIGH_US, BIT_ONE_LOW_US)) {
      out.data = (out.data >> 1) | 0x8000000000000000u;
    } else if (src.expect_item(BIT_HIGH_US, BIT_ZERO_LOW_US)) {
      out.data = (out.data >> 1) | 0;
    } else {
      return {};
    }
  }

  if (!src.expect_mark(FOOTER_HIGH_US))
    return {};
  return out;
}
void DaikinfwtProtocol::dump(const DaikinfwtData &data) {
  ESP_LOGI(TAG, "Received Daikinfwt: data=0x%" PRIX64 ", nbits=%d", data.data, data.nbits);
}

}  // namespace remote_base
}  // namespace esphome
