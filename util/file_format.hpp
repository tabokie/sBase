#ifndef SBASE_STORAGE_FILE_FORMAT_HPP_
#define SBASE_STORAGE_FILE_FORMAT_HPP_

#include <cstdint>


namespace sbase{

// Encoding File //
typedef uint8_t FileHandle;
const size_t kFileHandleWid = 1;
typedef uint32_t PageHandle;
const size_t kPageHandleWid = 4;
typedef uint32_t PageNum;
const size_t kPageNumWid = 3;
#define GetPageHandle(hFile, nPage)		(static_cast<PageHandle>(hFile)<<kLocalPageHandleWid + static_cast<PageHandle>(nPage) )
#define GetPageNum(hPage) 						(static_cast<PageNum>(hPage & 0xffffff))
#define GetFileHandle(hPage) 					(static_cast<FileHandle>(hPage >> 24))
#pragma pack(1)
struct FileHeader{
	FileHandle hFileCode; // 0 for database core
	uint8_t hFileBlockSize; // K
	uint32_t hOffsetBytes;
	uint32_t hRootOffsetBytes; // only needed for 0
}
struct BlockHeader{
	uint8_t hBlockCode;
	uint8_t hVoid;
}
#pragma pack()

} // namespace sbase


#endif // SBASE_STORAGE_FILE_FORMAT_HPP_

