#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/modbus/modbus.h"

#define SENSOR_SETTER(name) void set_ ## name ## _sensor(sensor_t *s) { this-> ## name ## _sensor_ = s; }
#define PHASE_SENSOR_SETTER(name) void set_phase_ ## name ## _sensor(uint8_t ph, sensor_t *s)\
        { this->phases_[ph]. ## name ## _sensor_ = s; }
#define SENSOR(name) sensor_t *name ## _sensor_ { nullptr }

namespace esphome {
namespace jsy_meter {

using sensor_t = sensor::Sensor;
static const char *TAG = "jsy_meter";

static const uint8_t MODBUS_CMD_READ = 0x03;
static const float UNIT_NO_DEC  = 1;
static const float UNIT_ONE_DEC = 0.1;
static const float UNIT_TWO_DEC = 0.01;

class JSYMeter : public PollingComponent, public modbus::ModbusDevice {
public:
  void loop() override;
  void update() override;
  void setup() override;
  void on_modbus_data(const std::vector<uint8_t>& data) override;
  void dump_config() override;

  SENSOR_SETTER(total_active_power);
  SENSOR_SETTER(total_forward_active_energy);
  SENSOR_SETTER(total_backward_active_energy);
  SENSOR_SETTER(frequency);
  PHASE_SENSOR_SETTER(active_power);
  PHASE_SENSOR_SETTER(forward_active_energy);
  PHASE_SENSOR_SETTER(backward_active_energy);
  PHASE_SENSOR_SETTER(voltage);
  PHASE_SENSOR_SETTER(current);
protected:

  bool update_needed_;
  uint32_t last_send_;
  std::vector<uint8_t> payload_ = {
    0x00,               /* To be filled with the modbus address */
    MODBUS_CMD_READ,
    0x01, 0x00,         /* Start address: 0x0100 */
    0x00, 0x44,         /* Read 0x44 registers */
  };

  SENSOR(total_active_power);
  SENSOR(total_forward_active_energy);
  SENSOR(total_backward_active_energy);
  SENSOR(frequency);

  struct Phase {
      SENSOR(voltage);
      SENSOR(current);
      SENSOR(active_power);
      SENSOR(forward_active_energy);
      SENSOR(backward_active_energy);
  } phases_[3];
};

}
}