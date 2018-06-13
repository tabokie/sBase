#ifndef SBASE_UTIL_REFLECTION_HPP_
#define SBASE_UTIL_REFLECTION_HPP_

#include <cstring>
#include <vector>
#include <string>
#include <sstream>
#include <initializer_list>

#include ".\util\blob.hpp"

#include <vector>
#include <iterator>
#include <algorithm>
#include <cassert>
#include <iostream>
using std::istream;
using std::ostream;
using std::stringstream;

// NOTICE //
// unsafe constructor using naked char*
// causing explicit difference between char* and std::string
// only use string to construct by literal
// #define Put32Char(str) 			do{if(str){printf("%d: ",str); for(int i = 0; i < 32; i++)printf("%02x ", str[i]);printf("\n");}else{printf("NAN\n");}}while(0)
// #define Put32CharS(str) 			do{if(str){FixChar::debugPtr = str;printf("%d: ",str); for(int i = 0; i < 32; i++)printf("%02x ", str[i]);printf("\n");}else{printf("NAN\n");}}while(0)
// #define FUNC() 							do{std::cout << __func__ << std::endl;if(FixChar::debugPtr)Put32Char(FixChar::debugPtr);}while(0)

// Custom Type and util //
struct FixChar{
	// static char* debugPtr;
 private:
	uint32_t length;
	char* str; // len = length+1
 public:
 	char* pointer(void){return str;}
 	const char* const_pointer(void) const{
 		return str;
 	}
 	size_t get_length(void) const{
 		return length;
 	}
	FixChar(int len = 0):length(len),str(nullptr){
		if(len < 0)length = 0;
		str = new char[length+1]();
		memset(str, 0, length+1);
	}
	FixChar(int len, std::string in):length(len),str(nullptr){
		if(len < 0)length = 0;
		str = new char[length + 1]();
		memset(str, 0, length+1);
		memcpy(str, in.c_str(), ((in.size()<length)?in.size():length) );
	}
	FixChar(const FixChar& rhs):length(rhs.length),str(nullptr){
		str = new char[length + 1]();
		memset(str, 0, length+1);
		memcpy(str, rhs.str, length); // cut down overflow
	}
	~FixChar(){
		// if(str)delete [] str;
	}
	operator std::string()const {
		std::string tmp;
		tmp = str;
		// tmp.copy(str, length, 0);
		// auto ret = std::string(str);
		return tmp;
	}
	friend stringstream& operator>>(stringstream& is, FixChar& rhs){
		memset(rhs.str, 0, rhs.length+1);
		std::string tmp;
		is >> tmp;
		memcpy(rhs.str, tmp.c_str(), ((tmp.size()<rhs.length)?tmp.size():rhs.length));
		// rhs.str[rhs.length] = '\0'; // in case of overflow
		return is;
	}
	friend stringstream& operator<<(stringstream& os, const FixChar& rhs){
		char* tmp = new char[rhs.length+1]();
		memcpy(tmp, rhs.str, rhs.length+1);
		// std::string tmp = rhs.str;
		os << tmp;
		delete [] tmp;
		return os;
	}	
	friend istream& operator>>(istream& is, FixChar& rhs){
		memset(rhs.str, 0, rhs.length+1);
		std::string tmp;
		is >> tmp;
		memcpy(rhs.str, tmp.c_str(), ((tmp.size()<rhs.length)?tmp.size():rhs.length));
		// rhs.str[rhs.length] = '\0'; // in case of overflow
		return is;
	}
	friend ostream& operator<<(ostream& os, const FixChar& rhs){
		char* tmp = new char[rhs.length+1];
		memcpy(tmp, rhs.str, rhs.length+1);
		// std::string tmp = rhs.str;
		os << tmp;
		delete [] tmp;
		return os;
	}
	bool operator<(const FixChar& rhs) const{
		return strcmp(str, rhs.str) < 0;
	}
	bool operator>(const FixChar& rhs) const{
		return strcmp(str, rhs.str) > 0;
	}
	bool operator<=(const FixChar& rhs) const{
		return strcmp(str, rhs.str) <= 0;
	}
	bool operator>=(const FixChar& rhs) const{
		return strcmp(str, rhs.str) >= 0;
	}
	bool operator==(const FixChar& rhs) const{
		return strcmp(str, rhs.str) == 0;
	}
	bool operator==(const std::string rhs) const{
		return strcmp(str, rhs.c_str()) == 0;
	}
	bool operator>(const std::string rhs) const{
		return strcmp(str, rhs.c_str()) > 0;
	}
	bool operator>=(const std::string rhs) const{
		return strcmp(str, rhs.c_str()) >= 0;
	}
	bool operator<(const std::string rhs) const{
		return strcmp(str, rhs.c_str()) < 0;
	}
	bool operator<=(const std::string rhs) const{
		return strcmp(str, rhs.c_str()) <= 0;
	}
	friend bool operator==(const std::string lhs, const FixChar& rhs){
		return rhs == lhs;
	}
	friend bool operator<(const std::string lhs, const FixChar& rhs){
		return rhs < lhs;
	}
	friend bool operator>(const std::string lhs, const FixChar& rhs){
		return rhs > lhs;
	}
	friend bool operator<=(const std::string lhs, const FixChar& rhs){
		return rhs <= lhs;
	}
	friend bool operator>=(const std::string lhs, const FixChar& rhs){
		return rhs >= lhs;
	}
};

