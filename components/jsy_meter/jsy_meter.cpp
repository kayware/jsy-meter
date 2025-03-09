
#include "jsy_meter.h"

namespace esphome {
namespace jsy_meter {

void JSYMeter::loop() {
  if (!this->update_needed_)
    return;
  this->update();
}

void JSYMeter::setup() {
  this->payload_[0] = this->address_;
}

void JSYMeter::update() {
  uint32_t now = millis();
  if (now - this->last_send_ < this->get_update_interval() / 2)
    return;
  if (this->waiting_for_response()) {
    this->update_needed_ = true;
    return;
  }
  this->update_needed_ = false;
  this->send_raw(this->payload_);
  this->last_send_ = millis();
}

void JSYMeter::on_modbus_data(const std::vector<uint8_t> &data) {
  if (!this->last_send_)
    return;
  this->last_send_ = 0;

  if (data.size() != 136) {
    this->log_error(data);
    return;
  }

  this->parse_registers(data);
}

void JSYMeter::log_error(const std::vector<uint8_t> &data) {
  ESP_LOGW(TAG, "Received %ld bytes from inverter, expected a differently sized response.", data.size());
}

void JSYMeter::parse_registers(const std::vector<uint8_t> &data) {
  auto read8 = [&](size_t off) -> uint8_t {
    return data[off*2];
  };

  auto read16 = [&](size_t off) -> uint16_t {
    return encode_uint16(data[off*2], data[off*2 + 1]);
  };

  auto read_reg16 = [&](size_t off, float unit) -> float {
    return (int16_t)read16(off) * unit;
  };

  auto read_regU16 = [&](size_t off, float unit) -> float {
    return read16(off) * unit;
  };

  auto read_regU32 = [&](size_t off, float unit) -> float {
    return encode_uint32(data[off*2], data[off*2 + 1], data[off*2 + 2], data[off*2 + 3]) * unit;
  };

  auto read_reg32 = [&](size_t off, float unit) -> float {
    return (int32_t)encode_uint32(data[off*2], data[off*2 + 1], data[off*2 + 2], data[off*2 + 3]) * unit;
  };

  auto update_sensor = [&](sensor_t *s, float value) {
    if (s)
        s->publish_state(value);
  };

  auto power_direction = read16(0x32);

  for (uint16_t i = 0; i < 3; i++) {
    float direction = (power_direction & (1 << i)) ? -1 : 1;
    update_sensor(this->phases_[i].voltage_, read_regU16(i, UNIT_TWO_DEC));
    update_sensor(this->phases_[i].current_, read_regU16(i + 3, UNIT_TWO_DEC));
    update_sensor(this->phases_[i].active_power_, read_regU16(i + 6, UNIT_NO_DEC) * direction);
    update_sensor(this->phases_[i].forward_active_energy_, read_regU32(i*2 + 0x34, UNIT_TWO_DEC));
    update_sensor(this->phases_[i].backward_active_energy_, read_regU32(i*2 + 0x3c, UNIT_TWO_DEC));
  }

  float direction = (power_direction & (1 << 3)) ? -1 : 1;
  update_sensor(this->total_active_power_sensor_, read_regU32(0x09, UNIT_NO_DEC) * direction);
  update_sensor(this->total_forward_active_energy_sensor_, read_regU32(0x3A, UNIT_TWO_DEC));
  update_sensor(this->total_backward_active_energy_sensor_, read_regU32(0x42, UNIT_TWO_DEC));
  update_sensor(this->frequency_sensor_, read_regU16(0x15, UNIT_TWO_DEC));
}

void JSYMeter::dump_config() {
  ESP_LOGCONFIG(TAG, "JSY Meter:");
  ESP_LOGCONFIG(TAG, "  Address: 0x%02X", this->address_);
}

}
}
