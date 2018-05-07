#include <gtest\gtest.h>

#include "scanner.hpp"
#include "./storage/status.hpp"

#include <string>
using namespace std;
using namespace sbase;

TEST(ScannerTest, ClassicSample){
	Scanner scanner;
	string str = "select * from A;";
	vector<Token> v;
	scanner.Scan(str,v);
	EXPECT_EQ(v[0].id, kWordDict["SELECT"]);
	EXPECT_EQ(v[1].id, kWordDict["MULTIPLY_OP"]);
	EXPECT_EQ(v[2].id, kWordDict["FROM"]);
	EXPECT_EQ(v[3].id, kWordDict["NAME"]);
}

TEST(ScannerTest, LargeSentenceTest){
	Scanner scanner;
	string str = "select A.column,B.column from A,B where B.a = A.b and A.a in (select * from C where C.c > 90);";
	str = str + str;
	str = str + str;
	vector<Token> v;
	scanner.Scan(str,v);
	EXPECT_EQ(v[v.size()-1].id, kWordDict["SEMICOLON"]);
}