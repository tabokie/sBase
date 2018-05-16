#include <gtest/gtest.h>

#include "buzy_vector.hpp"
// #include "util\buzy_vector.hpp"

TEST(BuzyVecTest, BasicVector){
	BuzyVector<int> v;
	v.push_back(0);
	v.push_back(1);
	v.push_back(2);
	EXPECT_EQ(3, v.size());
}