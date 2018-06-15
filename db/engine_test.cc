#include <gtest\gtest.h>

#include "engine.h"

#include <iostream>

using namespace std;
using namespace sbase;

TEST(EngineTest, CreateDropDatabase){
	Engine engine;
	EXPECT_TRUE(engine.CreateDatabase("root").ok());
	std::vector<Attribute> attr{Attribute("Key", intT), Attribute("Value", fixchar32T)};
	Schema tmp("firstSchema", attr.begin(), attr.end());
	EXPECT_TRUE(engine.CreateTable(tmp).ok());
	EXPECT_TRUE(engine.CloseDatabase().ok());
	EXPECT_TRUE(engine.LoadDatabase("root").ok());
	EXPECT_TRUE(engine.DropDatabase().ok());
}
TEST(EngineTest, FirstInsertQuery){
	Engine engine;
	ASSERT_TRUE(engine.CreateDatabase("root").ok());
	std::vector<Attribute> attr{Attribute("Key", intT), Attribute("Value", fixchar32T)};
	Schema tmp("firstSchema", attr.begin(), attr.end());
	ASSERT_TRUE(engine.CreateTable(tmp).ok());

	engine.Transaction();

	EXPECT_TRUE(engine.OpenCursor("firstSchema").ok());
	EXPECT_TRUE(engine.OpenCursor("firstSchema", "Key").ok());
	auto slice = tmp.NewObject();
	slice->SetValue(0, Value(intT, new RealValue<int32_t>(7)));
	slice->SetValue(1, Value(fixchar32T, std::string("values")));
	Value key = Value(intT, new RealValue<int32_t>(7));

	EXPECT_TRUE(engine.PrepareMatch(&key).ok());
	EXPECT_TRUE(engine.InsertSlice(slice).ok());

	EXPECT_TRUE(engine.PrepareMatch(&key).ok());
	std::vector<Slice> ret;
	SlicePtr tmpSlice = nullptr;
	EXPECT_TRUE(engine.NextSlice(tmpSlice).ok() );
	ASSERT_TRUE(tmpSlice != nullptr);

	EXPECT_EQ( static_cast<std::string>((*tmpSlice)[1].get<FixChar32>()) , std::string("values") );
	EXPECT_TRUE(engine.DropDatabase().ok());
}

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
	slice->SetValue(0, Value(intT, new RealValue<int32_t>(7)));
	slice->SetValue(1, Value(fixchar32T, std::string("values")));
	int size = 35000;

	for(int i = 0; i < size; i++){
		Value key(intT, new RealValue<int32_t>(i));
		slice->SetValue(0, key );
		slice->SetValue(1, Value(fixchar32T, std::string("values")+to_string(i) ) );
		EXPECT_TRUE(engine.PrepareMatch(&key).ok());
		// EXPECT_TRUE(engine.InsertSlice(slice).ok());
		auto status = engine.InsertSlice(slice);
		if(!status.ok())std::cout << status.ToString() << std::endl;
		ASSERT_TRUE(status.ok());
	}


	SlicePtr pSlice;

	for(int i = size-1; i >= 0; i--){
		Value key(intT, new RealValue<int32_t>(i));
		EXPECT_TRUE(engine.PrepareMatch(&key).ok());
		EXPECT_TRUE(engine.NextSlice(pSlice).ok());
		EXPECT_EQ(std::string(pSlice->GetValue(1)) ,std::string("values")+to_string(i));
	}
	EXPECT_TRUE(engine.DropDatabase().ok());
}

