#ifndef SBASE_STACK_HPP_
#define SBASE_STACK_HPP_

#include <cstddef>
#include <cassert>
#include <iostream>


template <typename _ET>
class Stack{
  using ElementType = _ET;
  static const size_t kDefaultStackSize = 15;
  size_t size_;
  size_t top_; // size
  ElementType* stack_;
 public:
  Stack(size_t size = kDefaultStackSize):size_(size), top_(0){ 
    stack_ = new ElementType[size_];
  }
  template <class ...Args>
  Stack(Args... args):size_(sizeof...(args)){
    stack_ = new ElementType[size_];
    top_ = size_;
    InitWithArgs(args...);
  }
  ~Stack(){delete [] stack_;}

  Stack& operator+=(ElementType new_element){
    push(new_element);
    return *this;
  }
  Stack& operator+=(const Stack& that){
    size_t new_size = top_ + that.top_;
    while(new_size > size_)
      assert(Expand());
    for(int i = top_; i < new_size; i++){
      stack_[i] = that.stack_[i-top_];
    }
    top_ = new_size;
    return *this;
  }
  inline ElementType push(ElementType element){
    if(top_+1 > size_)Expand();
    stack_[top_++] = element;
  }
  inline ElementType pop(void){
    if(top_<=0)return static_cast<ElementType>(0);
    return stack_[--top_];
  }
  // Accessors //
  ElementType first(void){
    if(top_ <= 0) return static_cast<ElementType>(0);
    return stack_[top_-1];
  }

 private:
  template <class ...Args>
  void InitWithArgs(ElementType head, Args... rest){
    stack_[--top_] = head;
    InitWithArgs(rest...);
  }
  void InitWithArgs(ElementType tail){
    assert(top_ == 1);
    stack_[--top_] = tail;
    top_ = size_;
    return ;
  }
  bool Expand(void){
    ElementType* new_stack = new ElementType[size_*2];
    for(int i = 0; i< top_; i++)new_stack[i] = stack_[i];
    size_ *=2;
    delete [] stack_;
    stack_ = new_stack;
    return true;
  }
  void Put(std::ostream& os){
    os << "||>";
    for(int i = 0; i<top_; i++){
      os << stack_[i] << " ";
    }
    os << std::endl;
    return ;
  }

};

#endif // SBASE_STACK_HPP_