#include <fstream>
#include <cstring>
#include <string>
#include <iostream>
#include <cstdio>

#include "util.h"


///////////////////////////////////////////////////////////////////////////////
// GPIO UTILITIES
///////////////////////////////////////////////////////////////////////////////
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
    fs.close();
}

/**
 * return 0 if mode is in, 1 if mode is out, -1 on error
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
    fs.close();
    if(dir[0] == 'i' && dir[1] == 'n'){
        return 0;
    }else if(dir[0] == 'o' && dir[1] == 'u' && dir[2] == 't'){
        return 1;
    }else{
        return -1;
    }
}

void gpio_set(std::string& base_path, int state){
    std::fstream fs;
    std::string value_path = base_path + "/value";
    if(!file_exists(value_path)){
        return;
    }
    fs.open(value_path, std::ios::out);
    fs << state;
    fs.close();
}

///////////////////////////////////////////////////////////////////////////////
// ENCODER UTILITIES
///////////////////////////////////////////////////////////////////////////////
int encoder_get_counts(std::string& base_path){
    int retval = 0;
    std::string full_path = base_path + "/counts";
    if(!file_exists(full_path)){
        std::cout << "File " << full_path << " does not exist." << std::endl;
        return retval;
    }
    std::fstream fs;
    fs.open(full_path, std::ios::in);
    fs >> retval;
    fs.close();
    return retval;
}
int encoder_get_pulse_period(std::string& base_path){
    int retval = 0;
    std::string full_path = base_path + "/pulse_period";
    if(!file_exists(full_path)){
        std::cout << "File " << full_path << " does not exist." << std::endl;
        return retval;
    }
    std::fstream fs;
    fs.open(full_path, std::ios::in);
    fs >> retval;
    fs.close();
    return retval;
}
int encoder_get_switch_state(std::string& base_path){
    int retval = 0;
    std::string full_path = base_path + "/switch_state";
    if(!file_exists(full_path)){
        std::cout << "File " << full_path << " does not exist." << std::endl;
        return retval;
    }
    std::fstream fs;
    fs.open(full_path, std::ios::in);
    fs >> retval;
    fs.close();
    return retval;
}