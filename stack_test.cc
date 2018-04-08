#include <gtest\gtest.h>

#include "stack.hpp"

#include <iostream>
using namespace std;


TEST(StackTest, BasicOp){
	Stack<int> stack;
	stack+= 1;
	stack+= 2;
	EXPECT_EQ(2, stack.pop());
}

TEST(StackTest, ArgsInit){
	Stack<string> stack("c","b","a");
	EXPECT_EQ(string("c"),stack.pop());
}

TEST(StackTest, TwoStackAdd){
	Stack<int> stack0(4,3,2,1);
	Stack<int> stack1(6,5);
	stack0 += stack1;
	EXPECT_EQ(6,stack0.pop());
}

