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
	
}