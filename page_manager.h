namespace sbase{

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
  PageHandle NewPage(FileHandle file, size_t size);
 private:
  NewSequentialPage(FileHandle file, size_t size);


};

} // namespace sbase

PageManager::ReadPage(PageHandle page){

}

class WritableFile{
 public:
  WritableFile(){ }
  virtual ~WritableFile();
  virtual Status Open(string filename);
  virtual Status Read(char* alloc_ptr, size_t offset);
  virtual Status Append(char* data_ptr, size_t offset);
  virtual Status Flush(char* data_ptr, size_t offset);
  virtual Status Close(void);
};


class SequentialFile : public WritableFile{
 private:
  std::string filename_;
  OsFileHandle fhandle_;
  size_t size_;
  size_t block_size_;
 public:
  Status Open(string filename) override;
  Status Read(char* alloc_ptr, size_t offset) override;
  Status Flush(char* data_ptr, size_t offset) override;
  Status Append(char* data_ptr, size_t offset) override;
  Status Close(void) override;
 private:
  Status AppendSpace(size_t offset);
}

class WinRandomAccessFile : public WritableFile{
 private:
  std::string filename_;
  OsFileHandle fhandle_;
  OsMapHandle mhandle_;
  char* map_ptr;
 public:
  Status Open(string filename) override;
  Status Read(char* alloc_ptr) override;
  Status Flush(char* data_ptr) override;
  Status Close(void) override;
 private:
  Status Map(string map_name);

}

class LinuxRandomAccessFile : public WritableFile{

}

