#ifndef _SEGMENT_DISPLAY_H_
#define _SEGMENT_DISPLAY_H_

#include <cstdint>
#include <mutex>
#include <string>
#include <cstring>
#include "pca9634.h"


#define SCRATCH_BUF_SIZE 16
#define DFLT_BRIGHTNESS  127

class SegmentDisplay {
private:
    int m_characters;
    pca9634_chip_t* m_display;
    char m_buf[SCRATCH_BUF_SIZE];
    std::string m_oe_gpio;
public:
    SegmentDisplay(int n_characters, uint8_t* addresses, int i2c_fd, std::string oe_gpio);
    void set_value(float value);
    void set_brightness(float duty);
    ~SegmentDisplay();
    void output_disable();
    void output_enable();
    void blink(bool enab, float period);
};



#endif