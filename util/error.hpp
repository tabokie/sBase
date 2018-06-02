#ifndef SBASE_UTIL_ERROR_HPP_
#define SBASE_UTIL_ERROR_HPP_

#include <iostream>

#define LOG_FUNC()			do{std::cout << __func__ << std::endl;}while(0)
#define LOG(str) 				do{std::cout << (str) << std::endl;}while(0)
#define LOG_VAR(v) 			do{std::cout << #v << (v) << std::endl;}while(0)

class Error{
 public:
	virtual std::string to_string(void){
		return std::string("Unknown Error");
	};
};

class IndexOutofBounds: public Error{
	std::string to_string(void){
		return std::string("Index Out Of Bounds");
	}
};
class MemoryOutofBounds: public Error{ 
	std::string to_string(void){
		return std::string("Memory Out Of Bounds");
	}
};
class InvalidArgument: public Error{
	std::string to_string(void){
		return std::string("Invalid Parameter");
	}
};

#endif // SBASE_UTIL_ERROR_HPP_