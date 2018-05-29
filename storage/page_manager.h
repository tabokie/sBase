#ifndef SBASE_STORAGE_PAGE_MANAGER_H_
#define SBASE_STORAGE_PAGE_MANAGER_H_

#include "./util/status.h" 
#include "./storage/file.h" // class WritableFile
#include "./storage/mempool.hpp" // class MemPool
#include "./util/utility.hpp" // class NonCopy
#include "./storage/page.hpp" // class Page
#include "./util/latch.hpp" // class Latch
#include "./storage/file_format.hpp" // typename FileHandle, PageHandle, PageNum

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
  using PageIterator = std::vector<PagePtr>::iterator;
  FilePtr file;
  std::atomic<bool> fileOpened;
  Latch latch; // control file modify(set end, pages ptr modify)
  std::vector<PagePtr> pages;
  std::atomic<size_t> free_index;
  std::atomic<size_t> free_size;
  std::atomic<size_t> size;
  PageWrapper(FilePtr f):file(f),free_index(0),free_size(0){ }
  ~PageWrapper(){ }
  inline PageIterator PageBegin(void){return pages.begin();}
  inline PageIterator PageEnd(void){return pages.end();}
  inline size_t size(void){return size;}
  PagePtr& operator[](size_t idx){
    idx = idx & PageNumMask; // safety
    if(idx >= size )return nullptr;
    return pages[idx];
  }
  PagePtr& GetPage(size_t idx){
    return (*this)[idx];
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
// no detail about lock
// no MemMap used
class PageManager: public NonCopy{ 
  friend PageRef;
public:
  using FilePtr = typename WritableFile::WritableFilePtr;
  using PagePtr = typename Page::PagePtr;
  using FileWrapperPtr = typename FileWrapper::FileWrapperPtr;
  HashMap<FileHandle, FileWrapperPtr> file_;
  MemPool<PageHandle> pool_;

 public:
  PageManager() = default;
  ~PageManager(){ };
  // File Interface // 
  Status NewFile(std::string name, size_t fileSize, FileHandle& hFile);
  Status OpenFile(std::string name, FileHandle& hFile);
  Status CloseFile(FileHandle hFile);
  Status DeleteFile(FileHandle hFile);
  // Page Interface //
  // Construct
  // modify file
  Status NewPage(FileHandle hFile, PageType type, PageHandle& hPage);
  Status DeletePage(FileHandle hFile);
  // To compatible with PageRef, all have lock control
  // Synchronic 
  // Function: Mem >= File
  Status SyncFromFile(PageHandle hPage, bool lock = true);
  // Function: File >= Mem
  Status SyncFromMem(PageHandle hPage, bool lock = true);
  // Function: ptr -> File
  Status DirectWrite(PageHandle hPage, char* ptr, bool lock = true);
  // Memory
  Status Pool(PageHandle hPage, bool lock = true); // for unreferenced page, ready to retire
  Status Expire(PageHandle hPage, bool lock = true); // flush and expire
  // Public Accessor //
  size_t GetFileSize(FileHandle hFile);
  size_t GetPageSize(PageHandle hPage);
 protected: 
  // data only accessible by pageRef
  // PageRef responsible for lock
  char* GetPageDataPtr(PageHandle hPage); 
  Latch* GetPageLatch(PageHandle hPage);
  Latch* GetFileLatch(FileHandle hFile);
 private:
  // Mem ~ File
  // manage page lock here
  Status FlushToDisk(PageHandle hPage, bool lock = true);
  Status PoolFromDisk(PageHandle hPage, bool lock = true);
  Status PoolFromMmap(PageHandle hPage) = delete;
  // helper
  inline PagePtr GetPage(PageHandle hPage);
  inline FilePtr GetFile(FileHandle hFile);
  inline FileWrapperPtr GetFileWrapper(FileHandle hFile);
}; // class PageManager

} // namespace sbase



#endif // SBASE_STORAGE_PAGE_MANAGER_H_