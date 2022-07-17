#pragma once

#include <stdint.h>
#include <string>

#define MAX5541_FS_COUNTS   32767.0

class MAX5541 {

private:
    int m_cs_pin_fd;
    std::string m_cs_pin_base;
    int m_spi_fd;
    float m_vref;
    void begin_spi_transaction();
    void assert_cs();
    void deassert_cs();
public:
    MAX5541(int spi_fd, std::string cs_pin_path, float vref);
    void write_value(uint16_t value);
    void set_voltage(float voltage);
};