#include <gtest/gtest.h>


#include "./storage/page_manager.h"


#include <string>

using namespace std;
using namespace sbase;

TEST(PageManagerTest, NewCloseFileTest){
	PageManager pager;
	for(int i = 0; i<100; i++){
		string name = string("pager_test")+to_string(i);
		FileMeta meta(name.c_str(), 200, kSequential);
		FileHandle ret;
		EXPECT_TRUE(pager.NewFile(meta, ret).ok());
		EXPECT_TRUE(pager.CloseFile(ret).ok());
		EXPECT_TRUE(pager.DeleteFile(ret).ok());
	}
}

TEST(PageManagerTest, WriteReadFileTest){
	size_t len = 200;
	PageManager pager;
	FileMeta meta("testIO", len, kSequential);
	FileHandle file;
	ASSERT_TRUE(pager.NewFile(meta, file).ok());

	char* data = new char[len];
	memset(data, 1, len);
	PageHandle page;
	Status status;

	status = pager.New(file,kBflowTablePage, page);	
	if(!status.ok()){
		cout << status.ToString() << endl << flush;
		exit(0);
	}
	status = pager.Write(page, data, len);
	if(!status.ok()){
		cout << status.ToString() << endl << flush;
		exit(0);
	}
	status = pager.Flush(page);
	if(!status.ok()){
		cout << status.ToString() << endl << flush;
		exit(0);
	}

	char* ret;
	EXPECT_TRUE(pager.Read(page, ret).ok());
	for(int i = 0; i<len; i++)EXPECT_EQ(data[i], ret[i]);
	ASSERT_TRUE(pager.CloseFile(file).ok());
	ASSERT_TRUE(pager.DeleteFile(file).ok());
}

