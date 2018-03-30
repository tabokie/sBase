#ifndef SBASE_TEST_FRONT_END_MOCK_HPP_
#define SBASE_TEST_FRONT_END_MOCK_HPP_

#include "storage\mempool.hpp"
#include "slice.hpp"
#include "storage\page_manager.h"

namespace sbase{

class FrontMock{
  FieldMeta meta;
  Type type0_;
  Type type1_;
  PageManager pager_;
  char* pool_;
 public:
  FrontMock(){
    SetupFieldMeta();
    SetupPageManager();
  }
  ~FrontMock(){ }
  void SetupFieldMeta(void){
    type0_.Instantialize<int>("int");
    meta.AddField(string("ID"), type0_);
    type1_.Instantialize<char>("char10",10*sizeof(char));
    meta.AddField(string("TEXT"), type1_);  
    
  }
  void SetupPageManager(void){
    Fragment key_ = meta.get_fragment("ID");

    PageMeta meta(0,0,4096);
    pager_.page_.push_back(meta);
    pager_.pool_.New(0,4096,pool_);
    memset(pool_, 0, 4096);
    pool_[0] = 4;
    int key = 0;
    int match = key_.length();
    int ptr = key_.length()+2;
    int offset = 4+key_.length();
    int cur = 1;
    for(int i = 0; i < pool_[0]; i++){
      pool_[cur + key] = (i);
      pool_[cur + match] = (2*i);
      pool_[cur + ptr] = (2*i+1);
      cur += offset;
    }
  } 
  FieldMeta& get_meta(void){
    return meta;
  }
  PageManager* get_pager(void){
    return &pager_;
  }
  Type* get_type(void){
    return &type0_;
  }
};

}


#endif // SBASE_TEST_FRONT_END_MOCK_HPP_