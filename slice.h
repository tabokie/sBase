#include <iostream>
#include <string>
#include <functional>
#include <typeinfo>
using namespace std;

struct Type{
	const type_info* meta;
	size_t size;

	function<void(ostream&, void*)> output;
	function<void(void*, void*)> equal;
	function<std::string(void*)> toString;
};

// var_text, fix_text
// float
// int
// bool
// null
// blob ? -> 
const struct FLOAT;
const struct BOOL;
// manifest: {name, type, (length)}
// interface in table
// init a manifest
// new a slice from input (read) 
	// to banary directly
// serialize
	// binary write
// deserialize
	// use manifest and type function
// get i-th item
class TypeConverter{
 public:

	void* p;
	// function<void(ostream&, void*)> f;
	Type t;

	template <typename T>
	void Add(T& a){
		cout << sizeof(T);
		t.meta = &typeid(a);
		p = reinterpret_cast<void*>(&a);
		t.output = [](ostream& os, void* p){
			os << *(static_cast<T*>(p));
		};
		t.equal = [](void* tis, void* tat){
			return *(static_cast<T*>(tis)) == *(static_cast<T*>(tat));
		};
		t.size = sizeof(a);
		return ;
	}
	void Put(ostream& os){
		t.output(os, p);
		return ;
	}

};

class SliceMeta: private NonCopy{
	vector<TypeMeta> type;
	vector<size_t> offset;
	template <typename T>
	void Add(size_t s = -1){
		size_t offset = vector[offset.size()-1];
		if(s > 0)offset += s;
		else offset += sizeof(T);
		TypeMeta t;
		t.output = [](ostream& os, void* p){
			os << *(static_cast<T*>(p));
		};
		t.equal = [](void* tis, void* tat){
			return *(static_cast<T*>(tis)) == *(static_cast<T*>(tat));
		};
		t.toString = [](void* p){
			std::string ret;
			std::stringstream stream;
			stream << *(static_cast<T*>(p));
			stream >> ret;
			return ret;
		};
		t.size = sizeof(a);
		type.push_back(t);
	}
}


class Slice{
 private:
 	SliceMeta* meta_;
 	char* data_;
 public:
	Slice(SliceMeta& meta):meta_(&meta){ }
	~Slice(){ }
	Status Update(size_t no, char* element){
		size_t offset = meta_->get_offset(no);
		size_t size = meta_->get_size(no);
		memcpy(data_+offset, element, size);
		return Status::OK();
	}
}