/*
struct FixChar{
	uint32_t length;
	mutable char* fixchar; // len = length+1
	FixChar(int len = 0):length(len),fixchar(nullptr){
		std::cout << "create by len " << len << std::endl;
		if(len < 0)length = 0;
		// lazy initialization
		// if(length > 0)fixchar = new char[length];
	}
	FixChar(int len, std::string in):length(len),fixchar(nullptr){
		std::cout << "create by string " << len << " " << in << std::endl;
		if(len < 0)length = 0;
		fixchar = new char[length + 1]();
		memset(fixchar, 0, length+1);
		memcpy(fixchar, in.c_str(), ((in.size()<length)?in.size():length) );
	}
	FixChar(const FixChar& rhs):length(rhs.length),fixchar(nullptr){
		std::cout << "create by reference " << std::string(rhs) << std::endl;
		// std::cout << "Copy fixchar: " << rhs.length << rhs.fixchar << std::endl;
		if(rhs.fixchar){
			std::cout << rhs.fixchar << std::endl;
			fixchar = new char[length + 1]();
			memset(fixchar, 0, length+1);
			memcpy(fixchar, rhs.fixchar, length); // cut down overflow
			std::cout << fixchar << std::endl;
		}
	}
	~FixChar(){
		std::cout << "destroy" << std::endl;
		if(fixchar){
			delete [] fixchar;
			fixchar = nullptr;
		}
	}
	operator std::string()const {
		if(!fixchar)return std::string("NaN");
		return std::string(fixchar);
	}
	friend istream& operator>>(istream& is, FixChar& rhs){
		if(rhs.length <= 0) return is;
		if(!rhs.fixchar)rhs.fixchar = new char[rhs.length + 1]();
		memset(rhs.fixchar, 0, rhs.length+1);
		std::string tmp;
		is >> tmp;
		memcpy(rhs.fixchar, tmp.c_str(), ((tmp.size()<rhs.length)?tmp.size():rhs.length));
		// rhs.fixchar[rhs.length] = '\0'; // in case of overflow
		return is;
	}
	friend ostream& operator<<(ostream& os, const FixChar& rhs){
		if(rhs.length <= 0 || !rhs.fixchar)return os;
		os << rhs.fixchar;
		return os;
	}
	bool operator<(const FixChar& rhs) const{
		if(!fixchar && !rhs.fixchar)return false;
		if(!fixchar)return true;
		if(!rhs.fixchar)return false;
		return strcmp(fixchar, rhs.fixchar) < 0;
	}
	bool operator>(const FixChar& rhs) const{
		if(!fixchar && !rhs.fixchar)return false;
		if(!fixchar)return false;
		if(!rhs.fixchar)return true;
		return strcmp(fixchar, rhs.fixchar) > 0;
	}
	bool operator<=(const FixChar& rhs) const{
		if(!fixchar)return true;
		if(!rhs.fixchar)return false;
		return strcmp(fixchar, rhs.fixchar) <= 0;
	}
	bool operator>=(const FixChar& rhs) const{
		if(!rhs.fixchar)return true;
		if(!fixchar)return false;
		return strcmp(fixchar, rhs.fixchar) >= 0;
	}
	bool operator==(const FixChar& rhs) const{
		if(!fixchar && !rhs.fixchar)return true;
		if(!fixchar || !rhs.fixchar)return false;
		return strcmp(fixchar, rhs.fixchar) == 0;
	}
	bool operator==(const std::string rhs) const{
		if(!fixchar)return rhs.length()==0;
		return strcmp(fixchar, rhs.c_str()) == 0;
	}
	bool operator>(const std::string rhs) const{
		if(!fixchar)return false;
		return strcmp(fixchar, rhs.c_str()) > 0;
	}
	bool operator>=(const std::string rhs) const{
		if(!fixchar)return rhs.length()==0;
		return strcmp(fixchar, rhs.c_str()) >= 0;
	}
	bool operator<(const std::string rhs) const{
		if(!fixchar)return rhs.length()!=0;
		return strcmp(fixchar, rhs.c_str()) < 0;
	}
	bool operator<=(const std::string rhs) const{
		if(!fixchar)return true;
		return strcmp(fixchar, rhs.c_str()) <= 0;
	}
	friend bool operator==(const std::string lhs, const FixChar& rhs){
		return rhs == lhs;
	}
	friend bool operator<(const std::string lhs, const FixChar& rhs){
		return rhs < lhs;
	}
	friend bool operator>(const std::string lhs, const FixChar& rhs){
		return rhs > lhs;
	}
	friend bool operator<=(const std::string lhs, const FixChar& rhs){
		return rhs <= lhs;
	}
	friend bool operator>=(const std::string lhs, const FixChar& rhs){
		return rhs >= lhs;
	}
};
*/
template<typename T>
struct TypeLen{ 
  size_t operator()(const T& key) const{
    return sizeof(key);
  }
};
// special for string
template<> struct TypeLen<FixChar>{
  size_t operator()(const FixChar& fixchar) const{
  	return fixchar.get_length();
  }
};

