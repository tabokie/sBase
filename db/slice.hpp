#ifndef SBASE_STORAGE_SLICE_HPP_
#define SBASE_STORAGE_SLICE_HPP_

/*
// interface:
// slice : support convertion from/to Blob, string, runtime type
// record : reference to field definition, contain value 

*/

#include ".\util\reflection.hpp"
#include ".\util\status.hpp"
#include ".\storage\file_format.hpp" 

#include <iostream>
#include <string>
#include <cstring>
#include <functional>
#include <vector>
#include <cassert>


namespace sbase{

// Your Expectations //
// get schema
// inject char* with schema into record
// compare record
// print out record

typedef Object Slice;
using SliceIterator = std::vector<Object>::iterator;

// Field //
class Schema: public ClassDef{
 private:
 	// effective auxiliary field
 	std::vector<bool> primary; // true for primary
 	std::vector<bool> unique;
 	// std::vector<bool> null; // true for allow null
 	// std::vector<std::string> defaultV; // store as string
 	std::vector<PageHandle> index;
 public:
 	Schema(std::string name):ClassDef(nullptr, name){ };
	template<typename attr_iterator>
	Schema(std::string name, attr_iterator attrBegin, attr_iterator attrEnd, std::vector<bool> prim, std::vector<bool> uniq):
		ClassDef(nullptr,name,attrBegin,attrEnd),primary(prim),unique(uniq),index((attrEnd-attrBegin),0){ }
	template<typename attr_iterator>
	Schema(std::string name, attr_iterator attrBegin, attr_iterator attrEnd, std::vector<bool> prim):
		ClassDef(nullptr,name,attrBegin,attrEnd),primary(prim),unique((attrBegin-attrEnd), true),index((attrEnd-attrBegin),0){ }

	template<typename attr_iterator>
	Schema(std::string name, attr_iterator attrBegin, attr_iterator attrEnd):
		ClassDef(nullptr,name,attrBegin,attrEnd),primary((attrEnd-attrBegin), false),unique((attrEnd-attrBegin), true),index((attrEnd-attrBegin),0){
		primary[0] = true; // first as primary
	} 
	// Initialize //
	Slice* NewSlice(void){
		return new Object(this);
	}
	bool SetIndex(size_t idx, PageHandle hPage){
		if(index.size() == 0){
			index.resize(attributeCount());
		}
		else if(index.size() < attributeCount())return false;
		index[idx] = hPage;
		return true;
	}	
	bool AddField(const Attribute& attr, bool prim, bool uniq){
		AddAttribute(attr);
		primary.push_back(prim);
		unique.push_back(uniq);
		return true;
	}

	// Get Attribute //
	bool GetPrimary(AttributeContainer& ret){
		for(int i = 0; i < effective_attr_.size(); i++){
			if(primary[i]){
				ret.push_back(effective_attr_[i]);
			}
		}
		return true;
	}
	size_t GetKeyIndexWithName(std::string name){
		return GetAttributeIndex(name);
	}
	Attribute GetKeyWithIndex(size_t idx){
		return GetAttribute(idx);
	}
	bool isPrimary(size_t idx){
		return primary[idx];
	}
	bool isUnique(size_t idx){
		return unique[idx];
	}

};

}

#endif // SBASE_STORAGE_SLICE_HPP_