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
  kFatalModify = 3 
};

namespace {
enum DeducedRuntimeAccessMode{
  kFailAccess = 0,
  kReadOnlyByPool = 1,
  // kReadOnlyByMap // reduce to file copy
  // kReadOnlyByFile
  kModifyByPool = 2,
  kModifyByFile = 3
};	
} // namespace anonymous

// use by pointer
// RAII reference pointer
struct PageRef: public NonCopy{
  PageManager* manager; // global
  PageHandle handle;
  DeducedRuntimeAccessMode mode;
 public:
  char* ptr;
  PageRef(PageManager* manage, PageHandle page, RuntimeAccessMode rmode):
  handle(page),manager(manage),ptr(nullptr){
    // LOG_FUNC();
    if(!manager)mode = kFailAccess;
    else{
      manager->Pool(handle); // assert(inPool || no modify process), safe to pool
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
      else mode = kFailAccess;
    }
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
    }
  }
}; // class PageRef

} // namespace sbase

#endif // SBASE_STORAGE_PAGE_REF_HPP_