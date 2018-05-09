#include <vector>
#include <string>
#include <sstream>

// #include "util\blob.hpp"
#include "blob.hpp"

#include <vector>
#include <iterator>
#include <algorithm>
#include <cassert>

// Type and Value //

class Value;

// holder of real value //
// convertion: 
// string <->	Value (public interface) 		 <-> blob
// 							|
// string <-> RealValue (<-explicit value) <-> blob
// 					 		|
// 					 BaseValue(internal usage)
class BaseValue{
 protected:
 	static std::stringstream ConvertHelper_;
 public:
 	// template constructor
 	virtual ~BaseValue(){ }
 	virtual BaseValue* clone() const = 0;
 	virtual std::string ToString() const = 0;
 	virtual void FromString(std::string) = 0;
 	virtual void set(BaseValue const*) = 0;
 	virtual void FromBlob(Blob) = 0;
 	virtual Blob ToBlob() const = 0;
 	virtual bool operator<(const BaseValue& rhs) const = 0;
 	virtual bool operator>(const BaseValue& rhs) const = 0;
 	virtual bool operator==(const BaseValue& rhs) const = 0;
};

std::stringstream BaseValue::ConvertHelper_("");



template<typename T>
struct TypeLen{ 
  size_t operator()(const T& key) const{
    return sizeof(key);
  }
};
// special for string
template<> struct TypeLen<std::string>{
  size_t operator()(const std::string& str) const{
    return str.size();
  }
};

