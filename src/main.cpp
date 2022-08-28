#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <json.hpp>
#include "util.h"
#include "ads1120.h"
#include "max5541.h"
#include "segment_display.h"

using json = nlohmann::json;

#define STATE_SETTING_VOLTAGE   0
#define STATE_SETTING_CURRENT   1
#define STATE_OUTPUT_ON         2

#define MODE_SETUP              0
#define MODE_NORMAL             1
#define MODE_TEST               2
///////////////////////////////////////////////////////////////////////////////
// TYPES
///////////////////////////////////////////////////////////////////////////////

typedef struct channel_struct {
    ADS1120* adc;
    MAX5541* vdac;
    MAX5541* idac;
    std::string enable_gpio;
    std::string v_encoder;
    bool last_venc_state;
    int venc_counts;
    std::string i_encoder;
    bool last_ienc_state;
    int ienc_counts;
    bool setting_i;
    float i_set_value;
    float i_reading;
    uint8_t adc_mux_setting;
    bool setting_v;
    float v_set_value;
    float v_reading;
    SegmentDisplay* vdisp;
    SegmentDisplay* idisp;
}channel_t;

typedef struct state_data_struct {
    std::mutex* state_mtx;
    bool done;
    channel_t channels[2];
    std::string state_path;
}state_t;

///////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
///////////////////////////////////////////////////////////////////////////////
std::string cs_mux_gpio_a;
std::string cs_mux_gpio_b;
std::string cs_mux_gpio_c;
uint8_t adc_config[4] = {ADS1120_PGA_BYPASS, ADS1120_DR_4 | ADS1120_MODE_NORMAL | ADS1120_CM_CONT, ADS1120_VREF_EXT, 0x00};
uint8_t ch1_vdisp_addrs[] = {0x01, 0x02, 0x43, 0x44};
uint8_t ch1_idisp_addrs[] = {0x45, 0x47, 0x08, 0x09};
uint8_t ch2_vdisp_addrs[] = {0x0A, 0x0B, 0x0C, 0x0D};
uint8_t ch2_idisp_addrs[] = {0x0E, 0x0F, 0x10, 0x11};

///////////////////////////////////////////////////////////////////////////////
// FUNCTION DEFINITIONS
///////////////////////////////////////////////////////////////////////////////
void set_cs_mux(int setting){
    int a = setting & 1;
    int b = (setting >> 1) & 1;
    int c = (setting >> 2) & 1;

    gpio_set(cs_mux_gpio_a, a);
    gpio_set(cs_mux_gpio_b, b);
    gpio_set(cs_mux_gpio_c, c);

    std::this_thread::sleep_for(std::chrono::microseconds(10));
}


void save_state_to_json(state_t& state, std::string path){
    if(path[0] != '/'){
        return;
    }
    json obj;
    
    channel_t* ch = &state.channels[0];
    obj["channel_1"]["i_set_value"] = ch->i_set_value;
    obj["channel_1"]["v_set_value"] = ch->v_set_value;
    
    ch = &state.channels[1];
    obj["channel_2"]["i_set_value"] = ch->i_set_value;
    obj["channel_2"]["v_set_value"] = ch->v_set_value;

    std::ofstream f(path, std::ios::out);
    f << obj.dump(4);
}

void load_state_from_json(state_t& state, std::string path){
    if(path[0] != '/'){
        return;
    }
    std::ifstream f(path, std::ios::in);
    try{
        json obj = json::parse(f);
        state.channels[0].i_set_value = obj["channel_1"]["i_set_value"].get<float>();
        state.channels[0].v_set_value = obj["channel_1"]["v_set_value"].get<float>();

        state.channels[1].i_set_value = obj["channel_2"]["i_set_value"].get<float>();
        state.channels[1].v_set_value = obj["channel_2"]["v_set_value"].get<float>();
    }catch(json::exception& e){
        std::cout << "JSON error: " << e.what() << std::endl;
    }
}


