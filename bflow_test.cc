#include <gtest\gtest.h>
#include "b_flow.h"
#include "frontend_mock.hpp"

using namespace std;
using namespace sbase;

TEST(BPlusCursorTest, DescendTest){
	FrontMock mocker;
	PageManager* pager = (mocker.get_pager());
	Type* type = mocker.get_type();
	BPlusCursor cursor(pager, type);
	cursor.MoveTo(0);
	int key = 2;
	char* key_ptr = reinterpret_cast<char*>(&key);
	cursor.Descend(key_ptr);
	PageHandle exp = 4;
	EXPECT_EQ(cursor.current(), exp);
}
