#pragma once
#include <chrono>
#include <iostream>
class Timer {
public:
	Timer() :start(std::chrono::steady_clock::now()){
	}
	~Timer() {
		auto end = std::chrono::steady_clock::now();
		auto cnt =  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
		std::cout<< "耗时:" << cnt << "ms\n";
	}
private:
	std::chrono::steady_clock::time_point start;
};