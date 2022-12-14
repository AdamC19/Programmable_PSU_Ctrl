
#include "max5541.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>

#include "util.h"

MAX5541::MAX5541(int spi_fd, int cs_mux_value, set_mux_func set_mux, float vref){
    m_spi_fd = spi_fd;
    m_vref = vref;
    m_set_mux = set_mux;
    m_cs_mux_value = cs_mux_value;
}

void MAX5541::write_value(uint16_t value){
    uint8_t buf[2];
    buf[0] = value >> 8;
    buf[1] = value & 0xFF;
    this->begin_spi_transaction();
    
    write(m_spi_fd, buf, 2);
}


void MAX5541::set_voltage(float voltage){
    float f_counts = (voltage/(2.0*m_vref))*MAX5541_FS_COUNTS;
    uint16_t counts = (uint16_t)(f_counts + 0.5);
    write_value(counts);
}

void MAX5541::begin_spi_transaction(){
    uint8_t mode = SPI_MODE_0;
    if(ioctl(m_spi_fd, SPI_IOC_WR_MODE, &mode) < 0){
        printf("Can't set SPI mode.\n");
        return;
    }
    uint32_t speed = 2500000;
    if(ioctl(m_spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0){
        printf("Can't set SPI speed.\n");
        return;
    }
    uint8_t bits_per_word = 8;
    if(ioctl(m_spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits_per_word) < 0){
        printf("Can't set SPI bits per word.\n");
        return;
    }
    uint8_t msb_first = 0;
    if(ioctl(m_spi_fd, SPI_IOC_WR_LSB_FIRST, &msb_first) < 0){
        printf("Can't set SPI LSB first to 0.\n");
        return;
    }
    m_set_mux(m_cs_mux_value);
}
