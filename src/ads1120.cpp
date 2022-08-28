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
#include <errno.h>


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

void ADS1120::self_cal(){
    printf("BEGINNING ADS1120 SELF CAL...\r\n");
    uint8_t buf[5];

    buf[0] = ADS1120_CMD_WREG | (1 << 2) | 1;
    buf[1] = ADS1120_DR_2 | ADS1120_MODE_NORMAL | ADS1120_CM_CONT;
    buf[2] = ADS1120_VREF_2_048;
    spi_xfer(buf, 3);

    set_input_mux(ADS1120_MUX_VREF_DIV_4);

    start_sync();
    m_vref = 2.048;
    float vref_cal = 0.0;

    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); 
    for(int i = 0; i < 4; i++){
        std::this_thread::sleep_for(std::chrono::milliseconds(25)); 
        vref_cal += get_voltage();
    }
    m_vref = vref_cal;
    printf("SELF CAL COMPELTE. VREF = %.6f\r\n", m_vref);
}


ADS1120::ADS1120(int spi_fd, int cs_mux_value, set_mux_func set_mux, float vref){
    m_spi_fd = spi_fd;
    m_vref = vref;
    m_cs_mux_value = cs_mux_value;
    m_set_mux = set_mux;
}

void ADS1120::spi_xfer(uint8_t* data, int len){
    struct spi_ioc_transfer xfer;
    memset(&xfer, 0, sizeof(struct spi_ioc_transfer));
    xfer.tx_buf = (unsigned long)data;
    xfer.len = len;
    xfer.rx_buf = (unsigned long)data;
    xfer.delay_usecs = 10;
    xfer.speed_hz = ADS1120_SPI_SPEED;
    xfer.cs_change = 0;
    xfer.bits_per_word = 8;
    
    if(ioctl(m_spi_fd, SPI_IOC_MESSAGE(1), &xfer) < 0){
        printf("SPI XFER FAILED: %s\r\n", strerror(errno));
        return;
    }
}

void ADS1120::begin(uint8_t* config){
    this->reset();
    // after a reset, must wait at least 50us + 32*(1/f_clk)
    // this waits for (1) the deassertion of cs line, and
    // (2) plenty of time no matter what operating mode we're in
    std::this_thread::sleep_for(std::chrono::microseconds(1000)); 

    this->begin_spi_transaction();
    uint8_t buf[5] = {0};
    uint8_t cmd = ADS1120_CMD_WREG | 3; // starts at register 0
    // this->assert_cs();
    buf[0] = cmd;
    buf[1] = config[0];
    buf[2] = config[1];
    buf[3] = config[2];
    buf[4] = config[3];
    write(m_spi_fd, buf, 5);
    // write(m_spi_fd, config, 4);
    // this->deassert_cs();
}

void ADS1120::reset(){
    this->begin_spi_transaction();
    uint8_t cmd = ADS1120_CMD_RESET;
    // this->assert_cs();
    write(m_spi_fd, &cmd, 1);
    // this->deassert_cs();
}

void ADS1120::start_sync(){
    this->begin_spi_transaction();
    uint8_t cmd = ADS1120_CMD_START_SYNC;
    // this->assert_cs();
    write(m_spi_fd, &cmd, 1);
    // this->deassert_cs();
}


int16_t ADS1120::get_conv(){
    this->begin_spi_transaction();
    // uint8_t cmd = ADS1120_CMD_RDATA;
    uint8_t buf[3] = {ADS1120_CMD_RDATA, 0, 0};
    
    spi_xfer(buf, 3);
    // write(m_spi_fd, &cmd, 1);
    // read(m_spi_fd, conv_buf, 2);
    int16_t retval = (buf[1] << 8) | buf[2];
    // printf("CONV: %d\r\n", retval);
    return retval;
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
    m_set_mux(m_cs_mux_value); // set the chip select mux to our value
}


void ADS1120::set_input_mux(uint8_t config){
    uint8_t buf[2];
    buf[0] = ADS1120_CMD_RREG; // register 0 (rr = 0), 1 bytes (nn = 0)
    buf[1] = 0;
    begin_spi_transaction();
    std::this_thread::sleep_for(std::chrono::microseconds(10));

    spi_xfer(buf, 2);
    
    // write(m_spi_fd, &cmd, 1);
    // read(m_spi_fd, &reg, 1);
    
    // std::this_thread::sleep_for(std::chrono::microseconds(100));

    buf[1] &= 0x0F; // clear mux bits [7:4];
    buf[1] |= config;
    buf[0] = ADS1120_CMD_WREG; // rr = 0, nn = 0

    spi_xfer(buf, 2);

    // write(m_spi_fd, &cmd, 1);
    // write(m_spi_fd, &reg, 1);
    
}

ADS1120::~ADS1120(){
}