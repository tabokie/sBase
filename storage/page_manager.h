#include "status.h"
#include "file.h"

#include <vector>
#include <cassert>

namespace sbase{

using std::vector;

typedef uint8_t FileHandle;
typedef uint8_t PageHandle;

struct PageMeta{
  FileHandle file;
  size_t offset;
  size_t size;
  PageMeta(FileHandle f, size_t o, size_t s = -1):
    file(f),offset(o),size(s){
      if(size < 0){
        size = file.block_size;
      }
    }
  PageMeta(PageMeta& that):file(that.file),offset(that.offset),size(that.size){ }
  ~PageMeta(){ }

};

// unique page handle & file handle

class PageManager{
  vector<WritableFile> file_;
  vector<PageMeta> page_;
  MemPool pool_;
  BuzyQueue<size_t> buzy_;

 public:
  PageManager();
  ~PageManager();
  // on file
  Status NewFile(FileMeta file, FileHandle& handle);
  Status CloseFile(FileHandle file);
  // on page
  Status New(FileHandle file, PageHandle& handle);
  Status New(FileHandle file, size_t size, PageHandle& handle);
  Status Read(PageHandle page, char*& ret_ptr);
  Status Read(PageHandle page, size_t size, char*& ret_ptr);
  Status Write(PageHandle page, char* data_ptr);
  Status Write(PageHandle page, size_t size, char* data_ptr);
 private:
  // mem pool interface
  Status FlushPage(PageHandle page);
  // disk interface
  Status DirectWritePage(PageHandle page, char* data_ptr);

};

} // namespace sbase



