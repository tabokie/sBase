#include <gtest\gtest.h>

#include "frontend_mock.hpp"
#include "slice.hpp"
#include <iostream>

using namespace std;

TEST(SliceTest, SchemaDefinition){
  Schema user_schema;
  Schema runtime_schema;
  Schema blob_schema;
}

TEST(SliceTest, UserIO){ // StringIO

}

TEST(SliceTest, ValueIO){

}

TEST(SliceTest, BlobIO){

}

TEST(FragmentTest, OutputTest){
  FrontMock mocker;
  FieldMeta& field = mocker.get_meta();
  auto fragment0 = field.get_fragment("ID");
  for(int i = 0; i< 10; i++){
    char* data = reinterpret_cast<char*>(&i);
    fragment0.Read(data);
    int ret = -1;
    stringstream Stream;
    Stream << fragment0;
    Stream >> ret;
    EXPECT_EQ(ret, i);  
  }
  auto fragment1 = field.get_fragment("TEXT"); // len 10
  for(int i = 0; i< 10; i++){
    char* c = new char[11];
    for(int j = 0; j<10; j++){
      c[j] = 'a'+i;
    }
    c[10] = '\0';
    string text = c;
    fragment1.Read(c);
    string ret;
    stringstream Stream;
    Stream << fragment1;
    Stream >> ret;
    EXPECT_EQ(ret, text); 
    delete [] c; 
  }
  
  auto fragment2 = field.get_fragment("TEXT"); // len 10
  for(int i = 0; i< 10; i++){
    char* c = new char[11];
    for(int j = 0; j<8; j++){
      c[j] = 'a'+i;
    }
    c[8] = ' ';
    c[9] = ' ';// wrapper
    c[10] = '\0';
    fragment2.Read(c);
    c[8] = '\0';
    string text = c;
    string ret;
    stringstream Stream;
    Stream << fragment2;
    Stream >> ret;
    EXPECT_EQ(ret, text);  
    delete [] c;
  }
}

TEST(FragmentTest, InputTest){
  FrontMock mocker;
  FieldMeta& field = mocker.get_meta();
  auto fragment = field.get_fragment("ID");
  for(int i = 0; i< 10; i++){
    const char* data = reinterpret_cast<char*>(&i);
    // fragment.Read(data);
    stringstream Stream;
    int id = 9;
    Stream << id;
    Stream >> fragment;
    Stream.clear();
    int ret = -1;
    Stream << fragment;
    Stream >> ret;
    EXPECT_EQ(ret, id);  
  }
}

TEST(FragmentTest, OperatorTest){
  FrontMock mocker;
  FieldMeta& field = mocker.get_meta();
  auto fragment = field.get_fragment("ID");
  int i = 9;
  char* data = reinterpret_cast<char*>(&i);
  data >> fragment;
  stringstream Stream;
  int ret = -1;
  Stream << fragment;
  Stream >> ret;
  EXPECT_EQ(ret, i);  
}

TEST(FragmentTest, SliceOperatorTest){
  FrontMock mocker;
  FieldMeta& field = mocker.get_meta();
  auto fragment = field.get_fragment("ID");
  auto slice = field.get_slice();
  
  int i = 9;
  char* id = reinterpret_cast<char*>(&i); // 4
  char* c = new char[11];
  for(int j = 0; j<8; j++){
    c[j] = 'c';
  }
  c[8] = ' ';
  c[9] = ' ';// wrapper
  c[10] = '\0';
  char* full = new char[14];
  memcpy(full, id, 4);
  memcpy(full+4, c, 10);


  full >> slice;
  stringstream Stream;
  int ret = -1;
  string ret_str;
  Stream << slice;
  Stream >> ret;
  Stream >> ret_str ;
  EXPECT_EQ(ret, i);
  full[4+8] = '\0';
  EXPECT_EQ(ret_str, string(full+4));
}