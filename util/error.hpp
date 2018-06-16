#ifndef SBASE_UTIL_ERROR_HPP_
#define SBASE_UTIL_ERROR_HPP_

#include <ctime>
#include <iostream>
#include <fstream>

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

class ErrorLog{
#ifndef ERROR_LOG_PATH
#define ERROR_LOG_PATH 		("elog.txt")
#endif
 public:
	static void Fatal(const char* message){
		time_t rawTime;
		struct tm* timeInfo;
		time(&rawTime);
		timeInfo = localtime(&rawTime);
		std::fstream f(ERROR_LOG_PATH, std::ios::out);
		if(f.bad())return ;
		f << (timeInfo->tm_year + 1900) \
		<< "-" << (timeInfo->tm_mon + 1) \
		<< "-" << (timeInfo->tm_mday) \
		<< " " << (timeInfo->tm_hour) \
		<< ":" << (timeInfo->tm_min) \
		<< "Fatal Message: " << message << std::endl;
		f.close();
	}
};


#endif // SBASE_UTIL_ERROR_HPP_