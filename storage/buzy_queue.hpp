
#include "page_manager.h"
#include "hash.hpp"
#include <memory>

template <class _ET>
class BuzyQueue{

  using ElementType = _ET;

  size_t size_;
  struct Wrapper_{
    shared_ptr<Wrapper_> pri;
    shared_ptr<Wrapper_> next;
    ElementType element;
    Wrapper_(ElementType e):pri(nullptr),next(nullptr),element(e){ }
    ~Wrapper_(){ }
  };
  using ListNodePtr = shared_ptr<Wrapper_>;
  ListNodePtr head_, tail_;

  HashMap<ElementType, ListNodePtr> map_;

 public:

  BuzyQueue(size_t initsize = kBuzyQueueSize):size_(initsize){ }
  ~BuzyQueue(){ }
  Status Visit(PageHandle page){
    ListNodePtr ptr = nullptr;
    if(map_.DeleteOnGet(page, ptr)){
      if(!ptr)return Status::Corruption("Cannot Get Element from Map");
      // make tail
      ListNodePtr pri  = ptr->pri;
      ListNodePtr next = ptr->next;
      assert((ptr==tail_) == (!next)); // equivalence test
      if(ptr == tail_){
        tail_ = pri;
      }
      if(pri)pri->next = next;
      if(next)next->pri = pri;
      // make head
      head_->pri = ptr;
      ptr->next = head_;
      head_ = ptr;
      return Status::OK();
    }
    else{
      ptr = make_shared<Wrapper_>(page);
      head_->pri = ptr;
      ptr->next = head_;
      head_ = ptr;
      if(!map_.Insert(page, ptr))return Status::Corruption("Insert to Buzy Hash Failed");
      return Status::OK();
    }
  }
  Status Last(PageHandle& ret){ // delete on get
    if(tail_ == nullptr){
      return Status::Corruption("Empty Tail Ptr");
    }
    ret = tail_->element;
    assert((!tail_->pri) == (tail_==head_));
    if(tail_->pri)tail_->pri->next = nullptr;
    else head_ = nullptr;
    tail_ = tail_->pri;
    return Status::OK();
  }

}