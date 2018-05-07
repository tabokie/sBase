#ifndef SBASE_UTIL_STACK_HPP_
#define SBASE_UTIL_STACK_HPP_

#include <cstddef>
#include <cassert>
#include <iostream>
using namespace std;


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
  Stack(const Stack& that):size_(that.size_),top_(that.top_){
    stack_ = new ElementType[size_];
    for(int i = 0; i<top_; i++)stack_[i] = that.stack_[i];
  }
  template <class ...Args>
  Stack(Args... args):size_(sizeof...(args)){
    stack_ = new ElementType[size_];
    top_ = size_;
    InitWithArgs(args...);
  }
  ~Stack(){ delete [] stack_; }

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
  Stack operator+(const Stack& that){
    Stack<ElementType> new_stack = *this;
    size_t new_size = new_stack.top_ + that.top_;
    while(new_size > new_stack.size_)
      assert(new_stack.Expand());
    for(int i = new_stack.top_; i < new_size; i++){
      new_stack.stack_[i] = that.stack_[i-top_];
    }
    new_stack.top_ = new_size;
    return new_stack;
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
  size_t size(void){return top_;}
  void Put(std::ostream& os){
    for(int i = top_-1; i >= 0; i--){
      os << stack_[i] << " ";
    }
    os << std::endl;
    return ;
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

  friend std::ostream& operator<<(std::ostream& os, Stack<_ET> that){
    os << "||>";
    for(int i = 0; i<that.top_; i++){
      os << that.stack_[i] << " ";
    }
    os << std::endl;
    return os;
  }


};

#endif // SBASE_UTIL_STACK_HPP_