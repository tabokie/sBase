#include <gtest/gtest.h>
#include "./util/buzy_queue.hpp"

using namespace sbase;

TEST(BuzyQueueTest, VisitTest){
	int ret;
	BuzyQueue<int> queue;
	for(int i = 0; i<100; i++){
		int item = i;
		EXPECT_TRUE(queue.Visit(item).ok());
		EXPECT_TRUE(queue.First(ret).ok());
		EXPECT_EQ(ret , item);
	}	
}

TEST(BuzyQueueTest, LastTest){
	int ret;
	BuzyQueue<int> queue;
	for(int i = 0; i<10; i++){
		int item = i;
		EXPECT_TRUE(queue.Visit(item).ok());
	}
	for(int i = 1; i < 10 ;i++){
		EXPECT_TRUE( queue.Last(ret).ok());
		EXPECT_EQ(i, ret);
	}
}