#include <gtest\gtest.h>

#include ".\db\engine.h"

#include <iostream>

using namespace std;
using namespace sbase;

#include <iostream>
#include <ctime> /* clock_t, clock() */
#include <windows.h>
#include <chrono>

class Timer{
	// LARGE_INTEGER start_;
	// LARGE_INTEGER end_;
	// LARGE_INTEGER freq_;
	std::chrono::time_point<std::chrono::high_resolution_clock> start;
	std::chrono::time_point<std::chrono::high_resolution_clock> end;
 public:
 	Timer(){
 		// QueryPerformanceFrequency(&freq_);
 	}
 	~Timer(){ }
 	void Start(void){
  	start = std::chrono::high_resolution_clock::now();
 		// QueryPerformanceCounter(&start_);
 	}
 	double End(void){
  	end = std::chrono::high_resolution_clock::now();
  	std::chrono::duration<double> elapsed = end - start;
  	return elapsed.count();
 		// QueryPerformanceCounter(&end_);
 		// return ((double)end_.QuadPart-(double)start_.QuadPart)/(double)freq_.QuadPart * 1000; // ms
 	}
};

TEST(EngineTest, InsertQueryPressureTest){
	Engine engine;
	ASSERT_TRUE(engine.CreateDatabase("root").ok());
	std::vector<Attribute> attr{Attribute("Key", intT), Attribute("Value", fixchar32T)};
	Schema tmp("firstSchema", attr.begin(), attr.end());
	ASSERT_TRUE(engine.CreateTable(tmp).ok());
	engine.Transaction();
	EXPECT_TRUE(engine.OpenCursor("firstSchema").ok());
	EXPECT_TRUE(engine.OpenCursor("firstSchema", "Key").ok());
	auto slice = tmp.NewObject();
	slice.SetValue(0, Value(intT, new RealValue<int32_t>(7)));
	slice.SetValue(1, Value(fixchar32T, std::string("values")));
	int size = 200000; // 3,5,8,10,20
	Timer timer;

	timer.Start();
	for(int i = 0; i < size; i++){
		Value key(intT, new RealValue<int32_t>(i));
		slice.SetValue(0, key );
		slice.SetValue(1, Value(fixchar32T, std::string("values")+to_string(i) ) );
		EXPECT_TRUE(engine.PrepareMatch(&key).ok());
		// EXPECT_TRUE(engine.InsertSlice(slice).ok());
		auto status = engine.InsertSlice(&slice);
		if(!status.ok())std::cout << status.ToString() << std::endl;
		ASSERT_TRUE(status.ok());
	}
	std::cout << timer.End() << std::endl;


	SharedSlicePtr pSlice;

	timer.Start();
	for(int i = size-1; i >= 0; i--){
		Value key(intT, new RealValue<int32_t>(i));
		EXPECT_TRUE(engine.PrepareMatch(&key).ok());
		EXPECT_TRUE(engine.NextSlice(pSlice).ok());
		EXPECT_EQ(std::string(pSlice->GetValue(1)) ,std::string("values")+to_string(i));
	}
	std::cout << timer.End() << std::endl;

	EXPECT_TRUE(engine.DropDatabase().ok());
}