template <typename T>
struct TypeBlob{
	const char* operator()(const T& key){
		return reinterpret_cast<const char*>(&key);
	}
	char* operator()(T& key){
		return reinterpret_cast<char*>(&key);
	}
};
template<> struct TypeBlob<FixChar>{
	char* operator()(FixChar& key){
		return key.pointer();
	}
	const char* operator()(const FixChar& key){
		return key.const_pointer();
	}
	// char* operator()(FixChar& key){
	// 	// if(!key.fixchar)key.fixchar = new char[key.length+1]();
	// 	return key.str.c_str();
	// }
};


// Type and Value //

class Value;

// holder of real value //
// convertion: 
// string <->	Value (public interface) 		 <-> blob
// 							|
// string <-> RealValue (<->explicit value)<-> blob
// 					 		|
// 					 BaseValue(internal usage)
class BaseValue{
 protected:
 	static std::stringstream ConvertHelper_;
 public:
 	// template constructor
 	virtual ~BaseValue(){  };
 	virtual BaseValue* clone() const = 0;
 	virtual std::string ToString() const = 0;
 	virtual void FromString(const std::string) = 0;
 	virtual void set(BaseValue const*) = 0;
 	// unsafe approach
 	virtual void FromNakedPtr(const char*) = 0;
 	virtual void ToNakedPtr(char*) const = 0;
 	virtual void FromBlob(const Blob) = 0;
 	virtual Blob ToBlob() const = 0;
 	virtual bool operator<(const BaseValue& rhs) const = 0;
 	virtual bool operator>(const BaseValue& rhs) const = 0;
 	virtual bool operator==(const BaseValue& rhs) const = 0;
 	virtual bool operator<=(const BaseValue& rhs) const = 0;
 	virtual bool operator>=(const BaseValue& rhs) const = 0;

 	virtual size_t length(void) = 0;
};


// real value holder

