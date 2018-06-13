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
#include <deque>

namespace sbase{

// Your Expectations //
// get schema
// inject char* with schema into record
// compare record
// print out record

typedef Object Slice;
using SliceContainer = std::deque<Object>;

// Field //
class Schema: public ClassDef{
 private:
 	// effective auxiliary field
 	std::vector<uint8_t> input_index;
 	// std::vector<bool> primary; // true for primary
 	std::vector<bool> unique;
 	// std::vector<bool> null; // true for allow null
 	// std::vector<std::string> defaultV; // store as string
 	std::vector<PageHandle> index;
 public:
 	// for all input, first is primary
 	// for vector input, assume that input_index == real_index
 	Schema(std::string name):ClassDef(nullptr, name){ };
	template<typename attr_iterator>
	Schema(std::string name, attr_iterator attrBegin, attr_iterator attrEnd):
		ClassDef(nullptr,name,attrBegin,attrEnd),input_index((attrEnd-attrBegin), 0),unique((attrEnd-attrBegin), false),index((attrEnd-attrBegin),0){
		for(int i = 0; i < attributeCount(); i++)input_index[i] = i;
		unique[0] = true; // primary
	} 
	void AppendField(Attribute attr, int idx, bool uniq = false){ // this order corresponding to real layout
		AddAttribute(attr);
		input_index.push_back(idx);
		unique.push_back(uniq);
		index.push_back(0);
		return ;
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
	// Get Attribute //
	PageHandle GetIndexHandle(size_t idx){
		if(index.size() == 0)return 0;
		return index[idx];
	}
	const Attribute& GetPrimaryAttr(void){
		return effective_attr_[0];
	}
	size_t GetKeyIndexWithName(std::string name){
		return GetAttributeIndex(name);
	}
	Attribute GetKeyWithIndex(size_t idx){
		return GetAttribute(idx);
	}
	bool isPrimary(size_t idx){
		return idx == 0;
	}
	bool isUnique(size_t idx){
		return unique[idx];
	}

};

}

#endif // SBASE_STORAGE_SLICE_HPP_