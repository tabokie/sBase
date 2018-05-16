#ifndef SBASE_STORAGE_PAGE_MANAGER_H_
#define SBASE_STORAGE_PAGE_MANAGER_H_

#include "./util/status.h"
#include "./storage/file.h"
#include "./storage/mempool.hpp"
#include "./util/buzy_queue.hpp"

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

enum PageType{
  kBFlowTablePage = 0,
  kBPlusIndexPage = 1,
  kBIndexPage = 2,
  kInvalidPage = 3
};

struct DataCursor{
  size_t offset;
  size_t size;
  DataCursor(size_t a, size_t b):offset(a),size(b){ }
  DataCursor():offset(0),size(0){ }
  static DataCursor NilCursor(void){return DataCursor();}
  bool isNilCursor(void) const{return size == 0;}
  bool operator<(const DataCursor& rhs){
    return offset+size < rhs.offset+rhs.size;
  }
  bool operator<=(const size_t end){
    return offset+size < end;
  }

};


struct PageMeta{
  FileHandle file;
  PageType type;
  Latch latch;
  size_t offset;
  size_t size;
  PageMeta(FileHandle f, PageType t, size_t o, size_t s = 0):
  file(f),offset(o),size(s),type(t) { }
  PageMeta(const PageMeta& that):
  file(that.file),type(that.type),offset(that.offset),size(that.size){ }
  ~PageMeta(){ }
};

// unique page handle & file handle
// class FrontMock;

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
  FileHandle file;
  DeducedRuntimeAccessMode mode;
  char* ptr;
  PageManager* manager;
 public:
  PageRef(PageManager* manage, PageHandle page, RuntimeAccessMode rmode):
  handle(page),manager(manage),ptr(nullptr){
    file = page >> (kLocalPageHandleWid) * 8;
    if(!manager || rmode == kFailAccess)goto FAILING_REFERENCE;
    else{
      manager->Pool(handle); // assert(inPool || no modify process), safe to pool
      if(rmode == kReadOnly){
        mode = kReadOnlyByPool;
        // read lock

      }
      else if(rmode == kLazyModify){
        mode = kModifyByPool; 
        // write lock

      }
      else{
        mode = kModifyByFile;
        // block other write

        // self init ptr
        ptr = new char[];
      }
    }
    FAILING_REFERENCE: mode = kFailAccess;
  }
  bool Invalid(void){return !manager || !}
  ~PageRef(){
    if(!manager && ptr){
      Error("Page reference by a corrupted page manager.");
    }
    if(ptr){
      if(mode == kReadOnlyByPool || mode == kModifyByPool){
        // unlock
      }
      else if(mode == kModifyByFile){
        // write to file
        manager->WriteFile(handle, ptr);
        // expire pool        
        manager->Expire(handle);
        delete [] ptr;
      }
    }
  }


};

// caller shouldnt know detail about file offset and size
class PageManager{ // for unreferenced page, ready to retire
public:

  using FilePtr = shared_ptr<WritableFile>;

  HashMap<FileHandle, FilePtr> file_;
  HashMap<ComposeHandle, PageMeta> page_;
  MemPool<ComposeHandle> pool_; // no control over page retire // not sequential
  BuzyQueue<PageHandle> buzy_; // control over page retire

 public:
  PageManager() = default;
  ~PageManager(){ };
  // on file
  Status NewFile(FileMeta file, FileHandle& handle);
  Status CloseFile(FileHandle file);
  Status DeleteFile(FileHandle file);
  // on page
  Status Get(ComposeHandle handle, RuntimeAccessMode mode, char*& ret){
    bool bool_ret;
    Status status_ret;
    if(mode == kFailAccess)return Status::Corruption("Invalid access mode.");
    else if(mode == kReadOnly || mode == kLazyModify){ // no pooling detail
      PageMeta page_meta;
      bool_ret = page_.Get(handle, page_meta);
      if(!bool_ret){
        return Status::InvalidArgument("Page not encoded.");
      }
      if(pool_.pooling(handle)){
        ret = pool_.get_ptr(handle);
        return Status::OK();
      }
      else{
        status_ret = Pool(handle);
        if(!status_ret.ok())return Status::IOError("Fail pool page.");
        ret = pool_.get_ptr(handle);
      }
    }
    else if(mode == kFatalModify){
      PageMeta page_meta;
      bool_ret = page_.Get(handle, page_meta);
      if(!bool_ret){
        return Status::InvalidArgument("Page not encoded.");
      }
      // write lock
      if(pool_.pooling(handle)){ // write to pool first

      }
      else{ // consistency: 
        // mem pool synchronic
        // add additional read lock
        // direct write
        // mem pool retire
      }
      // direct write

    }
    else{
      return Status::InvalidArgument("Unsupported access mode.")
    }
  }
  Status Pool();
  Status PoolByCopy();
  Status PoolByMap();

  Status NewPage(FileHandle file, DataCursor ptr = DataCursor::NilCursor());
  Status AppendPage(FileHandle file, PageHandle& handle, DataCursor ptr = DataCursor::NilCursor());
  // no intention to pooling page in memory
  Status ReadByCopy(PageHandle page, char*& ret_ptr, DataCursor ptr = DataCursor::NilCursor());
  // forced pooling page
  Status ReadByReference(PageHandle page, char*& ret_ptr, DataCursor ptr = DataCursor::NilCursor());
  // for lru
  Status DecrReference(PageHandle page); // atomic needed
  // still pooling
  Status WriteAndCommit(PageHandle page, char* data_ptr, DataCursor ptr = DataCursor::NilCursor());
  Status WriteToPool(PageHandle page, char* data_ptr, DataCursor ptr = DataCursor::NilCursor());
  Status Commit(PageHandle page);
  Status Unpool(PageHandle page);

  Status New(FileHandle file, PageHandle& handle);
  Status New(FileHandle file, size_t size, PageHandle& handle);
  Status Read(PageHandle page, char*& ret_ptr);
  Status Read(PageHandle page, size_t page_offset, size_t size, char*& ret_ptr);
  Status Write(PageHandle page, char* data_ptr);
  Status Write(PageHandle page, char* data_ptr, size_t size);
  Status Flush(PageHandle page){return FlushPage(page);}
  // page accessor
  inline PageType type(PageHandle page){
    if(page >= page_.size())return kInvalidPage;
    else return page_[page].type;
  }
 private:
  // mem pool interface
  Status FlushPage(PageHandle page);
  Status FlushPage(void);
  // disk interface
  Status DirectWritePage(PageHandle page, char* data_ptr);
  Status DirectWritePage(PageHandle page, char* data_ptr, size_t size);
};

} // namespace sbase



#endif // SBASE_STORAGE_PAGE_MANAGER_H_