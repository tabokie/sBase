#include "status.h"

#include <string>

namespace sbase{

struct FileMeta{
  const char* filename;
  size_t block_size; // bytes
  FileMeta(const char* name, size_t block):
    filename(name),block_size(block){ }
  FileMeta(const FileMeta& that){
    filename = that.filename;
    block_size = that.block_size;
  }
  ~FileMeta(){ }
};

class WritableFile{
 public:
  WritableFile(){ }
  virtual ~WritableFile(){ };
  virtual Status Open(void) = 0;
  // virtual Status Read( size_t offset, char* alloc_ptr);
  // virtual Status Append( size_t offset, char* data_ptr);
  // virtual Status Flush( size_t offset, char* data_ptr);
  virtual Status Close(void) = 0;
};

#if defined(__WIN32) || defined(__WIN64)

#include <windows.h>

typedef HANDLE OsFileHandle;
typedef HANDLE OsMapHandle;

class SequentialFile : public WritableFile{
 private:
  // std::string filename_;
  FileMeta file_;
  OsFileHandle fhandle_;
  size_t size_;
  size_t file_end_;
 public:
  SequentialFile(FileMeta file): file_(file){ }
  ~SequentialFile(){ }
  Status Open(void) {
    fhandle_ = CreateFile(file_.filename, 
      GENERIC_READ | GENERIC_WRITE, 
      0,  // share mode
      NULL,  // security
      OPEN_ALWAYS, 
      FILE_ATTRIBUTE_NORMAL, 
      NULL); // template file handle
    file_end_ = GetFileSize(fhandle_);
    return Status::OK();
  }
  Status Read(size_t offset, char* alloc_ptr) {
    DWORD dwPtr = SetFilePointer(fhandle_, 
      offset, 
      NULL, 
      0); // 0 for starting from beginning
    DWORD dwError;
    if(dwPtr == INVALID_SET_FILE_POINTER \
      && (dwError = GetLastError())!=NO_ERROR)return Status::IOError("Set File Pointer Failed");
    bool rfRes = ReadFile(fhandle_, 
      alloc_ptr, 
      file_.block_size, 
      NULL, // num of bytes read
      NULL); // overlapped structure
    if(!rfRes)return Status::IOError("Read File Failed");
    return Status::OK();
  }
  Status Flush(size_t offset, char* data_ptr) {
    DWORD dwPtr = SetFilePointer(fhandle_, 
      offset, 
      NULL, 
      0); // 0 for starting from beginning
    DWORD dwError;
    if(dwPtr == INVALID_SET_FILE_POINTER \
      && (dwError = GetLastError())!=NO_ERROR)return Status::IOError("Set File Pointer Failed");
    bool rfRes = WriteFile(fhandle_, 
      data_ptr, 
      file_.block_size, 
      NULL,  // num of bytes read
      NULL); // overlapped structure
    if(!rfRes)return Status::IOError("Write File Failed");
    return Status::OK();
  }
  Status Append(size_t offset) {
    DWORD dwPtr = SetFilePointer(fhandle_, \
      offset+file_.block_size, \
      NULL, \
      0); // 0 for starting from beginning
    DWORD dwError;
    if(dwPtr == INVALID_SET_FILE_POINTER \
      && (dwError = GetLastError())!=NO_ERROR)return Status::IOError("Set File Pointer Failed");
    SetEndOfFile(fhandle_);
    file_end_ = offset + file_.block_size;
    // SetFileValidData(fhandle_, offset+block_size_)
    return Status::OK();
  }
  Status Close(void){
    if(!CloseHandle(fhandle_))return Status::IOError("Close File Failed");
    return Status::OK();
  }
 private:
  // Status AppendSpace(size_t offset);
};

class RandomAccessFile : public WritableFile{
 private:
  // std::string filename_;
  FileMeta file_;
  OsFileHandle fhandle_;
  OsMapHandle mhandle_;
  char* map_ptr_;
  size_t file_end_;
 public:
  RandomAccessFile(FileMeta file): file_(file){ }
  ~RandomAccessFile(){ }
  Status Open(void) {
    fhandle_ = CreateFile(file_.filename, 
      GENERIC_READ | GENERIC_WRITE, 
      0,  // share mode
      NULL,  // security
      OPEN_ALWAYS, 
      FILE_ATTRIBUTE_NORMAL, 
      NULL); // template file handle
    file_end_ = GetFileSize(fhandle_);
    return Status::OK();
  }
  Status Append(size_t offset, size_t size){
    DWORD dwPtr = SetFilePointer(fhandle_, \
      offset+file_.block_size, \
      NULL, \
      0); // 0 for starting from beginning
    DWORD dwError;
    if(dwPtr == INVALID_SET_FILE_POINTER \
      && (dwError = GetLastError())!=NO_ERROR)return Status::IOError("Set File Pointer Failed");
    SetEndOfFile(fhandle_);
    file_end_ = offset + size;
    return Status::OK();
  }
  Status Read(size_t offset, size_t size, char* alloc_ptr) {
    mhandle_ = CreateFileMapping(fhandle_, 
      NULL,  // security
      PAGE_READWRITE, 
      0,  // higher dword
      offset,  // lower dword
      NULL); // map name
    if(mhandle_ == INVALID_HANDLE_VALUE)return Status::IOError("Map Invalid");
    map_ptr_ = (char*)MapViewOfFile(mhandle_, 
      FILE_MAP_ALL_ACCESS, 
      0,  // higher dword
      offset,  // lower dword
      size);
    if(map_ptr_ == NULL)return Status::IOError("Map View Invalid");
    memcpy(alloc_ptr, map_ptr_, size);
    if(!UnmapViewOfFile(map_ptr_))return Status::IOError("Map View Close Failed");
    if(!CloseHandle(mhandle_))return Status::IOError("Map Handle Close Failed");
    return Status::OK();
  }
  Status Flush(size_t offset, size_t size, char* data_ptr) {
    mhandle_ = CreateFileMapping(fhandle_, 
      NULL,  // security
      PAGE_READWRITE, 
      0,  // higher dword
      offset+size,  // lower dword
      NULL); // map name
    if(mhandle_ == INVALID_HANDLE_VALUE)return Status::IOError("Map Invalid");
    map_ptr_ = (char*)MapViewOfFile(mhandle_, 
      FILE_MAP_ALL_ACCESS,
      0,  // higher dword
      offset, // lower dword
      size);
    if(map_ptr_ == NULL)return Status::IOError("Map View Invalid");
    memcpy(map_ptr_, data_ptr, size);
    if(!UnmapViewOfFile(map_ptr_))return Status::IOError("Map View Close Failed");
    if(!CloseHandle(mhandle_))return Status::IOError("Map Handle Close Failed");
    return Status::OK();
  }
  Status Close(void) {
    if(!CloseHandle(fhandle_))return Status::IOError("File Close Failed");
    return Status::OK();
  }
 private:
  // char* Map(size_t offset, size_t size);

};

#endif


#if defined(__linux)

class SequentialFile : public WritableFile{

};

class RandomAccessFile : public WritableFile{

};

#endif

}