// PlainType pre-requisite
// PlainType(const PlainType&)
// operator<< >>
// operotr <=>
template <typename PlainType>
class RealValue: public BaseValue{
	friend Value;
	PlainType val;
 public: 
 	// explicit define
 	RealValue(PlainType v):val(v){ }
 	RealValue(const RealValue<PlainType>& rhs):val(rhs.val){ }
 	~RealValue() override {  }
 	BaseValue* clone(void) const{
 		return new RealValue<PlainType>(*this);
 	}
 	// string convert
 	std::string ToString(void) const{
 		ConvertHelper_.str("");
 		ConvertHelper_.clear();
 		ConvertHelper_ << val;
 		return ConvertHelper_.str();
 	}
 	void FromString(const std::string str){
 		ConvertHelper_.str("");
 		ConvertHelper_.clear();
 		ConvertHelper_.str(str);
 		ConvertHelper_ >> val;
 		return ;
 	}
 	// blob convert
 	void FromBlob(const Blob bob){
		
 		if(TypeLen<PlainType>{}(val) != bob.len){
 			return ;
 		}
 		memcpy(TypeBlob<PlainType>{}(val), bob.data, TypeLen<PlainType>{}(val) );
 		return ;
 	}
 	Blob ToBlob(void) const{
		
 		return Blob( TypeBlob<PlainType>{}(val),TypeLen<PlainType>{}(val));
 	}
 	// unsafe approach
 	void FromNakedPtr(const char* src){
		
 		memcpy(TypeBlob<PlainType>{}(val), src, TypeLen<PlainType>{}(val) );
 		return ;
 	}
 	void ToNakedPtr(char* des) const{
		
 		memcpy(des, TypeBlob<PlainType>{}(val), TypeLen<PlainType>{}(val) );
 		return ;
 	}
 	// set
 	void set(BaseValue const* v){
		
 		if(v){
 			val = (static_cast<const RealValue<PlainType>*>(v)->val);
 		}
 		else{
 			val = PlainType();
 		}
 		return ;
 	}
 	bool operator<(const BaseValue& rhs) const{
 		return val < (static_cast<const RealValue<PlainType>&>(rhs).val);
 	}
 	bool operator>(const BaseValue& rhs) const{
 		return val > (static_cast<const RealValue<PlainType>&>(rhs).val);
 	}
 	bool operator==(const BaseValue& rhs) const{
 		return val == (static_cast<const RealValue<PlainType>&>(rhs).val);
 	}
 	bool operator>=(const BaseValue& rhs) const{
 		return val >= (static_cast<const RealValue<PlainType>&>(rhs).val);
 	}
 	bool operator<=(const BaseValue& rhs) const{
 		return val <= (static_cast<const RealValue<PlainType>&>(rhs).val);
 	}
 	size_t length(void){return TypeLen<PlainType>{}(val);}

};

enum TypeT{
	tinyintT = 0, 
	intT = 1, 
	uintT = 2,
	bigintT = 3, 
	doubleT = 4, 
	// varchar = 0,
	fixchar8T = 5,
	fixchar16T = 6,
	fixchar32T = 7,
	fixchar64T = 8,
	fixchar128T = 9,
	fixchar256T = 10,
	unknownT = 11
};

class Type{
	friend class Value;
 public:
	explicit Type(TypeT type_id):typeId(type_id){ }
	BaseValue* newValue() const{
		if(typeId < 0 || typeId >= unknownT)return nullptr;
		return prototypes[typeId]->clone();
	}
	TypeT type() const{
		return typeId;
	}
	static size_t getLength(TypeT type_id){
		if(type_id < unknownT)return prototype_length[type_id];
		return 0;
	}
	static BaseValue* InfinityValue(TypeT typeId){
		if(typeId < 0 || typeId >= unknownT)return nullptr;
		return prototypes[typeId]->clone();
	}
 protected: 
	static BaseValue* prototypes[unknownT];
	static size_t prototype_length[unknownT];
	TypeT typeId;
};



