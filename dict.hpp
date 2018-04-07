#ifndef SBASE_INTERPRETER_DICT_HPP_
#define SBASE_INTERPRETER_DICT_HPP_

#include "./storage/hash.hpp"

#include <vector>
#include <cassert>

namespace sbase{

template <typename T>
class AutoDict{
  size_t size_;
  HashMap<T,int> map_;
  std::vector<T> word_;

 public:
  template <class ...Args>
  AutoDict(Args... args):size_(0){ 
    size_ = sizeof...(args);
    int arr[] = { ( word_.push_back(args), map_.Insert(args,word_.size()-1),0)... };

  }
  ~AutoDict(){ }

  template <class ...Args>
  static inline AutoDict<T> MakeAutoDict(Args... args){
    AutoDict<T> d;
    d.size_ = sizeof...(args);
    int arr[] = { ( d.word_.push_back(args), d.map_.Insert(args,d.word_.size()-1),0)... };
    return d;
  }
  T operator()(size_t idx){
    if(idx >= size_)return static_cast<T>(0);
    return word_[idx];
  }
  int operator[](T name){
    int ret;
    if(map_.Get(name, ret)){
      return ret;
    }
    return -1;
  }
};

template <typename _KT, typename _DT>
class Dict{
  size_t size_;
  HashMap<_KT,_DT> k2d_;
  HashMap<_DT, _KT> d2k_;

  ArgsInit(_KT key, _DT data){
    size_++;
    assert(k2d_.Insert(key,data));
    assert(d2k_.Insert(data,key));
  }
  template <class ...Args>
  ArgsInit(_KT key, _DT data, Args... args){ 
    size_++;
    assert(k2d_.Insert(key,data));
    assert(d2k_.Insert(data,key));
    ArgsInit(args...);
  }

 public:
  template <class ...Args>
  Dict(Args... args):size_(0){ 
    ArgsInit(args...);
  }  
  ~Dict(){ }

  // get data from key
  _DT operator[](_KT key){
    _DT ret;
    if(k2d_.Get(key, ret)){
      return ret;
    }
    return static_cast<_DT>(0);
  }
  // get key from data
  _KT operator()(_DT data){
    _KT ret;
    if(d2k_.Get(data, ret)){
      return ret;
    }
    return static_cast<_KT>(0);
  }
};



} // namespace sbase

#endif // SBASE_INTERPRETER_DICT_HPP_