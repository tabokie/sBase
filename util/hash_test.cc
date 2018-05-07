#include <gtest/gtest.h>
#include "./util/hash.hpp"

TEST(HashMapTest, OccupiedSize){
	HashMap<int,int> map;
	for(int i = 0; i < 100; i++){
		ASSERT_TRUE(map.Insert(i, i+1));
		EXPECT_EQ(map.occupied(), i+1);
	}
}

TEST(HashMapTest, AvailableSize){
	HashMap<int, int>map;
	for(int i = 0; i< 100; i++){
		ASSERT_TRUE(map.Insert(i, i+1));
	}
	EXPECT_TRUE(map.size()>=100);
}

TEST(HashMapTest, ElementDelete){
	HashMap<int, int>map;
	for(int i = 0; i<100; i++){
		ASSERT_TRUE(map.Insert(i,i+1));
	}
	ASSERT_EQ(map.occupied(), 100);
	for(int i = 0; i<100; i++){
		EXPECT_TRUE(map.Delete(i));
		EXPECT_EQ(map.occupied(), 99-i);
	}
	EXPECT_EQ(map.occupied(), 0);
}

TEST(HashMapTest, ElementRetrieve){
	HashMap<int , int>map;
	for(int i = 0; i < 100; i++){
		ASSERT_TRUE(map.Insert(i, i*i));
	}
	ASSERT_EQ(map.occupied(),100);
	int res;
	for(int i = 99; i>=0 ; i--){
		EXPECT_TRUE(map.Get(i, res));
		EXPECT_EQ(res, i*i);
	}
	EXPECT_EQ(map.occupied(), 100);
}

TEST(HashMapTest, StringHash){
	HashMap<std::string, int>map;
	for(int i = 0; i<100; i++){
		std::string str = std::to_string(i*7*i);
		ASSERT_TRUE(map.Insert(str, i));
	}
	ASSERT_EQ(map.occupied(), 100);
	int res;
	for(int i = 99; i>=0 ; i--){
		EXPECT_TRUE(map.Get(std::to_string(i*7*i), res));
		EXPECT_EQ(res, i);
	}
	EXPECT_EQ(map.occupied(), 100);
}