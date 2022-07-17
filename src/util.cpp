#include <fstream>
#include <cstring>

#include "util.h"

void gpio_set_direction(std::string& base_path, int dir){
    std::fstream fs;
    std::string dir_path = base_path + "/direction";
    if(!file_exists(dir_path)){
        return;
    }
    fs.open(dir_path, std::ios::out);
    if(dir == 0){
        // input
        fs << "in";
    }else{
        // output
        fs << "out";
    }
}

/**
 * return 0 if mode is in, 1 if mode is out
 */
int gpio_get_direction(std::string& base_path){
    std::fstream fs;
    std::string dir_path = base_path + "/direction";
    if(!file_exists(dir_path)){
        return -1;
    }
    fs.open(dir_path, std::ios::in);
    std::string dir;
    fs >> dir;
    if(dir[0] == 'i' && dir[1] == 'n'){
        return 0;
    }else{
        return 1;
    }
}

void gpio_set(std::string& base_path, bool state){
    std::fstream fs;
    std::string value_path = base_path + "/value";
    if(!file_exists(value_path)){
        return;
    }
    fs.open(value_path, std::ios::out);
    fs << state;
    fs.close();
}