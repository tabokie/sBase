#include <gtest\gtest.h>

#include "dict.hpp"
#include <string>

using namespace sbase;

TEST(AutoDictTest, StringAsKey){
  AutoDict<std::string> dict("SELECT", "COLUMN_LIST","FROM","TABLE_LIST","WHERE_CLAUSE","END");
  EXPECT_EQ(dict["FROM"],2);
  EXPECT_EQ(dict(0), string("SELECT"));
}

TEST(DictTest, StringToInt){
  Dict<std::string, int> dict("SELECT",6, "COLUMN_LIST",2,"FROM",1,"TABLE_LIST",9,"WHERE_CLAUSE",3,"END",11);
  EXPECT_EQ(dict["TABLE_LIST"], 9);
  EXPECT_EQ(dict(11),string("END"));
}

TEST(DictTest, StringToString){
  Dict<std::string, std::string> dict("SELECT","a", "COLUMN_LIST","b","FROM","c","TABLE_LIST","d","WHERE_CLAUSE","e","END","f");
  EXPECT_EQ(dict["TABLE_LIST"], string("d"));
  EXPECT_EQ(dict("f"),string("END"));
}