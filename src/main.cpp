#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>

#include <json.hpp>


using json = nlohmann::json;

int main(int argc, char* argv[]){

    if(argc < 2){
        printf("Too few arguments provided.\n");
        return -1;
    }

    std::ifstream json_stream(argv[1], std::ios::in);

    json obj;
    json_stream >> obj;

    std::cout << "Some JSON:\n" << obj << std::endl;

    auto ch1_info = obj["channel_1"];
    auto ch2_info = obj["channel_2"];

    

    return 0;
}