#ifndef SBASE_STORAGE_FILE_H_
#define SBASE_STORAGE_FILE_H_

namespace sbase{

enum Access {
  kSequential = 0,
  kRandom = 1
};

struct FileMeta{
  const char* filename;
  size_t block_size;
  Access access;

  FileMeta(const char* name, size_t block, Access acs):
    filename(name),block_size(block),access(acs){ }
  // copy allowed
  FileMeta(const FileMeta& that){
    filename = that.filename;
    block_size = that.block_size;
    access = that.access;
  }
  ~FileMeta(){ }
};

#if defined(__WIN32) || defined(__WIN64)


#include <windows.h>

typedef HANDLE OsFileHandle;
typedef HANDLE OsMapHandle;

class WritableFile{
 protected:
  FileMeta file_;
  OsFileHandle fhandle_;
  size_t file_end_;
 public:
  WritableFile(FileMeta file):file_(file){ }
  virtual ~WritableFile(){ };
  virtual Status Open(void);
  virtual Status Close(void);
  inline Access access(void){return file_.access;}
  inline const char* name(void){return file_.name;}
  inline const size_t size(void){return file_end_;}
};

class SequentialFile : public WritableFile{
 public:
  SequentialFile(FileMeta file):WritableFile(file){ }
  ~SequentialFile(){ }
  Status Read(size_t offset, char* alloc_ptr);
  Status Flush(size_t offset, char* data_ptr);
  Status Append(size_t offset);
};

class RandomAccessFile : public WritableFile{
 private:
  OsMapHandle mhandle_;
 public:
  RandomAccessFile(FileMeta file):WritableFile(file){ }
  Status Read(size_t offset, size_t size, char* alloc_ptr);
  Status Flush(size_t offset, size_t size, char* data_ptr);
  Status Append(size_t offset, size_t size);
};

#endif // __WINXX


}


#endif // SBASE_STORAGE_FILE_H_