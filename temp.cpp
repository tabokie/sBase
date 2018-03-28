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
 public:
	Slice(SliceMeta& meta):meta_(&meta){ }
	~Slice(){ }
}

int main(void){

	double a = 1.8;
	// char* conv = static_cast<char*>(&a);
	// string 
	TypeConverter conv;
	conv.Add<double>(a);
	conv.Put(cout);

	return 0;
}