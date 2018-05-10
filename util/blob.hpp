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
	Blob(const char* ptr, size_t l):len(l){
		if(len <=0 ){
			data = nullptr;
			len = 0;
		}
		else{
			data = new char[len];
			memcpy(data, ptr, sizeof(char)*len);			
		}
	}
	~Blob(){ if(data)delete [] data;}
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
	bool operator==(const Blob& rhs) const{
		if(len != rhs.len) return false;
		for(int i=  0; i< len; i++){
			char a = data[i];
			char b = rhs.data[i];
			if(a!=b)return false;
		}
		return true;
	}
};

#endif // SBASE_UTIL_BLOB_HPP_