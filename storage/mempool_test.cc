#include "./storage/mempool.hpp"

#include <gtest/gtest.h>
#include <string>

using namespace std;
using namespace sbase;

TEST(MemPoolTest, CrowdedNewCall){
	MemPool<int> pool;
	for(int i = 0; i < 100; i++){
		int no = (i);
		char* ret;
		EXPECT_TRUE(pool.New(no, 100, ret).ok());
		EXPECT_TRUE(ret!=nullptr);
		EXPECT_TRUE(pool.pooling(no));
	}		
}

TEST(MemPoolTest, FreeTest){
	MemPool<int> pool;
	for(int i = 0; i<40; i++){
		int no = i;
		char* ret;
		EXPECT_TRUE(pool.New(no, 200, ret).ok());
	}
	for(int i = 39; i >= 0 ; i--){
		int no = i;
		EXPECT_TRUE(pool.Free(no).ok());
		EXPECT_FALSE(pool.pooling(no));
	}
}

TEST(MemPoolTest, WriteReadTest){
	MemPool<int> pool;
	for(int i = 0; i< 20; i++){
		string write  = to_string(i);
		int no = i;
		char* ret;
		EXPECT_TRUE(pool.New(no, 10, ret).ok());
		ret[0] = write[0];
	}
	for(int i = 19; i >=0 ;i--){
		int no = i;
		char* data = pool.get_ptr(no);
		EXPECT_TRUE(data);
		EXPECT_EQ(data[0], to_string(i)[0]);
	}
}