void display_updater(state_t& state){
    uint8_t mode = MODE_SETUP;
    while(!state.done){
        state.state_mtx->lock();
        
        switch(mode){
            case MODE_SETUP:{

                std::cout << "Starting displays ..." << std::endl;
                for(int i = 0; i < 2; i++){
                    channel_t* ch = &state.channels[i];
                    ch->vdisp->set_value(0.0);
                    ch->vdisp->set_brightness(1.0);
                    ch->vdisp->output_enable();
                    ch->idisp->set_value(0.0);
                    ch->idisp->set_brightness(1.0);
                    ch->idisp->output_enable();
                }
                mode = MODE_NORMAL;
                break;
            }
            case MODE_NORMAL:{
                for(int i = 0; i < 2; i++){
                    channel_t* ch = &state.channels[i];
                    if(ch->setting_i){
                        // blink I display
                        ch->idisp->set_value(ch->i_set_value);
                        ch->idisp->blink(true, 0.5);
                    }else{
                        // steady on
                        ch->idisp->blink(false, 0.0);
                        // print value from adc
                        ch->idisp->set_value(ch->i_reading);
                    }
                    if(ch->setting_v){
                        // blink V display
                        ch->vdisp->set_value(ch->v_set_value);
                        ch->vdisp->blink(true, 0.5);
                    }else{
                        // steady on
                        ch->vdisp->blink(false, 0.0);
                        // print value from adc
                        ch->vdisp->set_value(ch->v_reading);
                    }
                }
                break;
            }
        }

        state.state_mtx->unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
}

void adc_dac_updater(state_t& state){
    uint8_t mode = MODE_SETUP;
    while(!state.done){
        state.state_mtx->lock();
        
        switch(mode){
            case MODE_SETUP:{
                std::cout << "Starting ADCs ..." << std::endl;
                for(int i = 0; i < 2; i++){
                    channel_t* ch = &state.channels[i];
                    ch->adc->begin(adc_config);
                    ch->adc_mux_setting = ADS1120_MUX_P0_N1;
                    ch->adc->set_input_mux(ch->adc_mux_setting);
                    ch->adc->start_sync();
                    ch->vdac->set_voltage(0.0);
                    ch->idac->set_voltage(1.0);
                }
                mode = MODE_NORMAL;
                break;
            }
            case MODE_NORMAL:{
                for(int i = 0; i < 2; i++){
                    channel_t* ch = &state.channels[i];
                    if(ch->adc_mux_setting == ADS1120_MUX_P0_N1){
                        ch->v_reading = 21.0*ch->adc->get_voltage();
                        ch->adc_mux_setting = ADS1120_MUX_P2_N3;
                    }else{
                        ch->i_reading = 4.0*ch->adc->get_voltage();
                        ch->adc_mux_setting = ADS1120_MUX_P0_N1;
                    }
                    ch->adc->set_input_mux(ch->adc_mux_setting);

                    /* SET DACS */
                    ch->vdac->set_voltage(ch->v_set_value/5.0);
                    ch->idac->set_voltage(ch->i_set_value);
                }
                break;
            }
        }

        state.state_mtx->unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

void encoder_updater(state_t& state){
    state.state_mtx->lock();
    for(int i = 0; i < 2; i++){
        channel_t* ch = &state.channels[i];
        gpio_set_direction(ch->enable_gpio, 1);
        ch->venc_counts = encoder_get_counts(ch->v_encoder);
        ch->ienc_counts = encoder_get_counts(ch->i_encoder);
        ch->last_ienc_state = encoder_get_switch_state(ch->i_encoder);
        ch->last_venc_state = encoder_get_switch_state(ch->v_encoder);
        ch->setting_i = false;
        ch->setting_v = false;
    }
    state.state_mtx->unlock();
    
    while(!state.done){
        state.state_mtx->lock();
        bool save_state = false;
        for(int i = 0; i < 2; i++){
            channel_t* ch = &state.channels[i];
            /* encoder switches */
            bool i_enc_state = encoder_get_switch_state(ch->i_encoder);
            if(i_enc_state && !ch->last_ienc_state){
                // printf("Toggling ch %d setting_i...\r\n", i+1);
                ch->setting_i = !ch->setting_i;
                save_state = true;
            }
            ch->last_ienc_state = i_enc_state;

            bool v_enc_state = encoder_get_switch_state(ch->v_encoder);
            if(v_enc_state && !ch->last_venc_state){
                // printf("Toggling ch %d setting_v...\r\n", i+1);
                ch->setting_v = !ch->setting_v;
                save_state = true;
            }
            ch->last_venc_state = v_enc_state;

            /* if we're setting either value, disable channel */
            if(ch->setting_i || ch->setting_v){
                gpio_set(ch->enable_gpio, 0);
            }else{
                gpio_set(ch->enable_gpio, 1);
            }

            /* encoder counts */
            int venc_counts_new = encoder_get_counts(ch->v_encoder);
            float venc_period = encoder_get_pulse_period(ch->v_encoder)/1.0e6;
            if(venc_counts_new != ch->venc_counts){
                int delta_counts = ch->venc_counts - venc_counts_new;
                ch->v_set_value += 1.0 * delta_counts / venc_period;
                if(ch->v_set_value < 0.0){
                    ch->v_set_value = 0.0;
                }
                save_state = true;
            }
            ch->venc_counts = venc_counts_new;


            int ienc_counts_new = encoder_get_counts(ch->i_encoder);
            float ienc_period = encoder_get_pulse_period(ch->i_encoder)/1.0e6;
            if(ienc_counts_new != ch->ienc_counts){
                int delta_counts = ch->ienc_counts - ienc_counts_new;
                ch->i_set_value += 1.0 * delta_counts / ienc_period;
                if(ch->i_set_value < 0.0){
                    ch->i_set_value = 0.0;
                }
                save_state = true;
            }
            ch->ienc_counts = ienc_counts_new;
        }
        
        if(save_state){
            save_state_to_json(state, state.state_path);
        }
        state.state_mtx->unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}


///////////////////////////////////////////////////////////////////////////////
// MAIN
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]){

    if(argc < 2){
        printf("Too few arguments provided.\n");
        return -1;
    }

    /* read our config.json or whatever path was passed in by the user */
    // printf(argv[1]);
    std::ifstream json_stream(argv[1], std::ios::in);

    json obj = json::parse(json_stream);

    std::string spi_path = obj["spi_path"].get<std::string>();
    std::string i2c_path = obj["i2c_path"].get<std::string>();
    cs_mux_gpio_a = obj["mux_a_gpio"].get<std::string>();
    cs_mux_gpio_b = obj["mux_b_gpio"].get<std::string>();
    cs_mux_gpio_c = obj["mux_c_gpio"].get<std::string>();
    gpio_set_direction(cs_mux_gpio_a, 1);
    gpio_set_direction(cs_mux_gpio_b, 1);
    gpio_set_direction(cs_mux_gpio_c, 1);

    auto ch1_info = obj["channel_1"];
    auto ch2_info = obj["channel_2"];
    
    /* Open devices */
    int spi_fd = open(spi_path.c_str(), O_RDWR);
    if(spi_fd < 0){
        printf("Error opening SPI bus at %s\r\n", spi_path.c_str());
        return -1;
    }

    int i2c_fd = open(i2c_path.c_str(), O_RDWR);
    if(i2c_fd < 0){
        printf("Error opening I2C bus at %s\r\n", i2c_path.c_str());
        return -2;
    }

    /* display system reset */
    std::cout << "Triggering a soft reset of all display drivers..." << std::endl;
    pca_soft_reset(i2c_fd);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    /* === setup STATE object === */
    state_t state;
    state.done = false;
    state.state_mtx = new std::mutex(); // create mutex

    if(argc > 2){
        // load state info from file
        state.state_path = std::string(argv[2]);
        load_state_from_json(state, state.state_path);
    }else{
        state.state_path = "none";
    }
    
    state.channels[0].adc = new ADS1120(spi_fd, ch1_info["adc_mux"].get<int>(), set_cs_mux, 2.600);
    state.channels[0].vdac = new MAX5541(spi_fd, ch1_info["vdac_mux"].get<int>(), set_cs_mux, 2.600);
    state.channels[0].idac = new MAX5541(spi_fd, ch1_info["idac_mux"].get<int>(), set_cs_mux, 2.600);
    state.channels[0].v_encoder = ch1_info["voltage_encoder"].get<std::string>();
    state.channels[0].i_encoder = ch1_info["current_encoder"].get<std::string>();
    std::cout << "Initializing Channel 1 Voltage display..." << std::endl;
    state.channels[0].vdisp = new SegmentDisplay(4, ch1_vdisp_addrs, i2c_fd, ch1_info["v_disp_oe_gpio"].get<std::string>());
    std::cout << "Initializing Channel 1 Current display..." << std::endl;
    state.channels[0].idisp = new SegmentDisplay(4, ch1_idisp_addrs, i2c_fd, ch1_info["i_disp_oe_gpio"].get<std::string>());;
    state.channels[0].enable_gpio = ch1_info["enable_gpio"].get<std::string>();

    state.channels[1].adc = new ADS1120(spi_fd, ch2_info["adc_mux"].get<int>(), set_cs_mux, 2.600);
    state.channels[1].vdac = new MAX5541(spi_fd, ch2_info["vdac_mux"].get<int>(), set_cs_mux, 2.600);
    state.channels[1].idac = new MAX5541(spi_fd, ch2_info["idac_mux"].get<int>(), set_cs_mux, 2.600);
    state.channels[1].v_encoder = ch2_info["voltage_encoder"].get<std::string>();
    state.channels[1].i_encoder = ch2_info["current_encoder"].get<std::string>();
    std::cout << "Initializing Channel 2 Voltage display..." << std::endl;
    state.channels[1].vdisp = new SegmentDisplay(4, ch2_vdisp_addrs, i2c_fd, ch2_info["v_disp_oe_gpio"].get<std::string>());
    std::cout << "Initializing Channel 2 Current display..." << std::endl;
    state.channels[1].idisp = new SegmentDisplay(4, ch2_idisp_addrs, i2c_fd, ch2_info["i_disp_oe_gpio"].get<std::string>());
    state.channels[1].enable_gpio = ch2_info["enable_gpio"].get<std::string>();

    /* === Create threads === */
    std::thread display_thread(display_updater, std::ref(state));
    std::thread encoder_thread(encoder_updater, std::ref(state));
    std::thread adc_dac_thread(adc_dac_updater, std::ref(state));

    display_thread.join();
    encoder_thread.join();
    adc_dac_thread.join();


    return 0;
}