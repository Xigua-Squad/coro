#pragma once
#include <stdexcept>
class broken_promise : public std::logic_error
{
public:
	broken_promise() 
		:std::logic_error("broken promise") {}
};