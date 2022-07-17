#ifndef _SEGMENT_DISPLAY_H_
#define _SEGMENT_DISPLAY_H_

#include <cstdint>
#include <mutex>
#include "pca9634.h"

typedef struct segment_display {
    int n_characters;
    pca9634_chip_t* display;
}segment_display_t;

void init_display(segment_display_t* disp, int n_characters, uint8_t* addresses, int i2c_fd);


#endif