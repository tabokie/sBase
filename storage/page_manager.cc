#include "./storage/page_manager.h"

namespace sbase{


// File Interface //

Status PageManager::NewFile(std::string name, uint8_t block, FileHandle& hFile){
  // allocate handle
  FileHandle fhandle = GetAvailableFileHandle();
  // create file and allocate header
  FilePtr f = make_shared<SequentialFile>(name);
  if(!f->Open().ok())return Status::IOError("Cannot Open Sequential File");
  if(!f->SetEnd(kFileHeaderLength).ok())return Status::IOError("Cannot set file size.");
  // edit header and assign to file
  FileHeader newFileHeader;
  newFileHeader.hFileCode = fhandle;
  newFileHeader.hFileBlockSize = block;
  newFileHeader.hRootOffsetBytes = 0;
  newFileHeader.hOffsetBytes = kFileHeaderLength;
  auto ret = f->Write(0, kFileHeaderLength, reinterpret_cast<char*>(&newFileHeader));
  if(!ret.ok()){
    f->Close();
    return ret;
  }
  // create wrapper and edit attr
  FileWrapperPtr fw = make_shared<FileWrapper>(f);
  fw->blockSize = block * 1024;
  fw->hFile = fhandle;
  fw->fileName = name;
  fw->dataOffset = newFileHeader.hOffsetBytes;
  // insert into synchronic HashMap
  file_.Insert(fhandle, fw);
  // return file handle
  hFile = fhandle;
  return Status::OK();
}
Status PageManager::OpenFile(std::string name, FileHandle& hFile){
  // create file and read header
  FilePtr f = make_shared<SequentialFile>(name);
  if(!f->Open().ok())return Status::IOError("Cannot Open Sequential File");
  if(f->size() < kFileHeaderLength){
    f->Close();
    return Status::IOError("File too short to contain proper header.");
  }
  FileHeader fileHeader;
  auto ret = f->Read(0, kFileHeaderLength, reinterpret_cast<char*>(&fileHeader));
  if(!ret.ok()){
    f->Close();
    ret;
  }
  // create wrapper and assign header to wrapper
  FileWrapperPtr fw = make_shared<FileWrapper>(f);
  fw->blockSize = fileHeader.hFileBlockSize * 1024;
  fw->hFile = fileHeader.hFileCode;
  fw->fileName = name;
  fw->dataOffset = fileHeader.hOffsetBytes;
  // insert to synchronic HashMap
  file_.Insert(fw->hFile, fw);
  // return handle
  hFile = fw->hFile;
  // encode existing pages
  size_t pageSize = (f->size() - fileHeader.hOffsetBytes ) / fw->blockSize;
  for(int i = 1; i <= pageSize; i++)
    fw->AddPage(make_shared<Page>(*(fw->file)));
  return Status::OK();
}
Status PageManager::CloseFile(FileHandle hFile){
  // get file wrapper
  FileWrapperPtr fw = nullptr;
  if(!(fw=GetFileWrapper(hFile)))return Status::InvalidArgument("File handle invalid.");
  // handle in memory page
  Status expire_ret = Status::OK();
  // traverse pages
  // sync on file
  fw->latch.WeakWriteLock();
  for(auto& p : fw->pages){
    if(p){
      // sync on page
      p->latch.WriteLock();
      // expire from pool
      auto ret = Expire(p->handle, false);
      if(!ret.ok())expire_ret = ret;
    }
  }
  if(!expire_ret.ok())return expire_ret;
  // delete pages, along with locks
  fw->pages.clear();
  // close file
  auto ret = fw->file->Close();
  if(!ret.ok())return ret;
  // delete wrapper from HashMap, along with lock
  auto bool_ret = file_.Delete(hFile);
  if(!bool_ret)return Status::IOError("Hash map corrupted.");
  return Status::OK();
}
Status PageManager::DeleteFile(FileHandle hFile){
  // get wrapper
  FileWrapperPtr fw = nullptr;
  if(!(fw=GetFileWrapper(hFile)))return Status::InvalidArgument("File handle invalid.");
  // handle in memory page //
  Status expire_ret = Status::OK();
  // traverse pages
  // sync on file
  fw->latch.WriteLock();
  for(auto& p : fw->pages){
    if(p){
      // sync on page
      p->latch.WriteLock();
      // expire from pool
      auto ret = Expire(p->handle, false);
      if(!ret.ok())expire_ret = ret;
    }
  }
  if(!expire_ret.ok())return expire_ret;
  // delete pages, along with locks
  fw->pages.clear();
  // delete file
  auto ret = fw->file->Close(); // close first
  if(!ret.ok())return ret;
  ret = fw->file->Delete();
  if(!ret.ok())return ret;
  // delete wrapper from HashMap
  auto bool_ret = file_.Delete(hFile);
  if(!bool_ret)return Status::IOError("Hash map corrupted.");
  return Status::OK();
}
// Construct Page //
Status PageManager::NewPage(FileHandle hFile, PageType type, PageHandle& hPage){ // append file if needed
  // get wrapper
  FileWrapperPtr fw = nullptr;
  if(!(fw=GetFileWrapper(hFile)))return Status::InvalidArgument("File handle invalid.");

  // get free means initial a complete page instance
  PageNum free = fw->GetFree();
  if(free == 0){
    if(!fw->file)return Status::Corruption("File pointer corrupted.");
    // usable size
    size_t fsize = fw->file->size() - fw->dataOffset;
    // new end
    size_t fileEnd = (fsize / fw->blockSize + 1) * fw->blockSize + fw->dataOffset;
    fw->file->SetEnd(fileEnd);
    // add new page
    hPage = fw->AddPage(make_shared<Page>(*(fw->file)));
  }
  else{ // meaning acquire a new page
    hPage = GetPageHandle(hFile, free);
  }

  return Status::OK();
}
Status PageManager::DeletePage(PageHandle hPage){
  Latch* pageLock = GetPageLatch(hPage);
  if(!pageLock)return Status::Corruption("lock invalid.");
  pageLock->WriteLock();
  // save change
  auto ret = Expire(hPage, false);
  if(!ret.ok())return ret;
  // get wrapper
  FileWrapperPtr fw = GetFileWrapper(GetFileHandle(hPage));
  if(!fw)return Status::InvalidArgument("Unknown file.");
  if(!(fw->DeletePage(hPage))){
    pageLock->ReleaseWriteLock();
    return Status::Corruption("Cannot delete page.");
  }
  // else lock destroy
  return Status::OK();
}
// Sync on Page //
Status PageManager::SyncFromFile(PageHandle hPage, bool lock){
  PagePtr p = GetPage(hPage);
  if(!p)return Status::InvalidArgument("Page not found.");
  // file is newer
  if(p->commited > p->modified)return PoolFromDisk(hPage, lock);
  return Status::OK();
}
Status PageManager::SyncFromMem(PageHandle hPage, bool lock){
  PagePtr p = GetPage(hPage);
  if(!p)return Status::InvalidArgument("Page not found.");
  // mem is newer
  if(p->commited < p->modified)return FlushFromPool(hPage, lock);
  return Status::OK();
}
Status PageManager::DirectWrite(PageHandle hPage, char* data, bool lock){
  return FlushFromPtr(hPage, data, lock);
}
// Memory //
Status PageManager::Pool(PageHandle hPage, bool lock){ // no care for sync
  if(pool_.Get(hPage)){
    return Status::OK();
  }
  return PoolFromDisk(hPage, lock);
}
Status PageManager::Expire(PageHandle hPage, bool lock){
  Latch* pageLock = nullptr;
  if(lock){
    pageLock = GetPageLatch(hPage);
    if(!pageLock)return Status::Corruption("lock invalid.");
    pageLock->WriteLock(); // strong
  }
  auto ret = SyncFromMem(hPage, false);
  if(ret.ok())ret = pool_.Delete(hPage);
  // mempool expire //
  if(lock)pageLock->ReleaseWriteLock();
  return ret;
}

// private Mem~File //
// can read, no write
// page being commited
Status PageManager::FlushFromPool(PageHandle hPage, bool lock){ 
  char* ptr = pool_.Get(hPage);
  if(!ptr)return Status::OK();                
  FileWrapperPtr fw = nullptr;
  fw = GetFileWrapper(GetFileHandle(hPage));
  if(!fw)return Status::InvalidArgument("Unknown file.");
  size_t size = GetPageSize(hPage);
  size_t offset = GetPageOffset(hPage);
  Latch* pageLock = nullptr;
  PagePtr p = GetPage(hPage);
  if(lock){
    pageLock = &(p->latch);
    if(!pageLock)return Status::Corruption("Cannot acquire lock.");
    pageLock->WeakWriteLock();
  }
  p->Commit();
  auto ret = fw->file->Write(offset, size, ptr);
  if(lock)pageLock->ReleaseWeakWriteLock();
  return ret;
}
Status PageManager::FlushFromPtr(PageHandle hPage, char* data, bool lock){
  if(!data)return Status::InvalidArgument("Invalid data pointer.");
  FileWrapperPtr fw = nullptr;
  fw = GetFileWrapper(GetFileHandle(hPage));
  if(!fw)return Status::InvalidArgument("Unknown file.");
  size_t size = GetPageSize(hPage);
  size_t offset = GetPageOffset(hPage);
  Latch* pageLock = nullptr;
  PagePtr p = GetPage(hPage);
  if(lock){
    pageLock = &(p->latch);
    if(!pageLock)return Status::Corruption("Cannot acquire lock.");
    pageLock->WeakWriteLock();
  }
  p->Commit();
  auto ret = fw->file->Write(offset, size, data);
  if(lock)pageLock->ReleaseWeakWriteLock();
  return ret;
}
// pool anyway
// no read nor write
Status PageManager::PoolFromDisk(PageHandle hPage, bool lock){ 
  char* p = nullptr;
  size_t newSize = GetPageSize(hPage);
  size_t offset = GetPageOffset(hPage);
  if(!(p = pool_.Get(hPage))){
    // alloc mem //
    auto ret = pool_.Add(hPage, newSize, p);
    if(ret.IsIOError() || !p){
      // do LRU here //
      uint64_t bestRank = 0;
      PageHandle bestPage;
      PageHandle curPage;
      PagePtr p;
      for(int i = 0; i < kLruIterationMax; i++){
        curPage = pool_.IterateHandleNext();
        if(curPage == 0)break;
        p = GetPage(curPage);
        if(p && bestRank < p->Rank()){
          bestRank = p->Rank();
          bestPage = curPage;
        } 
      }
      if(bestRank == 0)return Status::IOError("Not enough memory.");
      ret = Expire(bestPage);
      if(!ret.ok())return Status::IOError("Not enough memory.");
    }
    else if(!ret.ok())return ret;
  }
  // file -> mempool //
  FileWrapperPtr fw = GetFileWrapper(GetFileHandle(hPage));
  if(!fw)return Status::InvalidArgument("Unknown file.");
  Latch* pageLock = nullptr;
  PagePtr pg = GetPage(hPage);
  if(lock){
    pageLock = &(pg->latch);
    if(!pageLock)return Status::Corruption("Cannot acquire lock.");
    pageLock->WriteLock();
  }
  pg->Modify();
  auto ret = fw->file->Read(offset, newSize, p);
  if(lock)pageLock->ReleaseWriteLock();
  if(!ret.ok())return ret;  
  return Status::OK();
}



} // namespace sbase