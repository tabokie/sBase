#include "./storage/file.h"

namespace sbase{

#if defined(__WIN32) || defined(__WIN64)

// Base Type :: Writabel File //
Status WritableFile::Open(void){
  fhandle_ = CreateFile(fileName.c_str(), 
    GENERIC_READ | GENERIC_WRITE, 
    0,  // share mode
    NULL,  // security
    OPEN_ALWAYS, 
    FILE_ATTRIBUTE_NORMAL, 
    NULL); // template file handle
  if(fhandle_ == INVALID_HANDLE_VALUE)return Status::IOError("Invalid File Handle");
  file_end_ = GetFileSize(fhandle_, NULL);
  return Status::OK();
}

Status WritableFile::Close(void){
  if(!CloseHandle(fhandle_))return Status::IOError("Close File Failed");
  return Status::OK();
}

Status WritableFile::Delete(void){
  if(!DeleteFile(fileName.c_str())){
    std::cout << "<" << GetLastError() << ">" << std::endl;
    return Status::IOError("Cannot Delete File");
  }
  return Status::OK();
}

// Derived Type :: Sequential File //
Status SequentialFile::Read(size_t offset, size_t size, char* alloc_ptr){
  if(!alloc_ptr)return Status::InvalidArgument("Null data pointer.");
  if(offset + size > file_end_)return Status::InvalidArgument("Exceed file length.");
  DWORD dwPtr = SetFilePointer(fhandle_, 
    offset, 
    NULL, 
    0); // 0 for starting from beginning
  DWORD dwError;
  if(dwPtr == INVALID_SET_FILE_POINTER \
    && (dwError = GetLastError())!=NO_ERROR)return Status::IOError("Set File Pointer Failed");
  bool rfRes = ReadFile(fhandle_, 
    alloc_ptr, 
    size, 
    NULL, // num of bytes read
    NULL); // overlapped structure
  if(!rfRes)return Status::IOError("Read File Failed");
  return Status::OK();
}
Status SequentialFile::Write(size_t offset, size_t size, char* data_ptr) {
  if(!data_ptr)return Status::InvalidArgument("Null data pointer.");
  if(offset >= file_end_)return Status::OK();
  if(offset + size >= file_end_)size = file_end_;
  DWORD dwPtr = SetFilePointer(fhandle_, 
    offset, 
    NULL, 
    0); // 0 for starting from beginning
  DWORD dwError;
  if(dwPtr == INVALID_SET_FILE_POINTER \
    && (dwError = GetLastError())!=NO_ERROR)return Status::IOError("Set File Pointer Failed");
  bool rfRes = WriteFile(fhandle_, 
    data_ptr, 
    size, 
    NULL,  // num of bytes read
    NULL); // overlapped structure
  if(!rfRes)return Status::IOError("Write File Failed");
  return Status::OK();
}

Status SequentialFile::SetEnd(size_t offset) {
  DWORD dwPtr = SetFilePointer(fhandle_, \
    offset, \
    NULL, \
    0); // 0 for starting from beginning
  DWORD dwError;
  if(dwPtr == INVALID_SET_FILE_POINTER \
    && (dwError = GetLastError())!=NO_ERROR)return Status::IOError("Set File Pointer Failed");
  SetEndOfFile(fhandle_);
  file_end_ = offset;
  // SetFileValidData(fhandle_, offset+block_size_)
  return Status::OK();
}

/*
// Derived Type :: RandomAccessFile //

Status RandomAccessFile::Read(size_t offset, char* alloc_ptr) {
  return Read_(offset, alloc_ptr, file_.block_size);
}
Status RandomAccessFile::Read(size_t offset, char* alloc_ptr, size_t size) {
  if(size > file_.block_size)size = file_.block_size;
  return Read_(offset, alloc_ptr, size);
}
Status RandomAccessFile::Read_(size_t offset, char* alloc_ptr, size_t size) {
  mhandle_ = CreateFileMapping(fhandle_, 
    NULL,  // security
    PAGE_READWRITE, 
    0,  // higher dword
    offset,  // lower dword
    NULL); // map name
  if(mhandle_ == INVALID_HANDLE_VALUE)return Status::IOError("Map Invalid");
  char* map_ptr_ = (char*)MapViewOfFile(mhandle_, 
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
Status RandomAccessFile::Flush(size_t offset, char* data_ptr){
  return Flush_(offset, data_ptr, file_.block_size); 
}
Status RandomAccessFile::Flush(size_t offset, char* data_ptr, size_t size) {
  if(size > file_.block_size)size = file_.block_size;
  return Flush_(offset, data_ptr, size);
}
Status RandomAccessFile::Flush_(size_t offset, char* data_ptr, size_t size) {
  mhandle_ = CreateFileMapping(fhandle_, 
    NULL,  // security
    PAGE_READWRITE, 
    0,  // higher dword
    offset+size,  // lower dword
    NULL); // map name
  if(mhandle_ == INVALID_HANDLE_VALUE)return Status::IOError("Map Invalid");
  char* map_ptr_ = (char*)MapViewOfFile(mhandle_, 
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
Status RandomAccessFile::Append(size_t offset){
  DWORD dwPtr = SetFilePointer(fhandle_, \
    offset+file_.block_size, \
    NULL, \
    0); // 0 for starting from beginning
  DWORD dwError;
  if(dwPtr == INVALID_SET_FILE_POINTER \
    && (dwError = GetLastError())!=NO_ERROR)return Status::IOError("Set File Pointer Failed");
  SetEndOfFile(fhandle_);
  file_end_ = offset + file_.block_size;
  return Status::OK();
}
*/

#endif // __WINXX

#if defined(__linux)

class SequentialFile : public WritableFile{

};

class RandomAccessFile : public WritableFile{

};

#endif // __linux

}