// uniform interface
// exist because BaseValue is pure abstract class(no data, no ctor)
// need delegate
class Value{
	BaseValue* val;
	TypeT type_;
 public:
 	static Value InfinityValue(TypeT type){
 		return Value(type, Type::InfinityValue(type));
 	}	
	// Value(BaseValue const& v):val(v.clone()){ }
	Value(const Value& rhs):val(rhs.val ? rhs.val->clone() : nullptr),type_(rhs.type_){ }
	explicit Value(TypeT typeId, BaseValue* pv):val(pv),type_(typeId){ }
	~Value(){if(val)delete val;}
	Value& operator=(const Value& rhs){
		if(val){
			if(rhs.val){
				val->set(rhs.val);
			}
			else{
				delete val;
				val = nullptr;
			}
		}
		else{
			val = (rhs.val ? rhs.val->clone() : nullptr);
		}
		return *this;
	}
	template <typename PlainType>
	PlainType get()const{
		if(val){
			RealValue<PlainType> const & converted = dynamic_cast<RealValue<PlainType>const&>(*val);
			return converted.val;
		}
		return PlainType();
	}
	// inherit interface
	Value(TypeT typeId = unknownT):val(nullptr),type_(typeId){
		
		if(typeId < unknownT){
			val = Type::prototypes[type_]->clone();
		}
	}
	Value(TypeT typeId, const std::string str):val(nullptr),type_(typeId){
		
		if(typeId < unknownT){
			val = Type::prototypes[typeId]->clone();
			val->FromString(str);
		}
	}	
	// unsafe interface
	Value(TypeT typeId, const char* ptr):val(nullptr),type_(typeId){
		
		if(typeId < unknownT){
			val = Type::prototypes[typeId]->clone();
			val->FromNakedPtr(ptr);
		}
	}	
	Value(TypeT typeId, const Blob bob):val(nullptr),type_(typeId){
		
		if(typeId < unknownT){
			val = Type::prototypes[typeId]->clone();
			val->FromBlob(bob);
		}
	}
	void Read(const std::string str){
		
		if(!val)return ;
		val->FromString(str);
		return ;
	}
	void Read(const char* ptr){ // unsafe
		
		if(!val)return;
		val->FromNakedPtr(ptr);
		return ;
	}
	void Read(const Blob bob){
		
		if(!val)return;
		val->FromBlob(bob);
		return;
	}
	void Write(char* ptr) const{
		
		if(val)val->ToNakedPtr(ptr);
		return ;
	}
	operator std::string()const{
		
		if(!val)return std::string("NaN");
		return val->ToString();
		// if(!val)return std::string("");
		// return val->ToString();
	}
	operator Blob()const{
		
		if(!val)return Blob();
		return val->ToBlob();
	}
	bool operator<(const Value& rhs) const{
		// always push null to last
		if(!rhs.val)return false;
		else if(!rhs.val)return true;
		return (*val) < (*(rhs.val));
	}
	bool operator>(const Value& rhs) const{
		// always push null to last
		if(!val)return false;
		else if(!rhs.val)return true;
		return (*val) > (*(rhs.val));
	}
	bool operator==(const Value& rhs) const{
		// always push null to last
		if(!val && !rhs.val)return true;
		else if(!val || !rhs.val)return false;
		return (*val) == (*(rhs.val));
	}
	bool operator<=(const Value& rhs) const{
		// always push null to last
		if(!val)return true;
		else if(!rhs.val)return false;
		return (*val) <= (*(rhs.val));
	}
	bool operator>=(const Value& rhs) const{
		// always push null to last
		if(!rhs.val)return true;
		else if(!val)return false;
		return (*val) >= (*(rhs.val));
	}
	size_t length(void) const{
		if(!val)return 0;
		return val->length();
	}
	TypeT type(void) const{
		return type_;
	}
};

// Class and Object //
class Attribute{
	std::string attr_name_;
	TypeT type_;
 public:
 	Attribute(const std::string& name, TypeT typeId):
 	attr_name_(name),
 	type_(typeId){ }
 	Attribute():attr_name_("null"),type_(unknownT){ }
 	~Attribute(){ }
 	const std::string& name() const{
 		return attr_name_;
 	}
 	TypeT type(void) const{
 		return type_;
 	}
 	bool operator==(const Attribute& rhs) const{
 		return attr_name_ == rhs.attr_name_ && type_ == rhs.type_;
 	}
 	bool operator==(const std::string& name) const{
 		return attr_name_ == name;
 	}
};

// forward declaration
class Object;

