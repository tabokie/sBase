#ifndef UTIL_RANDOM_HPP_
#define UTIL_RANDOM_HPP_

#include <random>


class Random{
 public:
	// [0..ceiling-1]

	static uint64_t getIntRand(uint64_t ceiling = 100){
		return (uint64_t)(ceiling * ((double)rand()/RAND_MAX));
	}
	static double getDoubleRand(double ceiling = 1){
		return ceiling * ((double)rand()/RAND_MAX);
	}

};

#endif // UTIL_RANDOM_HPP_