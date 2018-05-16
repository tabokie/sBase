#ifndef UTIL_TIMER_HPP_
#define UTIL_TIMER_HPP_

#include <chrono>
#include <cstdint>


class Time{
 public:
	using TimeType = uint64_t;

	inline static uint64_t Now(void){
		std::chrono::time_point<std::chrono::system_clock,std::chrono::milliseconds> tp = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());  
	    auto tmp=std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());  
	    std::time_t timestamp = tmp.count();  
	    //std::time_t timestamp = std::chrono::system_clock::to_time_t(tp);  
	    return static_cast<uint64_t>(timestamp);  
	}
};




#endif // UTIL_TIMER_HPP_