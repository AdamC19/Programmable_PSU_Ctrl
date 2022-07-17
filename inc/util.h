#ifndef _UTIL_H_
#define _UTIL_H_

#include <cstring>
#include <sys/stat.h>


void gpio_set_direction(std::string& base_path, int dir)
void gpio_set(std::string& base_path, bool state);

inline bool file_exists(const std::string& name){
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

#endif