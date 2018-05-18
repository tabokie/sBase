#ifndef SBASE_STORAGE_PAGE_MANAGER_H_
#define SBASE_STORAGE_PAGE_MANAGER_H_

#include "./util/status.h"
#include "./storage/file.h"
#include "./storage/mempool.hpp"
#include "./util/buzy_queue.hpp"
#include "./util/utility.hpp"
#include "./storage/page.hpp"

#include <vector>
#include <cassert>
#include <iostream>
using namespace std;

namespace sbase{

using std::vector;

typedef uint8_t FileHandle;
const size_t kFileHandleWid = 1 ;
typedef uint32_t PageHandle;
const size_t kPageHandleWid = 4;
typedef uint32_t LocalPageHandle; // uint24_t in essence
const size_t kLocalPageHandleWid = 3;
typedef uint8_t PageSizeType;
const size_t kPageSizeWid = 1;

#define LocalPage2Page(f_, p_)      (static_cast<PageHandle>(f_)<<kLocalPageHandleWid + static_cast<PageHandle>(p_) )  

enum RuntimeAccessMode{
  kFailAccess = 0,
  kReadOnly = 1,
  kLazyModify = 2,
  kFatalModify = 3 
};

enum DeducedRuntimeAccessMode{
  kFailAccess = 0,
  kReadOnlyByPool = 1,
  // kReadOnlyByMap
  // kReadOnlyByFile
  kModifyByPool = 2,
  kModifyByFile = 3
};

// use by pointer
// RAII reference pointer
struct PageRef{
  PageManager* manager; // global
  PageHandle handle;
  // FileHandle file;
  DeducedRuntimeAccessMode mode;
 public:
  char* ptr;
  PageRef(PageManager* manage, PageHandle page, RuntimeAccessMode rmode):
  handle(page),manager(manage),ptr(nullptr){
    // file = page >> (kLocalPageHandleWid) * 8;
    if(!manager)mode = kFailAccess;
    else{
      manager->Pool(handle); // assert(inPool || no modify process), safe to pool
      if(rmode == kReadOnly){
        mode = kReadOnlyByPool;
        // read lock
        auto latch = manager->GetLatch(handle);
        latch->ReadLock();
        ptr = manager->GetPageDataPtr(handle);
      }
      else if(rmode == kLazyModify){
        mode = kModifyByPool; 
        // write lock
        auto latch = manager->GetLatch(handle);
        latch->WriteLock();
        ptr = manager->GetPageDataPtr(handle);
      }
      else if(rmode == kFatalModify){
        mode = kModifyByFile;
        // block other write
        auto latch = manager->GetLatch(handle);
        latch->WeakWriteLock();
        manager->Sync(handle);
        size_t size = manager->GetSize(handle);
        // self init ptr
        ptr = new char[size];
      }
      else mode = kFailAccess;
    }
  }
  ~PageRef(){
    if(!manager && ptr){
      Error("Page reference by a corrupted page manager.");
    }
    else if(ptr){
      if(mode == kReadOnlyByPooll){
        // unlock
        auto latch = manager->GetLatch(handle);
        latch->ReleaseReadLock();
      }
      else if(mode == kModifyByPool){
        auto latch = manager->GetLatch(handle);
        latch->ReleaseWriteLock();
      }
      else if(mode == kModifyByFile){
        // write to file
        manager->WriteFile(handle, ptr);
        // expire pool        
        manager->Expire(handle);
        auto latch = manager->GetLatch(handle);
        latch->ReleaseWeakWriteLock();
        delete [] ptr;
      }
    }
  }

}; // class PageRef

// caller shouldnt know detail about file offset and size
class PageManager{ // for unreferenced page, ready to retire
public:

  using FilePtr = shared_ptr<WritableFile>;
  using PagePtr = shared_ptr<Page>;
  struct FileWrapper: public NonCopy{ // only ctrl over vector
    FilePtr file;
    Latch latch;
    std::vector<PagePtr> pages;
    std::atomic<size_t> free_index;
    std::atomic<size_t> free_size;
    std::atomic<size_t> size;
    PageWrapper(FilePtr f):file(f),free_index(0),free_size(0){ }
    ~PageWrapper(){ }
    PagePtr operator[](size_t idx){
      if(idx >= size )return nullptr;
      return pages[idx];
    }
    LocalPageHandle AddPage(PagePtr p){
      size_t ret;
      latch.WeakWriteLock();
      pages.push_back(p);
      ret = static_cast<LocalPageHandle>(pages.size());
      latch.ReleaseWeakWriteLock();
      if(!p){
        if(free_size == 0)
          free_index = pages.size();
        free_size++;
      }
      size++;
      return ret;
    }
    // 0 is always for meta data, return 0 if no free
    LocalPageHandle GetFree(void){
      if(free_size == 0){
        return static_cast<LocalPageHandle>(0);
      }
      else{
        size_t index = free_index;
        free_size --; 
        if(free_size > 0)
          for(size_t i = free_index+1; i < pages.size(); i++){
            if(!pages[i]){
              free_index = i;
              break;
            }
          }
        return index;
      }
    }
  };
  using FileWrapperPtr = shared_ptr<FileWrapper>;

  HashMap<FileHandle, FileWrapperPtr> file_;
  MemPool<PageHandle> pool_; // no control over page retire // not sequential

 public:
  PageManager() = default;
  ~PageManager(){ };
  // File Interface // 
  Status NewFile(File file, FileHandle& handle);
  Status CloseFile(FileHandle file);
  Status DeleteFile(FileHandle file);
  // Page Interface //
  Status NewPage(FileHandle file, PageHandle& handle);
  Status Pool(PageHandle handle); // pool if not, assert(not commited > modified && inPool)
  Status Flush(PageHandle handle); // flush if commited < modified
  Status Expire(PageHandle handle);
 protected:
  Latch* GetPageLatch(PageHandle handle);
  Latch* GetFileLatch(FileHandle handle);
  size_t GetSize(PageHandle handle);
  char* GetPageDataPtr(PageHandle handle);
};

} // namespace sbase



#endif // SBASE_STORAGE_PAGE_MANAGER_H_