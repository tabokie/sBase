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

	// Match test
	cursor.MoveTo(0);
	int key = 7;
	char* key_ptr = reinterpret_cast<char*>(&key);
	cursor.Descend(key_ptr);
	PageHandle exp0 = key*2;
	EXPECT_EQ(exp0, cursor.current());

	// Lower Bound Test
	cursor.MoveTo(0);
	key = 0;
	key_ptr = reinterpret_cast<char*>(&key);
	cursor.Descend(key_ptr);
	PageHandle exp1 = 120;
	EXPECT_EQ(exp1, cursor.current());
}
