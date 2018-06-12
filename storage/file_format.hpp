#ifndef SBASE_STORAGE_FILE_FORMAT_HPP_
#define SBASE_STORAGE_FILE_FORMAT_HPP_

#include <cstdint>
#include <string>


namespace sbase{

// Encoding File & Page //
typedef uint8_t FileHandle;
const size_t kFileHandleWid = 1;
typedef uint32_t PageHandle;
const size_t kPageHandleWid = 4;
typedef uint32_t PageNum;
const size_t kPageNumWid = 3;
#define PageNumMask 									(0xffffff)
#define GetPageHandle(hFile, nPage)		(static_cast<PageHandle>((hFile)<<(kPageNumWid*8)) + static_cast<PageHandle>(nPage) )
#define GetPageNum(hPage) 						(static_cast<PageNum>(hPage & 0xffffff))
#define GetFileHandle(hPage) 					(static_cast<FileHandle>(hPage >> (kPageNumWid*8)))
const size_t kFileHeaderLength = (10);
const size_t kBlockHeaderLength = (2);

enum PageType{
	kUnknownPage = 0,
	kDatabaseRoot = 1,
	kTableRoot = 2,
	kBFlowPage = 3,
	kBPlusPage = 4
};

enum IndexType{
	kBFlowIndex = 0,
	kBPlusIndex = 1,
	kBIndex = 2
};


#pragma pack(1)
struct FileHeader{ // header do not take up a page, page encoded from OffsetBytes
	FileHandle hFileCode; // 0 for database core
	uint8_t hFileBlockSize; // K
	uint32_t hOffsetBytes; // page start
};
struct ManifestBlockHeader{
	uint8_t hBlockCode; // common field
	uint16_t oManifest0;
	uint8_t nManifest0; // number of record
	uint16_t oManifest1;
	uint8_t nManifest1;
	uint8_t hVoid;
};
struct BlockHeader{
	uint8_t hBlockCode;
	uint8_t hVoid;
};
struct BFlowHeader{
	uint16_t nSize; // number of slice, data+oTop*stripe is end of valid data
	PageHandle hPri;
	PageHandle hNext; // point back if end of cluster
};
struct BPlusHeader{
	uint16_t nSize;
	PageHandle hRight;
};
/*
struct BFlowSectionBHeader{
	uint8_t oHead;
	uint8_t oTail;
	PageHandle hNext;
};
struct BFlowSectionAHeader{
	uint8_t nSize;
};
*/
#pragma pack()

const size_t kBlockSize = 4; // use KB as internal unit
const size_t kBlockLen = kBlockSize * 1024;
const size_t kBFlowHeaderLen = sizeof(BFlowHeader);
const size_t kBFlowDataLen = kBlockLen - kBFlowHeaderLen;

// Database root page stores
const std::string kDatabaseRootPath = "root";
const FileHandle kDatabaseRootFile = 0;
const PageNum kDatabaseRootPageNum = 1;


} // namespace sbase


#endif // SBASE_STORAGE_FILE_FORMAT_HPP_

