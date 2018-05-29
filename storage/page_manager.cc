#include "./storage/page_manager.h"

namespace sbase{

GetAvailableFileHandle(void);


// File Interface //
Status PageManager::NewFile(std:string name, size_t fileSize, FileHandle& hFile){
  FileHandle fhandle = GetAvailableFileHandle();
  FilePtr f = make_shared<SequentialFile>(name);
  if(!f->Open().ok())return Status::IOError("Cannot Open Sequential File");
  if(!f->SetEnd(fileSize).ok())return Status::IOError("Cannot set file size.");
  file_.Insert(fhandle, f);
  hFile = fhandle;
  return Status::OK();
}
Status PageManager::OpenFile(std::string name, FileHandle& hFile){
  FilePtr f = make_shared<SequentialFile>(name);
  if(!f->Open().ok())return Status::IOError("Cannot Open Sequential File");
  file_.Insert(fhandle, f);
  hFile = fhandle;
  return Status::OK();
}
Status PageManager::CloseFile(FileHandle hFile){
  FileWrapperPtr fw = nullptr;
  if(!(fw=GetFileWrapper(hFile)))return Status::InvalidArgument("File handle invalid.");
  fw->latch.WriteLock();
  // handle in memory page //
  Status expire_ret = Status::OK();
  for(auto& p : fw->pages){
    if(p){
      p->latch.WriteLock();
      auto ret = Expire(p->handle);
      if(!ret.ok())expire_ret = ret;
    }
  }
  fw->pages.clear();
  fw->free_index = 0;
  fw->free_size = 0;
  fw->size = 0;
  fw->fileOpened = false;
  auto ret = fw->file->Close();
  fw->latch.ReleaseWriteLock();
  if(!ret.ok())return ret;
  return Status::OK();
}
Status PageManager::DeleteFile(FileHandle hFile){
  FileWrapperPtr fw = nullptr;
  if(!(fw=GetFileWrapper(hFile)))return Status::InvalidArgument("File handle invalid.");
  fw->latch.WriteLock();
  // handle in memory page //
  Status expire_ret = Status::OK();
  for(auto& p : fw->pages){
    if(p){
      p->latch.WriteLock();
      auto ret = Expire(p->handle);
      if(!ret.ok())expire_ret = ret;
    }
  }
  fw->pages.clear();
  fw->free_index = 0;
  fw->free_size = 0;
  fw->size = 0;
  fw->fileOpened = false;
  auto ret = fw->file->Delete();
  if(!ret.ok())return ret;
  auto bool_ret = file_.Delete(hFile);
  if(!bool_ret)return IOError("Hash map corrupted.");
  return Status::OK();
}
// Construct Page //
Status PageManager::NewPage(FileHandle hFile, PageType type, PageHandle& hPage){ // append file if needed
  FileWrapperPtr f = nullptr;
  auto bool_ret = file_.Get(fh, f);
  if(bool_ret && !f){
    return Status::Corruption("File corrupted.");
  }
  else if(!bool_ret){
    return Status::InvalidArgument("File not exist.");
  }
  else{
    Latch* fileLock = GetFileLatch(hFile);
    if(!fileLock)return Status::Corruption("Cannot acquire file lock.");
    fileLock->WeakWriteLock(); // read control by size

    PageNum free = f->GetFree();
    if(free == 0){
      if(!f->file)return Status::Corruption("File pointer corrupted.");
      size_t fsize = f->file->GetSize();
      size_t blocksize = f->file->GetBLockSize();
      fsize = (fsize / blocksize + 1) * blocksize;
      f->file->SetEnd(fsize);
      // add new page
      ret = f->AddPage(make_shared<Page>(*(f->file)));
    }
    else{
      ret = GetPageHandle(fh, free);
    }

    fileLock->ReleaseWeakWriteLock();
    return Status::OK();
  }
}
Status PageManager::DeletePage(PageHandle hPage){
  auto ret = SyncFromMem(hPage);
  if(!ret.ok())return ret;
  FileWrapperPtr fw = GetFileWrapper(GetFileHandle(hPage));
  if(!fw)return InvalidArgument("Unknown file.");
  fw->size--; // decr to use only weak lock
  fw->latch.WeakWriteLock();
  (*fw)[hPage]->WriteLock();
  (*fw)[hPage] = nullptr;
  fw->latch.ReleaseWeakWriteLock();
  return Status::OK();
}
// Sync on Page //
Status PageManager::SyncFromFile(PageHandle hPage, bool lock = true){
  PagePtr p = GetPage(hPage);
  if(!p)return Status::InvalidArgument("Page not found.");
  // file is newer
  if(p->commited > p->modified)return PoolFromDisk(hPage, lock);
  return Status::OK();
}
Status PageManager::SyncFromMem(PageHandle hPage, bool lock = true){
  PagePtr p = GetPage(hPage);
  if(!p)return Status::InvalidArgument("Page not found.");
  // mem is newer
  if(p->commited < p->modified)return FlushToDisk(hPage, lock);
  return Status::OK();
}
Status PageManager::DirectWrite(PageHandle hPage, char* data, bool lock = true){
  if(!data)return Status::InvalidArgument("Invalid data pointer.");
  FileWrapperPtr fw = nullptr;
  fw = GetFileWrapper(GetFileHandle(hPage));
  if(!fw)return Status::InvalidArgument("Unknown file.");
  size_t size = GetPageSize(hPage);
  size_t offset = GetPageOffset(hPage);
  Latch* pageLock = nullptr;
  if(lock){
    pageLock = GetPageLatch(hPage);
    if(!pageLock)return Status::Corruption("Cannot acquire lock.");
    pageLock->WeakWriteLock();
  }
  auto ret = fw->file->Write(offset, size, ptr);
  if(lock)pageLock->ReleaseWeakWriteLock();
  return ret;
}
// Memory //
Status PageManager::Pool(PageHandle hPage, lock = true){ // no care for sync
  if(pool_.Get(hPage)){
    return Status::OK();
  }
  return PoolFromDisk(hPage, lock);
}
Status PageManager::Expire(PageHandle hPage, lock = true){
  auto ret = SyncFromMem(hPage, lock);
  if(!ret.ok())return ret;
  // mempool expire //
  return pool_.Delete(hPage);
}
// Accessor //
inline size_t PageManager::GetFileSize(FileHandle hFile){
  FilePtr f = GetFile(hFile);
  if(f)return f->size();
  return 0;
}
inline size_t PageManager::GetPageSize(PageHandle hPage){
  FilePtr f = GetFile(GetFileHandle(hPage));
  if(f)return f->blockSize();
  return 0;
}
inline char* PageManager::GetPageDataPtr(PageHandle hPage){ // no explicit pooling
  return pool_.Get(hPage);
}
// protected
// return pointer ?
inline Latch* GetPageLatch(PageHandle hPage){
  PagePtr p = GetPage(hPage);
  if(p)return &(p->latch);
  return nullptr;
}
inline Latch* GetFileLatch(FileHandle hFile){
  FilePtr f = GetFile(hFile);
  if(f)return &(f->latch);
  return nullptr;
}
// private
inline PagePtr PageManager::GetPage(PageHandle hPage){
  FileWrapperPtr fw = GetFileWrapper(GetFileHandle(hPage));
  PageNum no = GetPageNum(hPage);
  if(fw){
    return (*fw)[no];
  }
  return nullptr;
}
inline FilePtr PageManager::GetFile(FileHandle hFile){
  FileWrapperPtr fw = GetFileWrapper(hFile);
  if(fw)return fw->file;
  return nullptr;
}
inline FileWrapperPtr PageManager::GetFileWrapper(FileHandle hFile){
  FileWrapperPtr fw = nullptr;
  file_.Get(hFile, fw);
  return fw;
}

// private Mem~File //
// can read, no write
Status PageManager::FlushToDisk(PageHandle hPage, bool lock = true){ 
  char* ptr = pool_.Get(hPage);
  if(!ptr)return Status::OK();
  FileWrapperPtr fw = nullptr;
  fw = GetFileWrapper(GetFileHandle(hPage));
  if(!fw)return Status::InvalidArgument("Unknown file.");
  size_t size = GetPageSize(hPage);
  size_t offset = GetPageOffset(hPage);
  Latch* pageLock = nullptr;
  if(lock){
    pageLock = GetPageLatch(hPage);
    if(!pageLock)return Status::Corruption("Cannot acquire lock.");
    pageLock->WeakWriteLock();
  }
  auto ret = fw->file->Write(offset, size, ptr);
  if(lock)pageLock->ReleaseWeakWriteLock();
  return ret;
}
// pool anyway
// no read nor write
Status PageManager::PoolFromDisk(PageHandle hPage, bool lock = true){ 
  char* p = nullptr;    
  size_t newSize = GetPageSize(hPage);
  size_t offset = GetPageOffset(hPage);
  if(!(p = pool_.Get(hPage))){
    // alloc mem //
    auto ret = pool_.Add(hPage, newSize, p);
    if(ret.IsIOError() || !p){
      // do LRU here //
    }
    else if(!ret.ok())return ret;
  }
  // file -> mempool //
  FileWrapperPtr fw = GetFileWrapper(GetFileHandle(hPage));
  if(!fw)return Status::InvalidArgument("Unknown file.");
  Latch* pageLock = nullptr;
  if(lock){
    pageLock = GetPageLatch(hPage);
    if(!pageLock)return Status::Corruption("Cannot acquire lock.");
    pageLock->WriteLock();
  }
  ret = fw->file->Read(offset, newSize, p);
  if(lock)pageLock->ReleaseWriteLock();
  if(!ret.ok())return ret;  
  return Status::OK();
}



} // namespace sbase