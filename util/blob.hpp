#ifndef SBASE_UTIL_BLOB_HPP_
#define SBASE_UTIL_BLOB_HPP_

#include <cstring>

struct Blob{
	size_t len;
	char* data;
	Blob():len(1){
		data = new char[1];
		data[0] = '\0';
	}
	Blob(char* ptr, size_t l):len(l){
		if(len <=0 ){
			data = nullptr;
			len = 0;
		}
		else{
			data = new char[len];
			memcpy(data, ptr, sizeof(char)*len);			
		}
	}
	~Blob(){ delete [] data;}
	Blob(const Blob& rhs):len(rhs.len){
		if(len <= 0){
			data = nullptr;
			len = 0;
		}
		else{
			data = new char[len];
			memcpy(data, rhs.data, sizeof(char)*len);
		}
	}
};

#endif // SBASE_UTIL_BLOB_HPP_