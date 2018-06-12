#include <gtest/gtest.h>


#include "./storage/page_manager.h"
#include "./storage/page_ref.hpp"

#include <string>

using namespace std;
using namespace sbase;

TEST(PageManagerTest, FileTest){
	PageManager pager;
	for(int i = 0; i<10; i++){
		string name = string("pager_test")+to_string(i);
		FileHandle ret;
		EXPECT_TRUE(pager.NewFile(name, 4, ret).ok());
		EXPECT_TRUE(pager.DeleteFile(ret).ok());
	}
}

TEST(PageManagerTest, PageTest){
	size_t len = 4096;
	PageManager pager;
	FileHandle file;
	ASSERT_TRUE(pager.NewFile("test_io", 4, file).ok());

	char* data = new char[len];
	memset(data, 1, len);
	PageHandle page;
	Status status;

	status = pager.NewPage(file,kBFlowPage, page);	
	if(!status.ok()){
		cout << status.ToString() << endl << flush;
		exit(0);
	}
	status = pager.DirectWrite(page, data);
	if(!status.ok()){
		cout << status.ToString() << endl << flush;
		exit(0);
	}
	ASSERT_TRUE(pager.CloseFile(file).ok());
	ASSERT_TRUE(pager.OpenFile("test_io", file).ok());
	page = GetPageHandle(file, 1);
	char* ret;
	PageRef* ref = new PageRef(&pager, page, kReadOnly);
	ret = ref->ptr;
	for(int i = 0; i<len; i++)EXPECT_EQ(data[i], ret[i]);	
	delete ref;
	delete [] data;
	ASSERT_TRUE(pager.DeleteFile(file).ok());
}
TEST(PageManagerTest, PressureTest){
	size_t len = 4096;
	PageManager pager;
	FileHandle file;
	ASSERT_TRUE(pager.NewFile(string("pressure"), 4, file).ok());
	for(int i = 0; i < 998; i++){
		PageHandle page;
		ASSERT_TRUE(pager.NewPage(file, kBFlowPage, page).ok());
		auto ret = pager.Pool(page);
		ASSERT_TRUE(ret.ok());
	}
	for(int i = 0; i < 100; i++){
		EXPECT_TRUE(pager.DeletePage(i+1).ok());
	}
	auto ret = pager.DeleteFile(file);
	EXPECT_TRUE(ret.ok());
}

void ParallelRefTest_read_thread(void){

}
void ParallelRefTest_write_thread(void){

}
void ParallelRefTest_fatal_write_thread(void){

}

TEST(PageManagerTest, ParallelRefTest){

}
