#ifndef _UTIL_H_
#define _UTIL_H_

#include <string>
#include <cstring>
#include <sys/stat.h>

typedef void (*set_mux_func)(int); // for setting the chip select mux

inline bool file_exists(const std::string& name){
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

int gpio_get_direction(std::string& base_path);
void gpio_set_direction(std::string& base_path, int dir);
void gpio_set(std::string& base_path, int state);

int encoder_get_counts(std::string& base_path);
int encoder_get_pulse_period(std::string& base_path);
int encoder_get_switch_state(std::string& base_path);

#endif