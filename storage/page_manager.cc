#include "./storage/page_manager.h"

namespace sbase{

// File Interface //
Status PageManager::NewFile(std:string name, FileHandle& hFile){
  FileHandle fhandle = GetAvailableFileHandle();
  FilePtr f = make_shared<SequentialFile>(name);
  if(!f->Open().ok())return Status::IOError("Cannot Open Sequential File");
  file_.Insert(fhandle, f);
  hFile = fhandle;
  return Status::OK();
}
Status PageManager::CloseFile(FileHandle hFile){
  FilePtr f = nullptr;
  if(!file_.Get(hFile, f))return Status::InvalidArgument("File handle invalid.");

  // handle in memory page //

  return f->Close();
}
Status PageManager::DeleteFile(FileHandle hFile){
  FilePtr f = nullptr;
  if(!file_.Get(hFile, f))return Status::InvalidArgument("File handle invalid.");

  // handle in memory page //

  return f->Delete();
}
// Construct Page //
Status PageManager::NewPage(FileHandle hFile, PageType type, PageHandle& hPage){
  FileWrapperPtr f = nullptr;
  auto bool_ret = file_.Get(fh, f);
  if(bool_ret && !f){
    return Status::Corruption("File corrupted.");
  }
  else if(!bool_ret){
    return Status::InvalidArgument("File not exist.");
  }
  else{
    LocalPageHandle free = f->GetFree();
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
    return Status::OK();
  }
}
Status PageManager::DeletePage(PageHandle hPage){

}
// Sync on Page //
Status PageManager::SyncFromFile(PageHandle hPage){
  PagePtr p = GetPage(hPage);
  if(!p)return Status::InvalidArgument("Page not found.");
  // file is newer
  if(p->commited > p->modified)return PoolFromDisk(hPage);
  return Status::OK();
}
Status PageManager::SyncFromMem(PageHandle hPage){
  PagePtr p = GetPage(hPage);
  if(!p)return Status::InvalidArgument("Page not found.");
  // mem is newer
  if(p->commited < p->modified)return FlushToDisk(hPage);
  return Status::OK();
}
// Memory //
Status PageManager::Pool(PageHandle hPage){
  PagePtr p = nullptr;
  if(pool_.Get(hPage,p)){
    return Status::OK();
  }
  // file -> mempool //
  // do LRU here //
}
Status PageManager::Expire(PageHandle hPage){
  auto ret = SyncFromMem(hPage);
  if(!ret.ok())return ret;
  // mempool expire //
}
// Accessor //
size_t PageManager::GetFileSize(FileHandle hFile){

}
size_t PageManager::GetPageSize(PageHandle hPage){

}
char* PageManager::GetPageDataPtr(PageHandle hPage){

}
// protected
Latch* GetPageLatch(PageHandle hPage){
  PagePtr p = GetPage(hPage);
  if(!p)return Status::InvalidArgument("Page not found.");
  return p->latch;
}
Latch* GetFileLatch(FileHandle hFile){
  FilePtr f = GetFile(hFile);
  if(!f)return Status::InvalidArgument("File not found.");
  return f->latch;
}
// private
PagePtr PageManager::GetPage(PageHandle hPage){

}
FilePtr PageManager::GetFile(FileHandle hFile){

}
FileWrapperPtr PageManager::GetFileWrapperPtr(FileHandle hFile){

}

// private Mem~File //
Status PageManager::FlushToDisk(PageHandle hPage){
  FileWrapperPtr f = nullptr;
  file_.Get(hPage, f);
  PagePtr p = nullptr;
  if(!f)return Status::InvalidArgument("File not exist.");
  if(!(p = (*f)[hPage]))return Status::InvalidArgument("Page not exist.");
  if(!pool_.inPool(hPage)){
    assert(p->commited >= p->modified); // page on disk is new enough
    return Status::OK();
  }
  else if(p->commited < p->modified){ // need flush
    // write disk using pooling ptr
    char* data = pool_.Get(hPage);
    return f->file->Write(data);
  }
}
Status PageManager::PoolFromDisk(PageHandle hPage){
  FileWrapperPtr f = nullptr;
  file_.Get(hPage, f);
  PagePtr p = nullptr;
  if(!f)return Status::InvalidArgument("File not exist.");
  if(!(p = (*f)[hPage]))return Status::InvalidArgument("Page not exist.");
  if(pool_.inPool(hPage)){
    assert(p->commited <= p->modified); // page in pool is newer than disk
  }
  else{
    // get ptr from pool
    // read from disk
  }
  return Status::OK();
}



} // namespace sbase