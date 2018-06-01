#ifndef SBASE_UTIL_ERROR_HPP_
#define SBASE_UTIL_ERROR_HPP_

#include <iostream>

#define LOG_FUNC()			do{std::cout << __func__ << std::endl;}while(0)
#define LOG(str) 				do{std::cout << (str) << std::endl;}while(0)

#endif // SBASE_UTIL_ERROR_HPP_