
extern "C"
{
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <i2c/smbus.h>
}
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "pca9634.h"

/**
 * @brief 
 * 
 * @param chip 
 * @param reg 
 * @return uint8_t 
 */
uint8_t pca_read_reg(pca9634_chip_t* chip, uint8_t reg){
    if(ioctl(chip->i2c_fd, I2C_SLAVE, chip->addr) < 0){
        return 0;
    }
    chip->regs[reg] = (uint8_t)i2c_smbus_read_byte_data(chip->i2c_fd, reg);
    return chip->regs[reg];
}

/**
 * @brief Read all registers of the chip, storing them in the regs
 * field of the chip struct
 * 
 * @param chip the struct that holds the info for accessing this chip
 */
void pca_read_all_regs(pca9634_chip_t* chip){
    if(ioctl(chip->i2c_fd, I2C_SLAVE, chip->addr) < 0){
        return;
    }
    uint8_t cmd = PCA_AUTO_INC_ALL; // auto-increment for all registers
    write(chip->i2c_fd, &cmd, 1);
    int n_read = read(chip->i2c_fd, chip->regs, PCA_REG_COUNT);
    if(n_read != PCA_REG_COUNT){
        // error occurred
        return;
    }
}

/**
 * @brief 
 * 
 * @param chip 
 * @param reg 
 * @param value 
 */
void pca_write_reg(pca9634_chip_t* chip, uint8_t reg, uint8_t value){
    if(ioctl(chip->i2c_fd, I2C_SLAVE, chip->addr) < 0){
        return;
    }
    i2c_smbus_write_byte_data(chip->i2c_fd, reg, value);
}

/**
 * @brief Write all registers of the chip, setting them to whatever values 
 * are in the regs field of the chip struct
 * 
 * @param chip 
 */
void pca_write_all_regs(pca9634_chip_t* chip){
    if(ioctl(chip->i2c_fd, I2C_SLAVE, chip->addr) < 0){
        return;
    }
    uint8_t cmd = PCA_AUTO_INC_ALL; // auto-increment for all registers
    i2c_smbus_write_block_data(chip->i2c_fd, cmd, PCA_REG_COUNT, chip->regs);
}

/**
 * @brief Reset the sleep bit in the MODE1 register, waking up the chip
 * 
 * @param chip 
 */
void pca_wakeup(pca9634_chip_t* chip){
    uint8_t reg = pca_read_reg(chip, PCA_REG_MODE1);
    reg &= ~(1 << 4); // RESET SLEEP BIT
    pca_write_reg(chip, PCA_REG_MODE1, reg);
}

/**
 * 
 */
void pca_set_output_config(pca9634_chip_t* chip, uint8_t config){
    uint8_t reg = pca_read_reg(chip, PCA_REG_MODE2);
    if(config){
        reg |= (1 << 2);
    }else{
        reg &= ~(1 << 2);
    }
    pca_write_reg(chip, PCA_REG_MODE2, reg);
}


void pca_soft_reset(int i2c_fd){
    if(ioctl(i2c_fd, I2C_SLAVE, 0x03) < 0){
        return;
    }
    uint8_t data[] = {0xA5, 0x5A};
    write(i2c_fd, data, 2);
}


void pca_update_brightness(pca9634_chip_t* chip){
    if(ioctl(chip->i2c_fd, I2C_SLAVE, chip->addr) < 0){
        return;
    }
    uint8_t cmd = PCA_AUTO_INC_BRIGHTNESS | PCA_REG_PWM0; // auto-increment for brightness only
    i2c_smbus_write_block_data(chip->i2c_fd, cmd, 8, chip->regs + PCA_REG_PWM0);
}


void set_segment_value(pca9634_chip_t* chip, uint8_t* value, bool dp){
    uint16_t ledout_regs = 0;
    for(int i = 0; i < 7; i++){
        if(value[i]){
            ledout_regs |= (0x03 << (2*i)); // this segment shall have its brightness controlled by the PWM registers
        }
    }
    if(dp){
        ledout_regs |= (0x03 << 14); // decimal point shall have its brightness controlled by the PWM regs
    }
    uint8_t ledout0_reg = ledout_regs & 0xFF;
    uint8_t ledout1_reg = (ledout_regs >> 8) & 0xFF;
    pca_write_reg(chip, PCA_REG_LEDOUT0, ledout0_reg);
    pca_write_reg(chip, PCA_REG_LEDOUT1, ledout1_reg);
}