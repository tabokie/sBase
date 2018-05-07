#include "./storage/file.h"
#include <iostream>

using namespace sbase;
using std::cout;
using std::endl;

int main(void){

	char* test_data = new char[2000];
	memset(test_data, 'x', 2000);

	FileMeta sequential_file_meta("sequential_file_test_0", 2000); // 8 kb per block
	SequentialFile sequential_file(sequential_file_meta);
	sequential_file.Open();
	cout << sequential_file.Append(0).ToString() << endl;
	cout << sequential_file.Flush(0, test_data);
	sequential_file.Close();

	FileMeta random_file_meta("random_file_test_0", 2000);
	RandomAccessFile random_file(random_file_meta);
	random_file.Open();
	cout << random_file.Append(0, 2000).ToString() << endl;
	cout << random_file.Flush(0, 2000, test_data).ToString() <<endl;
	random_file.Close();

	delete [] test_data;

	return 0;
}