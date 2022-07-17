#ifndef _PCA9634_H_
#define _PCA9634_H_

#include <cstdint>


#define PCA_ALL_CALL_ADDR   0x70
#define PCA_SOFT_RESET_ADDR 0x06

/* PCA9634 REGISTER MAP */
#define PCA_REG_MODE1       0x00
#define PCA_REG_MODE2       0x01
#define PCA_REG_PWM0        0x02
#define PCA_REG_PWM1        0x03
#define PCA_REG_PWM2        0x04
#define PCA_REG_PWM3        0x05
#define PCA_REG_PWM4        0x06
#define PCA_REG_PWM5        0x07
#define PCA_REG_PWM6        0x08
#define PCA_REG_PWM7        0x09
#define PCA_REG_GRPPWM      0x0A
#define PCA_REG_GRPFREQ     0x0B
#define PCA_REG_LEDOUT0     0x0C
#define PCA_REG_LEDOUT1     0x0D
#define PCA_REG_SUBADR1     0x0E
#define PCA_REG_SUBADR2     0x0F
#define PCA_REG_SUBADR3     0x10
#define PCA_REG_ALLCALLADR  0x11
#define PCA_REG_COUNT       0x12

#define PCA_NO_AUTO_INC             (0x00)
#define PCA_AUTO_INC_ALL            (0x04 << 5)
#define PCA_AUTO_INC_BRIGHTNESS     (0x05 << 5)
#define PCA_AUTO_INC_GLOBAL_CTRL    (0x06 << 5)
#define PCA_AUTO_INC_IND_GBL_CTRL   (0x07 << 5)

typedef void (*delay_func_t)(int);

typedef struct pca9634_chip {
    uint8_t addr;
    int i2c_fd;
    uint8_t regs[PCA_REG_COUNT];
    delay_func_t delay_us;
}pca9634_chip_t;

uint8_t pca_read_reg(pca9634_chip_t* chip, uint8_t reg);
void pca_read_all_regs(pca9634_chip_t* chip);
void pca_write_reg(pca9634_chip_t* chip, uint8_t reg, uint8_t value);
void pca_write_all_regs(pca9634_chip_t* chip);
void pca_wakeup(pca9634_chip_t* chip);
void pca_soft_reset(int i2c_fd);
void pca_update_brightness(pca9634_chip_t* chip);

#endif