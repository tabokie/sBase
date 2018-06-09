#ifndef SBASE_STORAGE_PAGE_MANAGER_H_
#define SBASE_STORAGE_PAGE_MANAGER_H_

#include "./util/status.hpp" 
#include "./storage/file.h" // class WritableFile
#include "./storage/mempool.hpp" // class MemPool
#include "./util/utility.hpp" // class NonCopy
#include "./storage/page.hpp" // class Page
#include "./util/latch.hpp" // class Latch
#include "./storage/file_format.hpp" // typename FileHandle, PageHandle, PageNum
#include "./util/random.hpp" // random

#include <vector>
#include <cassert>
#include <string>
#include <iostream>
#include <memory>

namespace sbase{

class PageRef;

// namespace {
struct FileWrapper: public NoCopy{
  using FilePtr = typename WritableFile::WritableFilePtr;
  using FileWrapperPtr = shared_ptr<FileWrapper>;
  using PagePtr = typename Page::PagePtr;
  using PageIterator = ::std::vector<PagePtr>::iterator;
  // Low-level
  FilePtr file;
  // Extern Info
  int blockSize; // bytes
  FileHandle hFile;
  ::std::string fileName;
  size_t dataOffset;
  // Data
  ::std::vector<PagePtr> pages;
  // Synchronic
  Latch latch; // control multiple modifiers
  std::atomic<size_t> free_index;
  std::atomic<size_t> free_size;
  std::atomic<size_t> page_size;
  // self initial essential data
  FileWrapper(FilePtr& f):file(f),free_index(0),free_size(0),page_size(0){ }
  ~FileWrapper(){ }
  inline PageIterator PageBegin(void){return pages.begin();}
  inline PageIterator PageEnd(void){return pages.end();}
  inline size_t size(void){return static_cast<int>(page_size);}
  // weak consistency
  inline PagePtr& operator[](size_t idx){
    PageNum inner = (idx & PageNumMask) - 1; // safety
    assert(inner >= 0 && inner < page_size);
    return pages[inner];
  }
  inline  PagePtr& GetPage(size_t idx){
    return (*this)[idx];
  }
  PageNum AddPage(PagePtr p){
    // LOG_FUNC();
    size_t ret;
    // sync
    latch.WeakWriteLock();
    // push to vector
    pages.push_back(p);
    // store handle in page
    p->handle = GetPageHandle(hFile, pages.size());
    // now update size
    page_size++;
    assert(pages.size() == page_size);
    // handle freelist
    if(!p){
      if(free_size == 0)
        free_index = pages.size();
      free_size++;
    }
    latch.ReleaseWeakWriteLock();
    // return handle
    ret = static_cast<PageNum>(page_size);
    return ret;
  }
  bool DeletePage(PageHandle hPage){
    PageNum idx = (hPage & PageNumMask) - 1; // safety
    assert(idx >= 0 && idx < page_size);
    PagePtr& p = pages[idx];
    if(!p)return true; // already deleted
    latch.WeakWriteLock();
    p = nullptr;
    // update freelist
    if(free_size == 0 || free_index > idx){
      free_size++;
      free_index = idx;
    }
    latch.ReleaseWeakWriteLock();
    return true;
  }
  // 0 is always for meta data, return 0 if no free
  PageNum GetFree(void){
    if(free_size == 0){
      return static_cast<PageNum>(0);
    }
    else{
      latch.WeakWriteLock();
      size_t index = free_index;
      free_size --;
      assert(index < page_size && !pages[index]);
      // initial a new page
      pages[index] = std::make_shared<Page>(*file);
      // find new free
      if(free_size > 0)
        for(size_t i = free_index+1; i < page_size; i++){
          if(!pages[i]){
            free_index = i;
            break;
          }
        }
      latch.ReleaseWeakWriteLock();
      return index+1;
    }
  }
};


// }

const int kLruSearchMax = 12;
const int kLruSampleMax = 5;
// const int kLruPackSize = 5;

// caller shouldnt know detail about file offset and size
// no detail about lock
// no MemMap used
class PageManager: public NoCopy{ 
  friend PageRef;  
  using FilePtr = typename WritableFile::WritableFilePtr;
  using PagePtr = typename Page::PagePtr;
  using FileWrapperPtr = typename FileWrapper::FileWrapperPtr;
  HashMap<FileHandle, FileWrapperPtr> file_;
  MemPool<PageHandle> pool_;
 public:
  PageManager() = default;
  ~PageManager(){ };
 private:
  // essential helper
  inline FileWrapperPtr GetFileWrapper(FileHandle hFile){
    FileWrapperPtr fw = nullptr;
    file_.Get(hFile, fw);
    return fw;
  }  
  inline PagePtr GetPage(PageHandle hPage){
    FileWrapperPtr fw = GetFileWrapper(GetFileHandle(hPage));
    PageNum no = GetPageNum(hPage);
    if(fw){
      return (*fw)[no];
    }
    return nullptr;
  }
  inline FilePtr GetFile(FileHandle hFile){
    FileWrapperPtr fw = GetFileWrapper(hFile);
    if(fw)return fw->file;
    return nullptr;
  }
 protected:
  // data only accessible by pageRef
  // PageRef responsible for lock
  inline char* GetPageDataPtr(PageHandle hPage){ // no explicit pooling
    // assume get modified
    PagePtr p = GetPage(hPage);
    p->Modify();
    return pool_.Get(hPage);
  } 
  inline Latch* GetPageLatch(PageHandle hPage){
    PagePtr p = GetPage(hPage);
    if(p)return &(p->latch);
    return nullptr;
  }
  inline Latch* GetFileLatch(FileHandle hFile){
    FileWrapperPtr fw = GetFileWrapper(hFile);
    if(fw)return &(fw->latch);
    return nullptr;
  }
  // helper
  inline uint8_t GetAvailableFileHandle(void){
    for(FileHandle hF = 0; hF <= 255 ;hF++){
      FileWrapperPtr p = nullptr;
      if(!pool_.pooling(hF)){
        return hF;
      }
    }
    return 255;
  }
  inline size_t GetPageOffset(PageHandle hPage){
    if(hPage == 0)return 0;
    return kFileHeaderLength + (hPage-1) * GetPageSize(hPage);
  }
 public:
  /// File Interface ///
  // :: add/override hashmap of hFile to FileWrapper
  // sync: hand down to HashMap
  // undefined behaviour if file already opened
  Status NewFile(::std::string name, uint8_t block, FileHandle& hFile);
  // :: open file and read in header and pages
  Status OpenFile(::std::string name, FileHandle& hFile);
  // :: close file and expire all related data
  // sync: WriteLock on pages
  Status CloseFile(FileHandle hFile);
  // :: delete file and expire all related data
  // sync: WriteLock on pages
  Status DeleteFile(FileHandle hFile);
  /// Page Interface ///
  // for Construct //
  // find free slot or append new block on file
  Status NewPage(FileHandle hFile, PageType type, PageHandle& hPage);
  Status DeletePage(PageHandle hFile);
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
  Status Switch(PageHandle hPage, PageHandle hPageNew, bool lock = true); // flush and expire
  // Public Accessor //
  inline size_t GetFileSize(FileHandle hFile){
    FilePtr f = GetFile(hFile);
    if(f)return f->size();
    return 0;
  }
  inline size_t GetPageSize(PageHandle hPage){
    FileWrapperPtr fw = GetFileWrapper(GetFileHandle(hPage));
    if(fw)return fw->blockSize;
    return 0;
  }
  bool fileIsOpened(FileHandle hFile){
    FileWrapperPtr pRet = nullptr;
    if(!file_.Get(hFile, pRet) || !pRet)return false;
    return true;
  }
  // Public File Header Mpdifier //
  // inline bool ChangeRootOffset(FileHandle hFile, size_t offset);
 private:
  // Mem ~ File
  // manage page lock here
  // manage LRU state here
  Status FlushFromPool(PageHandle hPage, bool lock = true);
  Status FlushFromPtr(PageHandle hPage, char* data, bool lock = true);
  Status PoolFromDisk(PageHandle hPage, bool lock = true);
  Status PoolFromMmap(PageHandle hPage) = delete;

}; // class PageManager

} // namespace sbase



#endif // SBASE_STORAGE_PAGE_MANAGER_H_