TEST(EngineTest, SequenceQueryTest){
	Engine engine;
	ASSERT_TRUE(engine.CreateDatabase("root").ok());
	std::vector<Attribute> attr{Attribute("Key", intT), Attribute("Value", fixchar32T)};
	Schema tmp("firstSchema", attr.begin(), attr.end());
	ASSERT_TRUE(engine.CreateTable(tmp).ok());
	engine.Transaction();
	EXPECT_TRUE(engine.OpenCursor("firstSchema").ok());
	EXPECT_TRUE(engine.OpenCursor("firstSchema", "Key").ok());
	auto slice = tmp.NewObject();
	slice->SetValue(0, Value(intT, new RealValue<int32_t>(7)));
	slice->SetValue(1, Value(fixchar32T, std::string("values")));
	int size = 3500;

	for(int i = 0; i < size; i++){
		Value key(intT, new RealValue<int32_t>(i));
		slice->SetValue(0, key );
		slice->SetValue(1, Value(fixchar32T, std::string("values")+to_string(i) ) );
		EXPECT_TRUE(engine.PrepareMatch(&key).ok());
		// EXPECT_TRUE(engine.InsertSlice(slice).ok());
		auto status = engine.InsertSlice(slice);
		if(!status.ok())std::cout << status.ToString() << std::endl;
		ASSERT_TRUE(status.ok());
	}


	SlicePtr pSlice;
	Value minKey(intT, new RealValue<int32_t>(0));
	EXPECT_TRUE(engine.PrepareSequence(&minKey, nullptr).ok());
	for(int i = 0; i < size; i++){
		EXPECT_TRUE(engine.NextSlice(pSlice).ok());
		EXPECT_EQ(std::string(pSlice->GetValue(1)) ,std::string("values")+to_string(i));
	}
	// Value maxKey(intT, new RealValue<int32_t>(size));
	// EXPECT_TRUE(engine.PrepareSequence(nullptr, &maxKey).ok());
	// for(int i = size-1; i >= 0; i--){
	// 	EXPECT_TRUE(engine.NextSlice(pSlice).ok());
	// 	EXPECT_EQ(std::string(pSlice->GetValue(1)) ,std::string("values")+to_string(i));
	// }
	EXPECT_TRUE(engine.DropDatabase().ok());
}

TEST(EngineTest, CreateNonPrimaryIndex){
	Engine engine;
	ASSERT_TRUE(engine.CreateDatabase("root").ok());
	std::vector<Attribute> attr{Attribute("Key", intT), Attribute("Value", fixchar32T)};
	Schema tmp("firstSchema", attr.begin(), attr.end());
	ASSERT_TRUE(engine.CreateTable(tmp).ok());
	engine.Transaction();
	EXPECT_TRUE(engine.OpenCursor("firstSchema").ok());
	EXPECT_TRUE(engine.OpenCursor("firstSchema", "Key").ok());
	auto slice = tmp.NewObject();
	slice->SetValue(0, Value(intT, new RealValue<int32_t>(7)));
	slice->SetValue(1, Value(fixchar32T, std::string("values")));
	int size = 3500;

	for(int i = 0; i < size; i++){
		Value key(intT, new RealValue<int32_t>(i));
		slice->SetValue(0, key );
		slice->SetValue(1, Value(fixchar32T, std::string("values")+to_string(i) ) );
		EXPECT_TRUE(engine.PrepareMatch(&key).ok());
		// EXPECT_TRUE(engine.InsertSlice(slice).ok());
		auto status = engine.InsertSlice(slice);
		if(!status.ok())std::cout << status.ToString() << std::endl;
		ASSERT_TRUE(status.ok());
	}

	auto status = engine.MakeIndex("firstSchema", "Value");
	if(!status.ok())std::cout << status.ToString() << std::endl;
	EXPECT_TRUE(status.ok());

	EXPECT_TRUE(engine.OpenCursor("firstSchema", "Value").ok());
	SlicePtr pSlice;
	int test = 374;
	Value key(fixchar32T, std::string("values")+to_string(test));
	status = engine.PrepareMatch(&key);
	if(!status.ok())std::cout << status.ToString() << std::endl;
	EXPECT_TRUE(status.ok());
	bool found = false;
	while(true){
		status = engine.NextSlice(pSlice);
		if(!status.ok())cout << status.ToString() << endl;
		EXPECT_TRUE(status.ok());
		if(!pSlice)break;
		if(std::string(pSlice->GetValue(0)) == to_string(test))found = true;
	}
	EXPECT_TRUE(found);

	EXPECT_TRUE(engine.DropDatabase().ok());
}

TEST(EngineTest, DeleteTest){

}