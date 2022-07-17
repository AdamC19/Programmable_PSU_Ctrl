
#include <cstdlib>
#include <cstdio>

#include "segment_display.h"

void init_display(segment_display_t* disp, int n_characters, uint8_t* addresses, int i2c_fd){
    disp->n_characters = n_characters;
    disp->display = (pca9634_chip_t*)malloc(sizeof(pca9634_chip_t) * n_characters);
    for(int i = 0; i < n_characters; i++){
        disp->display[i].addr = addresses[i];
        disp->display[i].i2c_fd = i2c_fd;
        pca_read_all_regs(&disp->display[i]);
    }
}