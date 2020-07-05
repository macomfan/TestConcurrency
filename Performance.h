#pragma once

#include <chrono>
#include <iostream>

class Time{
public:
    using TIME_POINT = std::chrono::time_point<std::chrono::system_clock,std::chrono::nanoseconds>;
    
    void start() {
        start_ = std::chrono::system_clock::now();
    }
    
    void end(const std::string& title) {
        TIME_POINT end = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_);
        std::cout << title << " spend time(ms): "<< duration.count() << std::endl;
    }
    
private:
    TIME_POINT start_;
};

