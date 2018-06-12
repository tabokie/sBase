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
TEST(EngineTest, CreateDropTable){
	Engine engine;
	ASSERT_TRUE(engine.CreateDatabase("root").ok());
	std::vector<Attribute> attr{Attribute("Key", intT), Attribute("Value", fixchar32T)};
	Schema tmp("firstSchema", attr.begin(), attr.end());
	ASSERT_TRUE(engine.CreateTable(tmp).ok());
	EXPECT_TRUE(engine.OpenCursor("firstSchema", "Key").ok());
	auto slice = tmp.NewObject();
	slice->SetValue(0, Value(intT, new RealValue<int32_t>(7)));
	slice->SetValue(1, Value(fixchar32T, std::string("values")));
	EXPECT_TRUE(engine.Insert(slice).ok());
	std::vector<Slice> ret;
	EXPECT_TRUE(engine.Get(Value(intT, new RealValue<int32_t>(7))),  ret);
	EXPECT_TRUE( static_cast<std::string>(ret[0][1].get<FixChar>()) , std::string("values") );
	EXPECT_TRUE(engine.DropDatabase().ok());
}