class ClassDef{
 protected:
	using AttributeContainer = std::vector<Attribute>;
	using AttributeIterator = std::vector<Attribute>::const_iterator;
	// attributes
	// std::vector<Attribute> attr_;
	std::vector<Attribute> effective_attr_; // include full attr
	// base inheritence
	ClassDef const* base_;
	// flag
	std::string name_;
	mutable bool definition_fix_;
 public:
 	template<typename attr_iterator>
	ClassDef(ClassDef const* base, std::string name, attr_iterator attrBegin, attr_iterator attrEnd):		
		base_(base),
		name_(name),
		definition_fix_(false){ 
		// std::copy(attrBegin, attrEnd, std::back_inserter<AttributeContainer>(attr_));
		BaseInit();
		// effective_attr_.insert(effective_attr_.end(), attr_.begin(), attr_.end());
		effective_attr_.insert(effective_attr_.end(), attrBegin, attrEnd);
	}
	ClassDef(ClassDef const* base, std::string name):
		base_(base),
		name_(name),
		definition_fix_(false){ 
		BaseInit();
	}
	Attribute operator[](size_t idx) const{
		return GetAttribute(idx);
	}
	bool AddAttribute(Attribute newAttr){
		if(!definition_fix_){
			effective_attr_.push_back(newAttr);
			return true;
		}
		return false;
	}
	Attribute GetAttribute(size_t idx)const{
		assert(idx < effective_attr_.size());
		return effective_attr_[idx];
	}
	int GetAttributeIndex(std::string name)const{
		for(int idx = 0; idx < effective_attr_.size(); idx++){
			auto attr = effective_attr_[idx];
			if(attr.name() == name){
				return idx;
			}
		}
		return -1;
	}
	size_t length(void) const{ // byte
		size_t ret = 0;
		for(auto& attr : effective_attr_){
			ret += Type::getLength(attr.type());
		}
		return ret;
	}
	size_t attributeCount(void)const{return effective_attr_.size();}
	AttributeIterator attributeBegin(void)const{return effective_attr_.begin();}
	AttributeIterator attributeEnd(void)const{return effective_attr_.end();}
	std::string name(void)const{return name_;}
	void setUnfix(void) const{definition_fix_ = true;}
	Object* NewObject(void)const;

 protected:
 	void BaseInit(void){
 		if(base_){
 			base_->setUnfix();
 			std::copy(base_->attributeBegin(), 
 				base_->attributeEnd(), 
 				std::back_inserter<AttributeContainer>(effective_attr_));
 		}
 	}
};

class Object{
	ClassDef const* const class_;
	std::vector<Value> values_;
 public:
 	Object(ClassDef const* cls):class_(cls){
		
 		InitAttributeValue();
 	}
 	Object(ClassDef const* cls, const std::initializer_list<Value>& pack):class_(cls){
		
 		assert(pack.size() <= class_->attributeCount());
 		auto attr = class_->attributeBegin();
 		for(auto& val : pack){
 			values_.push_back(val);
 			attr++;
 		}
 		while(attr < class_->attributeEnd()){
 			values_.push_back(Value( attr->type() ) );
 			attr++;
 		}
 	}
 	template <class ...Args>
 	Object(ClassDef const* cls, Args... args):class_(cls){
		
 		InitAttributeValue(args...);
 	} 	
 	Object(const Object& rhs):class_(rhs.class_),values_(rhs.values_){	}
 	~Object(){ }
 	Object* clone(void) const{return new Object(class_);};
 	ClassDef const* instanceOf(void) const{return class_;}
 	Value operator[](size_t idx) const{
		
 		return GetValue(idx);
 	}
 	Value GetValue(size_t idx) const{
		
 		if(idx >= values_.size())return Value();
 		return values_[idx];
 	}
 	Value GetValue(const std::string& name) const{
		
 		size_t index = class_->GetAttributeIndex(name);
 		if(index >= 0)return values_[index];
 		else return Value();
 	}
 	bool SetValue(size_t idx, const Value& val){
		
 		if(idx >= values_.size())return false;
 		values_[idx] = val;
 		return true;
 	}
 	bool SetValue(std::string name, const Value& val){
		
 		size_t index = class_->GetAttributeIndex(name);
 		if(index >= 0){
 			values_[index] = val;
 			return true;
 		}
 		return false;
 	}
 	// unsafe
 	void Read(const char* ptr){
		
 		for(auto& val: values_){
 			val.Read(ptr);
 			ptr += val.length();
 		}
 		return ;
 	}
 	void Write(char* ptr) const{
		
 		for(auto& val: values_){
 			val.Write(ptr);
 			ptr += val.length();
 		}
 		return ; 		
 	}
 	size_t length(void) const{if(!class_)return 0; return class_->length();}
 private:
 	void InitAttributeValue(void){
		
 		if(class_)
 		for(auto attr = class_->attributeBegin(); attr < class_->attributeEnd(); attr++){
 			values_.push_back(Value((*attr).type()));
 		}
 	}
 	template <class ...Args>
 	void InitAttributeValue(Args... args){
		
 		assert(sizeof...(args) <= class_->attributeCount());
 		auto attr = class_->attributeBegin();
 		int arr[] = { ( values_.push_back(Value((*attr).type(), args)),attr++,0)... };
 		while(attr < class_->attributeEnd()){
 			values_.push_back(Value((*attr).type()));
 			attr++;
 		}
 	}

};



#endif // SBASE_UTIL_REFLECTION_HPP_