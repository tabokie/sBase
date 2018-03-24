#ifndef DISC_MEMMAP_HPP_
#define DISC_MEMMAP_HPP_

#include <string>
#include <iostream>

// Memory Map OS API //
// for windows
#if defined(__WIN64) || defined(__WIN32)
#include <windows.h>
#endif // __WINXX
// for linux
#ifdef __linux
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif // __linux

class MemMap{
 public:
	Memmep();
	~Memmep();
	bool Open(const char*);
	bool Map(const char*);
	bool Share(const char*);
	bool Close(const char*);
	char* get_ptr();
 private:
 	void Unmap();
 	void Unfile();
 	void Error();
}


#endif /* DISC_MEMMAP_HPP_ */

