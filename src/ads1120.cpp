#include "ads1120.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <errno.h>
#include <thread>
#include <chrono>
#include "util.h"


float compute_adc_voltage(int16_t conv, float vref){
    if(conv >= 0){
        return vref*conv/ADS1120_FS_COUNTS_POS;
    }else{
        return vref*conv/ADS1120_FS_COUNTS_NEG;
    }
}


float ADS1120::get_voltage(){
    return compute_adc_voltage(get_conv(), m_vref);
}


ADS1120::ADS1120(int spi_fd, std::string cs_pin_path, float vref){
    m_spi_fd = spi_fd;
    m_vref = vref;
    // m_spi_fd = open(spi_path.c_str(), O_RDWR);
    m_cs_pin_base = cs_pin_path;
    gpio_set_direction(m_cs_pin_base, PIN_DIR_OUT);
    deassert_cs();
}

void ADS1120::begin(uint8_t* config){
    this->reset();
    this->begin_spi_transaction();
    uint8_t cmd = ADS1120_CMD_WREG | 3; // starts at register 0
    this->assert_cs();
    write(m_spi_fd, &cmd, 1);
    write(m_spi_fd, config, 4);
    this->deassert_cs();

    // after a reset, must wait at least 50us + 32*(1/f_clk)
    // this waits for (1) the deassertion of cs line, and
    // (2) plenty of time no matter what operating mode we're in
    std::this_thread::sleep_for(std::chrono::microseconds(500)); 
}

void ADS1120::reset(){
    this->begin_spi_transaction();
    uint8_t cmd = ADS1120_CMD_RESET;
    this->assert_cs();
    write(m_spi_fd, &cmd, 1);
    this->deassert_cs();
}

void ADS1120::start_sync(){
    this->begin_spi_transaction();
    uint8_t cmd = ADS1120_CMD_START_SYNC;
    this->assert_cs();
    write(m_spi_fd, &cmd, 1);
    this->deassert_cs();
}


int16_t ADS1120::get_conv(){
    this->begin_spi_transaction();
    uint8_t cmd = ADS1120_CMD_RDATA;
    uint8_t conv_buf[2];
    this->assert_cs();
    write(m_spi_fd, &cmd, 1);
    read(m_spi_fd, conv_buf, 2);
    this->deassert_cs();
    return (int16_t)((conv_buf[0] << 8) | conv_buf[1]);
}

void ADS1120::begin_spi_transaction(){
    uint8_t mode = SPI_MODE_1;
    if(ioctl(m_spi_fd, SPI_IOC_WR_MODE, &mode) < 0){
        printf("Can't set SPI mode\n");
        return;
    }
    uint32_t speed = ADS1120_SPI_SPEED;
    if(ioctl(m_spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0){
        printf("Can't set SPI speed\n");
        return;
    }
    uint8_t bits_per_word = 8;
    if(ioctl(m_spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits_per_word) < 0){
        printf("Can't set SPI bits per word\n");
        return;
    }
    uint8_t msb_first = 0;
    if(ioctl(m_spi_fd, SPI_IOC_WR_LSB_FIRST, &msb_first) < 0){
        printf("Can't set SPI LSB first to 0\n");
        return;
    }
}


void ADS1120::set_input_mux(uint8_t config){
    uint8_t cmd = ADS1120_CMD_RREG; // register 0 (rr = 0), 1 bytes (nn = 0)
    uint8_t reg = 0;

    begin_spi_transaction();
    assert_cs();
    write(m_spi_fd, &cmd, 1);
    read(m_spi_fd, &reg, 1);
    deassert_cs();
    std::this_thread::sleep_for(std::chrono::microseconds(100));

    reg &= 0x0F; // clear mux bits [7:4];
    reg |= config;
    cmd = ADS1120_CMD_WREG; // rr = 0, nn = 0
    assert_cs();
    write(m_spi_fd, &cmd, 1);
    write(m_spi_fd, &reg, 1);
    deassert_cs();
}

void ADS1120::assert_cs(){
    gpio_set_value(m_cs_pin_base, 0);
    std::this_thread::sleep_for(std::chrono::microseconds(100));
}
void ADS1120::deassert_cs(){
    gpio_set_value(m_cs_pin_base, 1);
}

ADS1120::~ADS1120(){
}