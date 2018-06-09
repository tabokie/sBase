#include <gtest/gtest.h>
#include "./util/reflection.hpp"

#include <string>
#include <iostream>
using namespace std;

TEST(ReflectionTest, CreateNewValue){
	auto intv = Value(intT, std::string("9"));
	EXPECT_EQ(intv.get<int>(), 9);
	auto strv = Value(fixchar16T, std::string("test"));
	EXPECT_EQ(strv.get<FixChar>(), string("test"));
}

TEST(ReflectionTest, ComplexConvertion){
	int value = 177;
	char* holder = reinterpret_cast<char*>(&value);
	Blob bob(holder, sizeof(int));
	auto intv = Value(intT, bob);
	EXPECT_EQ(intv.get<int>(), value);
	EXPECT_EQ(string(intv), std::string("177"));
	EXPECT_EQ(Blob(intv), bob);
}

TEST(ReflectionTest, FromRealValue){
	int value = 177;
	auto intv = Value(intT, new RealValue<int>(value));
	EXPECT_EQ(intv.get<int>(), 177);
	std::string str = "199";
	auto strv = Value(fixchar16T, new RealValue<FixChar>( FixChar(16, str) ));
	EXPECT_EQ(strv.get<FixChar>(), str);
}

TEST(ReflectionTest, DefineClass){
	// from iterator
	std::vector<Attribute> attrv;
	attrv.push_back(Attribute("attr0", intT));
	attrv.push_back(Attribute("attr1", fixchar16T));
	attrv.push_back(Attribute("attr2", doubleT));
	ClassDef cdef0(nullptr, "Base", attrv.begin(), attrv.end());
	EXPECT_EQ(cdef0.attributeCount(), 3);
	int index = cdef0.GetAttributeIndex("attr1");
	auto attr1 = cdef0.GetAttribute(index);
	EXPECT_EQ(attr1.name(), string("attr1"));
	EXPECT_EQ(attr1.type(), fixchar16T);
	EXPECT_EQ(cdef0.length(), 4+16+4);
	// from base class
	ClassDef cdef1(&cdef0, "Derived");
	EXPECT_EQ(cdef1.attributeCount(), 3);
	cdef1.AddAttribute(Attribute("attr1", intT));
	EXPECT_EQ(cdef1.attributeCount(), 4);
	index = cdef1.GetAttributeIndex("attr1");
	attr1 = cdef1.GetAttribute(index);
	EXPECT_EQ(attr1.type(), fixchar16T);
	attr1 = cdef1.GetAttribute(4);
	EXPECT_EQ(attr1.type(), intT);
	EXPECT_EQ(cdef1.length(), 4+16+4+4);
}

TEST(ReflectionTest, CreateObj){
	// from iterator
	std::vector<Attribute> attrv;
	attrv.push_back(Attribute("id", intT));
	attrv.push_back(Attribute("name", fixchar16T));
	attrv.push_back(Attribute("salary", doubleT));
	ClassDef cdef(nullptr, "Base", attrv.begin(), attrv.end());
	EXPECT_EQ(cdef.attributeCount(), 3);
	// set one by one
	Object instance0(&cdef);
	instance0.SetValue("id", Value(intT, std::string("9")));
	instance0.SetValue("name", Value(fixchar16T, std::string("txy")));
	instance0.SetValue("salary",Value(doubleT, std::string("57.9")));
	EXPECT_EQ(57.9, instance0.GetValue("salary").get<double>());
	EXPECT_EQ(instance0.length(), 4+16+4);
	// set by list
	Object instance(&cdef, string("7"), string("yxy"), string("48.3"));
	EXPECT_EQ(instance.GetValue("name").get<FixChar>(), string("yxy"));
	EXPECT_EQ(instance.length(), 4+16+4);
}
