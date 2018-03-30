#ifndef SBASE_TEST_FRONT_END_MOCK_HPP_
#define SBASE_TEST_FRONT_END_MOCK_HPP_

class FrontMock{
  FieldMeta meta;
  Type type0_;
  Type type1_;
 public:
  FrontMock(){
  	SetupFieldMeta();
  }
  ~FrontMock(){ }
  void SetupFieldMeta(void){
    type0_.Instantialize<int>("int");
    meta.AddField(string("ID"), type0_);
    type1_.Instantialize<char>("char10",10*sizeof(char));
    meta.AddField(string("TEXT"), type1_);  
  }
  FieldMeta& get_meta(void){
    return meta;
  }
};


#endif // SBASE_TEST_FRONT_END_MOCK_HPP_