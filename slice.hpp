#ifndef SBASE_STORAGE_SLICE_HPP_
#define SBASE_STORAGE_SLICE_HPP_

#include "storage\status.h"

#include <iostream>
#include <string>
#include <cstring>
#include <functional>
#include <vector>
#include <cassert>

using std::ostream;
using std::istream;
using std::vector;
using std::function;
using std::string;

namespace sbase{

struct Type{
  
  size_t word;
  size_t size;
  string name;
  function<void(ostream&, char*)> output;
  function<void(istream&, char*)> input;
  function<bool(char*, char*)> equal;
  function<bool(char*, char*)> less;
  function<bool(char*, char*)> greater;

  Type() = default;
  ~Type(){ }

  size_t length(void)const{return size * word;}

  template <typename T>
  void Instantialize(const char* n, int len = -1){ // 0 for var word
    name = string(n);
    word = sizeof(T);
    if(len == 0){
      word = 0;
      size = 0;
    }
    else if(len % word == 0){
      size = len / word;
    }
    else size = 1;
    if(size == 1){
      output = [](ostream& os, char* p){
        os << *(reinterpret_cast<T*>(p));
        return;
      };
      input = [](istream& is, char* p){
        is >> *(reinterpret_cast<T*>(p));
        return;
      };
      equal = [](char* a, char* b){
        return (*(reinterpret_cast<T*>(a)) == *(reinterpret_cast<T*>(b)) );
      };
      less = [](char* a, char* b){
        return ( *(reinterpret_cast<T*>(a)) < *(reinterpret_cast<T*>(b)) );
      };
      greater = [](char* a, char* b){
        return ( *(reinterpret_cast<T*>(a)) > *(reinterpret_cast<T*>(b)) );
      };
    }
    else{
      output = [this](ostream& os, char* p){
        for(int i = 0; i<this->size; i++)
          os << *(reinterpret_cast<T*>(p+this->word*i));
        return;
      };
      input = [this](istream& is, char* p){
        for(int i = 0; i<this->size; i++)
          is >> *(reinterpret_cast<T*>(p+this->word*i));
        return;
      };
      equal = [this](char* a, char* b){
        for(int i = 0; i<this->size; i++)
          if( *(reinterpret_cast<T*>(a+this->word*i)) != *(reinterpret_cast<T*>(b+this->word*i)) )return false;
        return true;
      };
      less = [this](char* a, char* b){
        for(int i = 0; i<this->size; i++)
          if ( *(reinterpret_cast<T*>(a+this->word*i)) >= *(reinterpret_cast<T*>(b+this->word*i)) )return false;
        return true;
      };
      greater = [this](char* a, char* b){
        for(int i = 0; i<this->size; i++)
          if ( *(reinterpret_cast<T*>(a+this->word*i)) <= *(reinterpret_cast<T*>(b+this->word*i)) )return false;
        return true;
      };  
    }
    
  }
  bool operator==(const Type& that) const{
    return name == that.name;
  }
};

class Fragment{
  const Type* meta_;
  char* data_;
 public:
  Fragment(const Type& meta):meta_(&meta),data_(nullptr){ }
  Fragment(const Type* meta):meta_(meta),data_(nullptr){ }
  Fragment(const Fragment& that): data_(nullptr){
    meta_ = that.meta_;
  }
  ~Fragment(){ delete [] data_; }
  static Fragment NilFrag(void){
    return Fragment(nullptr);
  }
  inline void Read(const char* data){
    if(!data_)data_ = new char[meta_->length()];
    memcpy(data_, data ,meta_->length());
  }
  inline void Attach(char* data){
    if(data == NULL)data_ = nullptr;
    else data_ = data;
  }
  inline size_t length(void){ if(!meta_)return 0; return meta_->length(); }
  bool operator==(Fragment& that) const{
    if(!(*meta_ == *(that.meta_)))return false;
    return meta_->equal(data_, that.data_);
  }
  // input
  friend istream & operator>>(istream &is, Fragment& frag){
    if(!frag.data_)frag.data_ = new char[frag.meta_->length()];
    frag.meta_->input(is, frag.data_);
  }
  friend char* operator>>(char* input, Fragment& frag){
    frag.Read(input);
    input += frag.meta_->length();
    return input;
  }
  // output
  friend ostream & operator<<(ostream &os, Fragment& frag){
    frag.meta_->output(os, frag.data_);
  }
  friend char* operator<<(char* output, Fragment& frag){
    frag.Read(output);
    output += frag.meta_->length();
    return output;
  }
  
};

class Slice{
  vector<Fragment> frag_;
  vector<string> name_;
 public:
  Slice() = default;
  ~Slice(){ }
  void AddFrag(string name, Fragment frag){
    frag_.push_back(frag);
    name_.push_back(name);
  }
  size_t length(void){
    size_t ret = 0;
    for(auto& frag : frag_){
      ret += frag.length();
    }
    return ret;
  }
  // input
  friend char* operator>>(char* input, Slice& slice){
    assert(input);
    char* cur = input;
    for(auto &frag : slice.frag_){
      cur = (cur >> frag);
    }
    return cur;
  }
  friend istream& operator>>(istream& is, Slice& slice){
    for(auto &frag : slice.frag_){
      is >> frag;
    }
    return is;
  }
  // output
  friend char* operator<<(char* output, Slice& slice){
    assert(output);
    char* cur = output;
    for(auto& frag : slice.frag_){
      cur = (cur << frag);
    }
    return output;
  }
  friend ostream& operator<<(ostream& os, Slice& slice){
    for(auto &frag : slice.frag_){
      os << frag;
    }
    return os;
  }
};

class FieldMeta{
  vector<string> names_;
  vector<Type*> types_;
  vector<bool> primary_;
  vector<bool> not_null_;
  vector<bool> unique_;
  size_t size_;
 public:
  FieldMeta():size_(0){ }
  ~FieldMeta(){ }
  Status AddField(string name, Type& type, 
    bool primary = false, bool null = false, bool unique = false){
    names_.push_back(name);
    types_.push_back(&type);
    primary_.push_back(primary);
    not_null_.push_back(null);
    unique_.push_back(unique);
    size_++;
    return Status::OK();
  }
  Fragment get_fragment(string& name){
    for(int i = 0; i<size_; i++){
      if(names_[i] == name){
        return Fragment(types_[i]);
      }
    }
    return Fragment::NilFrag();
  }
  Fragment get_fragment(const char* name){
    string name_str = string(name);
    for(int i = 0; i<size_; i++){
      if(names_[i] == name_str){
        return Fragment(types_[i]);
      }
    }
    return Fragment::NilFrag();
  }
  Fragment get_fragment(size_t no){
    if(no < 0 || no >= size_)return Fragment::NilFrag();
    return Fragment(types_[no]);
  }
  Slice get_slice(void){
    Slice slice;
    for(int i = 0; i < size_; i++){
      slice.AddFrag(names_[i], get_fragment(i));
    }
    return slice;
  }
};

}



#endif // SBASE_STORAGE_SLICE_HPP_