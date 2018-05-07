#ifndef SBASE_UTIL_DICT_HPP_
#define SBASE_UTIL_DICT_HPP_

#include "./util/hash.hpp"

#include <vector>
#include <cassert>

// #include <iostream>
// using std::cout;
// using std::endl;

namespace sbase{

// coded from 1
template <typename T>
class AutoDict{
  size_t size_;
  HashMap<T,int> map_;
  std::vector<T> word_;

 public:
  template <class ...Args>
  AutoDict(Args... args):size_(0),map_(){ 
    size_ = sizeof...(args);
    int arr[] = { ( word_.push_back(args), map_.Insert(args,word_.size()),0)... };
  }
  ~AutoDict(){ }

  template <class ...Args>
  static inline AutoDict<T> MakeAutoDict(Args... args){
    AutoDict<T> d;
    d.size_ = sizeof...(args);
    int arr[] = { ( d.word_.push_back(args), d.map_.Insert(args,d.word_.size()),0)... };
    return d;
  }
  T operator()(size_t idx) const {
    if(idx > size_)return static_cast<T>(0);
    return word_[idx-1];
  }
  int operator[](T name) const {
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
  Dict(Args... args):size_(0),k2d_(sizeof...(args)),d2k_(sizeof...(args)){ 
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

template <typename _KT, typename _DT>
class LayeredDict{
  size_t size_;
  using DataType = _DT;
  using LayerPtr = shared_ptr<std::vector<DataType>>;
  using KeyType = _KT;
  HashMap<KeyType,LayerPtr> k2d_;

  ArgsInit(KeyType key, DataType data){
    size_++;
    LayerPtr slot;
    if(k2d_.Get(key, slot)){
      slot->push_back(data);
    }
    else{
      auto ptr = make_shared<std::vector<DataType>>();
      ptr->push_back(data);
      assert(k2d_.Insert(key,ptr ));
    }
  }
  template <class ...Args>
  ArgsInit(_KT key, _DT data, Args... args){ 
    size_++;
    LayerPtr slot;
    if(k2d_.Get(key, slot)){
      slot->push_back(data);
    }
    else{
      auto ptr = make_shared<std::vector<DataType>>();
      ptr->push_back(data);
      assert(k2d_.Insert(key,ptr ));
    }
    ArgsInit(args...);
  }

 public:
  template <class ...Args>
  LayeredDict(Args... args):size_(0){ 
    ArgsInit(args...);
  }  
  ~LayeredDict(){ }

  // get certain rule
  DataType get_rule(_KT key, int idx){
    LayerPtr ret;
    if(k2d_.Get(key, ret)){
      if(idx <0 || idx >= (*ret).size())return DataType();
      return (*ret)[idx];

    }
    return DataType();
  }

  // get data from key
  std::vector<DataType> operator[](_KT key){
    LayerPtr ret;
    if(k2d_.Get(key, ret)){
      return *ret;
    }
    return std::vector<DataType>();
  }

};

} // namespace sbase

#endif // SBASE_UTIL_DICT_HPP_