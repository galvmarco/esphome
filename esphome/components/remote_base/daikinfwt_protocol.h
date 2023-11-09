#pragma once

#include "esphome/core/component.h"
#include "remote_base.h"

namespace esphome {
namespace remote_base {

struct DaikinfwtData {
  uint64_t data;
  uint8_t nbits;
  uint8_t checksum;

  bool operator==(const DaikinfwtData &rhs) const { return data == rhs.data && nbits == rhs.nbits && checksum == rhs.checksum; }
};

class DaikinfwtProtocol : public RemoteProtocol<DaikinfwtData> {
 public:
  void encode(RemoteTransmitData *dst, const DaikinfwtData &data) override;
  optional<DaikinfwtData> decode(RemoteReceiveData src) override;
  void dump(const DaikinfwtData &data) override;
  static uint8_t computeDaikinFWTChecksum(uint64_t data);
};

DECLARE_REMOTE_PROTOCOL(Daikinfwt)

template<typename... Ts> class DaikinfwtAction : public RemoteTransmitterActionBase<Ts...> {
 public:
  TEMPLATABLE_VALUE(uint64_t, data)
  TEMPLATABLE_VALUE(uint8_t, nbits)

  void encode(RemoteTransmitData *dst, Ts... x) override {
    DaikinfwtData data{};
    data.data = this->data_.value(x...);
    data.nbits = this->nbits_.value(x...);
    DaikinfwtProtocol().encode(dst, data);
  }
};

}  // namespace remote_base
}  // namespace esphome
