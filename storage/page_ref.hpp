#ifndef SBASE_STORAGE_PAGE_REF_HPP_
#define SBASE_STORAGE_PAGE_REF_HPP_

#include "./storage/file_format.hpp"
#include "./storage/page_manager.h"
#include "./util/error.hpp"

#include <iostream>
#include <cstdio>

namespace sbase{


enum RuntimeAccessMode{
  kEmptyAccess = 0,
  kReadOnly = 1,
  kLazyModify = 2,
  kFatalModify = 3,
  kIncrementalModify = 4
};

namespace {
enum DeducedRuntimeAccessMode{
  kFailAccess = 0,
  kReadOnlyByPool = 1,
  // kReadOnlyByMap // reduce to file copy
  // kReadOnlyByFile
  kModifyByPool = 2,
  kModifyByPoolNonFatal = 3,
  kModifyByFile = 4
};	
} // namespace anonymous

// use by pointer
// RAII reference pointer
struct PageRef: public NoCopy{
  PageManager* manager; // global
  PageHandle handle;
  DeducedRuntimeAccessMode mode;
 public:
  char* ptr;
  PageRef(PageManager* manage, PageHandle page, RuntimeAccessMode rmode):
  handle(page),manager(manage),ptr(nullptr){
    // std::cout << "referencing: " << page << std::endl;
    // LOG_FUNC();
    if(!manager)mode = kFailAccess;
    else{
      manager->Pool(handle);
      // std::cout << "pooling: " << manager->Pool(handle).ToString() << std::endl;

      if(rmode == kReadOnly){
        mode = kReadOnlyByPool;
        // read lock
        auto latch = manager->GetPageLatch(handle);
        latch->ReadLock();
        ptr = manager->GetPageDataPtr(handle);
      }
      else if(rmode == kLazyModify){
        mode = kModifyByPool; 
        // write lock
        auto latch = manager->GetPageLatch(handle);
        latch->WriteLock();
        ptr = manager->GetPageDataPtr(handle);
      }
      else if(rmode == kFatalModify){
        mode = kModifyByFile;
        // block other write
        auto latch = manager->GetPageLatch(handle);
        latch->WeakWriteLock();
        manager->SyncFromFile(handle, false);
        size_t size = manager->GetPageSize(handle);
        // self init ptr
        ptr = new char[size];
      }
      else if(rmode == kIncrementalModify){
        mode = kModifyByPoolNonFatal;
        // block other write
        auto latch = manager->GetPageLatch(handle);
        latch->WeakWriteLock();
        ptr = manager->GetPageDataPtr(handle);
      }
      else mode = kFailAccess;
      if(!ptr)std::cout << "Page referenced as a null ptr!" << std::endl;
    }
  }
  bool LiftToWrite(void){
    if(mode == kReadOnlyByPool){
      auto latch = manager->GetPageLatch(handle);
      latch->ReadLockLiftToWriteLock();
      mode = kModifyByPool;
      return true;
    }
    else if(mode == kModifyByPoolNonFatal){
      auto latch = manager->GetPageLatch(handle);
      latch->WeakWriteLockLiftToWriteLock();
      mode = kModifyByPool;
      return true;
    }
    return false;
  }
  ~PageRef(){
    // LOG_FUNC();
    if(!manager && ptr){
      LOG("Page reference by a corrupted page manager.");
      exit(0);
    }
    else if(ptr){
      if(mode == kReadOnlyByPool){
        // unlock
        auto latch = manager->GetPageLatch(handle);
        latch->ReleaseReadLock();
      }
      else if(mode == kModifyByPool){
        auto latch = manager->GetPageLatch(handle);
        latch->ReleaseWriteLock();
      }
      else if(mode == kModifyByFile){
        // write to file
        manager->DirectWrite(handle, ptr, false);
        // expire pool        
        manager->Expire(handle, false);
        auto latch = manager->GetPageLatch(handle);
        latch->ReleaseWeakWriteLock();
        delete [] ptr;
      }
      else if(mode == kModifyByPoolNonFatal){
        auto latch = manager->GetPageLatch(handle);
        latch->ReleaseWeakWriteLock();        
      }
    }
  }
}; // class PageRef

} // namespace sbase

#endif // SBASE_STORAGE_PAGE_REF_HPP_