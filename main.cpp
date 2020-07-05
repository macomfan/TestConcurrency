#include <cstdlib>
#include <thread>
#include <chrono>
#include "TestQueue.h"
#include "TestKV.h"

/*
 * 
 */
int main(int argc, char** argv) {
    //StartTestKV();
    
    StartTestQueue(40000 * 4, 1, 100, [](const std::string & value) {
        std::string tmp = value;
        std::string str = "a";
        for (int i = 0; i < 10000; i++) {
            str.append("a");
        }
        //std::this_thread::sleep_for(std::chrono::microseconds(1));
    });
    return 0;
}