// real value holder
template <typename PlainType>
class RealValue: public BaseValue{
	PlainType val;
 public: 
 	// explicit define
 	RealValue(PlainType v):val(v){ }
 	RealValue(const RealValue<PlainType>& rhs):val(rhs.val){ }
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
 	void FromString(std::string str){
 		ConvertHelper_.str("");
 		ConvertHelper_.clear();
 		ConvertHelper_.str(str);
 		ConvertHelper_ >> val;
 		return ;
 	}
 	// blob convert
 	void FromBlob(Blob bob){
 		if(TypeLen<PlainType>(val) != bob.len){
 			return ;
 		}
 		val = *(reinterpret_cast<PlainType*>(bob.data));
 		return ;
 	}
 	Blob ToBlob(void) const{
 		return Blob((static_cast<char*>(&val)),TypeLen<PlainType>(val));
 	}
 	// set
 	void set(BaseValue const* v){
 		if(v){
 			val = (static_cast<RealValue<PlainType>*const>(v).val);
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

};

enum TypeT{
	stringT = 0, 
	tinyintT = 1, 
	intT = 2, 
	bigintT = 3, 
	doubleT = 4, 
	unknownT = 5
};

class Type{
	friend class Value;
 public:
	explicit Type(TypeT type_id):typeId(type_id){ }
	BaseValue* newValue() const{
		if(typeId < 0 || typeId >= unknownT)return nullptr;
		return prototypes[typeId]->clone();
	}
	TypeT getType() const{
		return typeId;
	}
 protected: 
	static BaseValue* prototypes[unknownT];
	TypeT typeId;
};

BaseValue* Type::prototypes[unknownT] = {
	new RealValue<std::string>(""),
	new RealValue<int8_t>(0),
	new RealValue<int32_t>(0),
	new RealValue<int64_t>(0),
	new RealValue<double>(0) };

// uniform interface
// exist because BaseValue is pure abstract class(no data, no ctor)
// need delegate
class Value{
	BaseValue* val;
 public:
	Value(BaseValue const& v):val(v.clone()){ }
	Value(Value const& rhs):val(rhs.val ? rhs.val->clone() : nullptr){ }
	explicit Value(BaseValue* pv = nullptr):val(pv){ }
	~Value(){delete val;}
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
	Value(TypeT type):val(nullptr){
		if(type < unknownT){
			val = Type::prototypes[type]->clone();
		}
	}
	operator double()const{
		return 0;
		// if(!val)return std::string("");
		// return val->ToString();
	}
	Value(TypeT type, std::string str):val(nullptr){
		if(type < unknownT){
			val = Type::prototypes[type]->clone();
			val->FromString(str);
		}
	}
	// operator class Blob()const{
	// 	if(!val)return Blob();
	// 	return val->ToBlob();
	// }
	Value(TypeT type, Blob bob):val(nullptr){
		if(type < unknownT){
			val = Type::prototypes[type]->clone();
			val->FromBlob(bob);
		}
	}
	bool operator<(const Value& rhs) const{
		// always push null to last
		if(!val && !rhs.val)return false;
		else if(!val)return false;
		else if(!rhs.val)return true;
		return (*val) < (*(rhs.val));
	}
	bool operator>(const Value& rhs) const{
		// always push null to last
		if(!val && !rhs.val)return false;
		else if(!val)return false;
		else if(!rhs.val)return true;
		return (*val) > (*(rhs.val));
	}
	bool operator==(const Value& rhs) const{
		// always push null to last
		if(!val && !rhs.val)return true;
		else if(!val)return false;
		else if(!rhs.val)return false;
		return (*val) == (*(rhs.val));
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
 	const std::string& getName() const{
 		return attr_name_;
 	}
 	TypeT getType(void) const{
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
	using AttributeContainer = std::vector<Attribute>;
	using AttributeIterator = std::vector<Attribute>::const_iterator;
	// attributes
	std::vector<Attribute> attr_;
	std::vector<Attribute> effective_attr_;
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
		std::copy(attrBegin, attrEnd, std::back_inserter<AttributeContainer>(attr_));
		BaseInit();
		effective_attr_.insert(effective_attr_.end(), attr_.begin(), attr_.end());
	}
	ClassDef(ClassDef const* base, std::string name):
		base_(base),
		name_(name),
		definition_fix_(false){ 
		BaseInit();
		effective_attr_.insert(effective_attr_.end(), attr_.begin(), attr_.end());
	}
	bool AddAttribute(const Attribute& newAttr){
		if(!definition_fix_){
			attr_.push_back(newAttr);
			return true;
		}
		return false;
	}
	bool SubAttribute(size_t idx){
		if(!definition_fix_ && idx < attr_.size()){
			attr_.erase(attr_.begin()+idx);
			return true;
		}
		return false;
	}
	bool SubAttribute(std::string name){
		if(definition_fix_)return false;
		auto found = std::find(attr_.begin(), attr_.end(), name);
		if(found != attr_.end()){
			attr_.erase(found);
			return true;
		}
		return false;
	}
	Attribute GetAttribute(size_t idx)const{
		assert(idx < effective_attr_.size());
		return effective_attr_[idx];
	}
	Attribute GetAttribute(std::string name)const{
		auto found = std::find(effective_attr_.begin(), effective_attr_.end(), name);
		if(found != effective_attr_.end()){
			return *found;
		}
		return Attribute();
	}
	size_t attributeCount(void)const{return effective_attr_.size();}
	AttributeIterator attributeBegin(void)const{return effective_attr_.begin();}
	AttributeIterator attributeEnd(void)const{return effective_attr_.end();}
	std::string name(void)const{return name_;}
	void setUnfix(void) const{definition_fix_ = true;}
	Object* NewObject(void)const;

 private:
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
 	template <class ...Args>
 	Object(ClassDef const* cls, Args... args):class_(cls){
 		InitAttributeValue(args...);
 	}
 	~Object(){ }
 	Object* clone(void){return nullptr;};
 	ClassDef const* instanceOf(void) const{return class_;}
 	Value GetValue(size_t idx);
 	Value GetValue(const std::string& name);
 	void SetValue(size_t idx, const Value& val);
 	void SetValue(std::string name, const Value& val);
 private:
 	void InitAttributeValue(void){
 		for(auto attr = class_->attributeBegin(); attr < class_->attributeEnd(); attr++){
 			values_.push_back(Value((*attr).getType()));
 		}
 	}
 	template <class ...Args>
 	void InitAttributeValue(Args... args){
 		assert(sizeof...(args) <= class_->attributeCount());
 		auto attr = class_->attributeBegin();
 		int arr[] = { ( values_.push_back(Value((*attr).getType(), args)),attr++,0)... };
 		while(attr < class_->attributeEnd()){
 			values_.push_back(Value((*attr).getType()));
 			attr++;
 		}
 	}

};


// Afterward definition for ClassDef
Object* ClassDef::NewObject(void)const{
	return new Object(this);
}

