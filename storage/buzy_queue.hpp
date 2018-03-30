#ifndef SBASE_STORAGE_BUZY_QUEUE_HPP_
#define SBASE_STORAGE_BUZY_QUEUE_HPP_


#include "hash.hpp"
#include "status.h"
#include <memory>
#include <iostream>

namespace sbase{

static size_t kBuzyQueueSize = 10; // link

template <class _ET>
class BuzyQueue{

  using ElementType = _ET;

  size_t size_;
  size_t cur_;
  ElementType* deque_;
  size_t head_;
  size_t tail_;

 public:

  BuzyQueue(size_t initsize = kBuzyQueueSize):size_(initsize),cur_(0){
    deque_ = new ElementType[size_];
    head_ = 0;
    tail_ = size_-1;
  }
  ~BuzyQueue(){ }
  Status Visit(ElementType idx){
    // Plot();
    if(cur_<=0){
      head_ = 0;
      tail_ = 0;
      deque_[0] = idx;
      cur_ = 1;
      return Status::OK();
    }
    size_t tail = (tail_ >= head_) ? tail_ : tail_+size_;
    for(size_t cur = head_; cur <= tail; cur++){
      if(deque_[cur%size_] == idx){
        deque_[cur%size_] = static_cast<_ET>(1); // delete
        head_ = (head_ < 1) ? size_-1 : head_-1;
        deque_[head_] = idx;
        cur_++;
        if(cur_ >= size_)tail_ = (tail_ < 1) ? size_-1 : tail_-1;
        return Status::OK();
      }
    }
    head_ = (head_ < 1) ? size_-1 : head_-1;
    deque_[head_] = idx;
    cur_++;
    if(cur_ >= size_)tail_ = (tail_ < 1) ? size_-1 : tail_-1;
    return Status::OK();
  }
  Status Last(ElementType& ret){ // delete on get
    while(deque_[tail_] == static_cast<_ET>(1) && cur_ > 0 ){
      tail_ = (tail_ < 1) ? size_-1 : tail_-1;
      cur_--;
    }
    if(cur_ <= 0)return Status::Corruption("Empty Buzy Queue");
    ret = deque_[tail_];
    tail_ = (tail_ < 1) ? size_-1 : tail_-1;
    return Status::OK();
  }
  // for debug
  Status First(ElementType& ret){
    if(cur_<=0)return Status::Corruption("Empty Buzy Queue");
    ret = deque_[head_];
    return Status::OK();
  }
  void Plot(void){
    int tail = (tail_ >= head_) ? tail_ : tail_+size_;
    for(int i = head_; i <= tail; i++){
      std::cout << '[' << deque_[i%size_] <<']';
    }
    std::cout << std::endl << std::flush;
  }

};

}

#endif // SBASE_STORAGE_BUZY_QUEUE_HPP_
