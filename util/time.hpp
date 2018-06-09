#ifndef UTIL_TIME_HPP_
#define UTIL_TIME_HPP_

#include <chrono>
#include <cstdint>


using TimeType = uint64_t;
using MinTimeType = uint8_t;

class Time{
 public:
	inline static uint64_t Now(void){
		::std::chrono::time_point<::std::chrono::system_clock,::std::chrono::milliseconds> tp = ::std::chrono::time_point_cast<std::chrono::milliseconds>(::std::chrono::system_clock::now());  
	    auto tmp=::std::chrono::duration_cast<::std::chrono::milliseconds>(tp.time_since_epoch());  
	    time_t timestamp = tmp.count();  
	    //std::time_t timestamp = std::chrono::system_clock::to_time_t(tp);  
	    return static_cast<uint64_t>(timestamp);  
	}

	inline static uint8_t Now_min(void){
		return static_cast<uint8_t>(Time::Now()>>4 & 0xff);
	}
};







#endif // UTIL_TIME_HPP_