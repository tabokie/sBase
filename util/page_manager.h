#include "status.h"


namespace sbase{

struct FileMeta{
  const char* filename;
  size_t blocksize;
};

typedef uint8_t FileHandle;
typedef uint8_t PageHandle;

struct File{
  const char* file_name;
  File(File& that):file_name(that.file_name){ }
  ~File(){ }
};

struct FileHandle : public File{
  FileHandle(File& that, char mode): write_mode(mode){ }
  ~FileHandle(){ }
  char write_mode; // a for additional, n for new
};

typedef uint32_t PageHandle;

class PageManager : private NoCopy{
 public:
  PageManager();
  ~PageManager();
  Status NewFile(FileMeta file);
  Status FetchPage(FileHandle file, PageHandle page);
  PageHandle NewPage(FileHandle file);
 private:
  NewSequentialPage(FileHandle file, size_t initial_block = 1);
  NewRandom

};

} // namespace sbase

PageManager::ReadPage(PageHandle page){

}

