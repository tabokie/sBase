#ifndef SBASE_STORAGE_PAGE_MANAGER_H_
#define SBASE_STORAGE_PAGE_MANAGER_H_

#include "./util/status.h" 
#include "./storage/file.h" // class File
#include "./storage/mempool.hpp" // class MemPool
#include "./util/utility.hpp" // class NonCopy
#include "./storage/page.hpp" // class Page
#include "./util/latch.hpp" // class Latch
#include "./storage/file_format.hpp"

#include <vector>
#include <cassert>
#include <iostream>

namespace sbase{

class PageRef;

namespace {
struct FileWrapper: public NonCopy{
  using FilePtr = typename WritableFile::WritableFilePtr;
  using FileWrapperPtr = shared_ptr<FileWrapper>;
  using PagePtr = typename Page::PagePtr;
  FilePtr file;
  Latch latch;
  std::vector<PagePtr> pages;
  std::atomic<size_t> free_index;
  std::atomic<size_t> free_size;
  std::atomic<size_t> size;
  PageWrapper(FilePtr f):file(f),free_index(0),free_size(0){ }
  ~PageWrapper(){ }
  PagePtr operator[](size_t idx){
    idx = idx & kLocalPageHandleMask;
    if(idx >= size )return nullptr;
    return pages[idx];
  }
  PageNum AddPage(PagePtr p){
    size_t ret;
    latch.WriteLock();
    p->handle = GetPageHandle(file->handle, pages.size());
    pages.push_back(p);
    ret = static_cast<PageNum>(pages.size());
    latch.ReleaseWriteLock();
    if(!p){
      if(free_size == 0)
        free_index = pages.size();
      free_size++;
    }
    size++;
    return ret;
  }
  // 0 is always for meta data, return 0 if no free
  PageNum GetFree(void){
    if(free_size == 0){
      return static_cast<PageNum>(0);
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


}

// caller shouldnt know detail about file offset and size
// no MemMap used
class PageManager: public NonCopy{ // for unreferenced page, ready to retire
  friend PageRef;
public:
  using FilePtr = typename WritableFile::WritableFilePtr;
  using PagePtr = typename Page::PagePtr;
  using FileWrapperPtr = typename FileWrapper::FileWrapperPtr;
  HashMap<FileHandle, FileWrapperPtr> file_;
  MemPool<PageHandle> pool_; // no control over page retire // not sequential

 public:
  PageManager() = default;
  ~PageManager(){ };
  // File Interface // 
  Status NewFile(std::string name, FileHandle& hFile);
  Status CloseFile(FileHandle hFile);
  Status DeleteFile(FileHandle hFile);
  // Page Interface //
  Status NewPage(FileHandle hFile, PageType type, PageHandle& hPage);
  Status DeletePage(FileHandle hFile);
  // Function: Mem >= File
  Status SyncFromFile(PageHandle hPage);
  // Function: File >= Mem
  Status SyncFromMem(PageHandle hPage); // flush if commited < modified
  Status Pool(PageHandle hPage);
  Status Expire(PageHandle hPage); // expire on flush
  // Public Accessor //
  size_t GetSize(PageHandle hPage);
  char* GetPageDataPtr(PageHandle hPage); 
 protected:
  Latch* GetPageLatch(PageHandle hPage);
  Latch* GetFileLatch(FileHandle hFile);
 private:
  Status PoolFromDisk(PageHandle hPage); // pool if not, assert(if inPool then commited<modified)
  Status PoolFromMmap(PageHandle hPage) = delete;
}; // class PageManager

} // namespace sbase



#endif // SBASE_STORAGE_PAGE_MANAGER_H_