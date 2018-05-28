#include "latch.hpp"

#include <thread>
#include <iostream>
#include <windows.h>

using namespace sbase;
using namespace std;

void test_read(Latch* latch){
	latch->ReadLock();
	cout << "read locked" << endl;
	Sleep(1000);
	latch->ReleaseReadLock();
}

void test_weak_write(Latch* latch){
	latch->WeakWriteLock();
	cout << "weak write locked" << endl;
	latch->ReleaseWeakWriteLock();
}

void test_write(Latch* latch){
	latch->WriteLock();
	cout << "strong write locked" << endl;
	latch->ReleaseWriteLock();
}

int main(void){

	Latch latch;

	std::thread thread0(test_read, &latch);
	std::thread thread1(test_weak_write, &latch);
	std::thread thread2(test_write, &latch);

	thread0.join();
	thread1.join();
	thread2.join();
	cout << "all threads join" << endl;

	return 0;
}