
#include <cstdlib>
#include <cstdio>
#include <string>
#include <cstring>
#include <thread>
#include <chrono>

#include "util.h"
#include "seven_segment.h"
#include "pca9634.h"
#include "segment_display.h"

SegmentDisplay::SegmentDisplay(int n_characters, uint8_t* addresses, int i2c_fd, std::string oe_gpio){
    m_oe_gpio = oe_gpio;
    gpio_set_direction(m_oe_gpio, 1);
    output_disable();
    m_characters = n_characters;
    m_display = (pca9634_chip_t*)malloc(sizeof(pca9634_chip_t) * m_characters);
    for(int i = 0; i < m_characters; i++){
        m_display[i].addr = addresses[i];
        m_display[i].i2c_fd = i2c_fd;
        pca_read_all_regs(&m_display[i]);
        m_display[i].regs[PCA_REG_MODE2] &= ~(1 << 2); // clear the OUTDRV bit, meaning open drain outputs
        pca_write_reg(&m_display[i], PCA_REG_MODE2, m_display[i].regs[PCA_REG_MODE2]);
        m_display[i].regs[PCA_REG_MODE1] &= ~(1 << 4); // RESET SLEEP BIT (WAKE UP CHIP)
        pca_write_reg(&m_display[i], PCA_REG_MODE1, m_display[i].regs[PCA_REG_MODE1]);
        // pca_write_all_regs(&m_display[i]);
    }
    std::this_thread::sleep_for(std::chrono::microseconds(500));
}


void SegmentDisplay::output_disable(){
    gpio_set(m_oe_gpio, 1);
}
void SegmentDisplay::output_enable(){
    gpio_set(m_oe_gpio, 0);
}


void SegmentDisplay::set_brightness(float duty){
    uint8_t pwm_reg = (uint8_t)(255*duty + 0.5);
    for(int i = 0; i < m_characters; i++){
        m_display[i].regs[PCA_REG_PWM0] = pwm_reg;
        m_display[i].regs[PCA_REG_PWM1] = pwm_reg;
        m_display[i].regs[PCA_REG_PWM2] = pwm_reg;
        m_display[i].regs[PCA_REG_PWM3] = pwm_reg;
        m_display[i].regs[PCA_REG_PWM4] = pwm_reg;
        m_display[i].regs[PCA_REG_PWM5] = pwm_reg;
        m_display[i].regs[PCA_REG_PWM6] = pwm_reg;
        m_display[i].regs[PCA_REG_PWM7] = pwm_reg;
        pca_update_brightness(&m_display[i]);
    }
    
}

void SegmentDisplay::set_value(float value){
    snprintf(m_buf, SCRATCH_BUF_SIZE, "%.3f", value);
    int i = 0; // index for the m_buf
    int disp_i = 0; // index for display sections
    while(disp_i < m_characters){
        char c = m_buf[i];
        bool dp = false;
        if(m_buf[i+1] == '.'){
            dp = true;
            i += 2; // skip over the next character since it's a decimal point
        }else{
            i++;
        }
        int lut_ind = c - 48;
        if(c == '-'){
            lut_ind = 11;
        }else if(lut_ind > 10){
            lut_ind = 10; // set to blank
        }
        set_segment_value(&m_display[disp_i], seven_seg_lut[lut_ind], dp);
        
        disp_i++;
    }
}

void SegmentDisplay::blink(bool enab, float period){
    if(enab){
        uint8_t grpfreq = (uint8_t)(((period * 24) - 1) + 0.5);
        pca_write_reg(&m_display[0], PCA_REG_GRPFREQ, grpfreq);

        uint8_t mode2_reg = pca_read_reg(&m_display[0], PCA_REG_MODE2);
        mode2_reg |= (1 << 5); // set DMBLNK bit
        pca_write_reg(&m_display[0], PCA_REG_MODE2, mode2_reg);
        pca_write_reg(&m_display[0], PCA_REG_GRPPWM, 175); // duty cycle
    }else{
        uint8_t mode2_reg = pca_read_reg(&m_display[0], PCA_REG_MODE2);
        mode2_reg &= ~(1 << 5); // reset DMBLNK bit
        pca_write_reg(&m_display[0], PCA_REG_MODE2, mode2_reg);
    }
}

SegmentDisplay::~SegmentDisplay(){
    free(m_display);
}