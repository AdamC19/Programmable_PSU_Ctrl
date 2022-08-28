#pragma once

#include <stdint.h>
#include <string>
#include "util.h"

#define MAX5541_FS_COUNTS   32767.0

class MAX5541 {

private:
    int m_cs_pin_fd;
    std::string m_cs_pin_base;
    int m_spi_fd;
    float m_vref;
    set_mux_func m_set_mux;
    int m_cs_mux_value;
    void begin_spi_transaction();
public:
    MAX5541(int spi_fd, int cs_mux_value, set_mux_func set_mux, float vref);
    void write_value(uint16_t value);
    void set_voltage(float voltage);
};