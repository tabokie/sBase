#ifndef UTIL_BUZY_VEC_HPP_
#define UTIL_BUZY_VEC_HPP_

#include "util\timer.hpp"
#include "util\status.hpp"

#include <algorithm>
#include <iterator>
#include <vector>
#include <cassert>

// in range of LOCAL_TIME_T,
// count must be allowed to decrement to 1

using LocalTimeType = uint8_t;
using CountType = uint8_t;
const CountType kCountMax = 255;
const CountType kCountInit = 1;
const LocalTimeType kCountDecrPerUnit = 1; // range = 0..255, max decr = 255

const size_t kDefaultBound = 50;
const size_t kDefaultPickUpSize = 5;
const double kBuzyVecLogFactor = 2.0; // ??

template <class T>
class BuzyVector{
	using DataType = T;
	using DataContainer = std::vector<T>;
	using DataIterator = DataContainer::const_iterator;
	// using TimeContainer = std::vector<LocalTimeType>;
	// using TimeIterator = TimeContainer::const_iterator;
	// using CountContainer = std::vector<CountType>;
	// using CountIterator = CountContainer::const_iterator;
	struct AdditionField{
		LocalTimeType last_decr_time;
		CountType count;
		AdditionField():last_decr_time(0),count(kCountMax){ }
		AdditionField(LocalTimeType t, CountType c):last_decr_time(t),count(c){ }
		AdditionField(const AdditionField& that):last_decr_time(that.last_decr_time),count(that.count){ }
		~AdditionField(){ }
		// competence sort
		// first one is one with least count
		bool operator<(const AdditionField& rhs){
			return count < rhs.count;
		}
	};

	using AdditionContainer = std::vector<AdditionField> v;
	using AdditionIterator = AdditionContainer::const_iterator;

	DataContainer data_;
	AdditionContainer addition_;
	size_t size_;
	size_t bound_;
 public:
	BuzyVector(void):size_(0),bound_(kDefaultBound){ }
	// size in consistence with data
	// BuzyVector(size_t size):std::vector<T>(size),last_decr_time_(size),count_(size),size_(size){ }
	BuzyVector(const BuzyVector& that):size_(that.size_),bound_(that.bound_){
		std::copy(that.DataBegin(), that.DataEnd(), std::back_inserter<DataContainer>(data_));
		std::copy(that.AdditionBegin(), that.AdditionEnd(), std::back_inserter<AdditionContainer>(data_));
	}
	BuzyVector(size_t bound):bound_(bound),size_(0){ }
	~BuzyVector(){ }

	size_t size(void){return size_;}
	size_t bound(void){return bound_;}
	void SetBound(size_t bd){
		if(bd < size_){
			int i = size_ - bd;
			while(i--)Retire();
		}
		bound_ = bd;
		return ;
	}

	size_t push_back(const DataType& val){
		if(size_ >= bound_){
			auto ret = Retire();
			assert(ret.ok());
		}
		data_.push_back(val);
		last_decr_time_.push_back(Time::Now());
		count_.push_back(1);
		return data_.size()-1;
	}
	void Reference(DataType val)
	const DataType& operator[](size_t idx){
		Increment(idx);
		return data_[idx]; // leave exception for vector 
	}
	DataIterator DataBegin(void){
		return data_.begin();
	}
	DataIterator DataEnd(void){
		return data_.end();
	}
 private:
	AdditionIterator AdditionBegin(void){
		return last_decr_time_.begin();
	}
	AdditionIterator AdditionEnd(void){
		return last_decr_time_.end();
	}
	void Check(size_t index){
		auto& field = addition_[index];
		int add_time = static_cast<int>(static_cast<LocalTimeType>(Time::Now()) - field.last_decr_time) * kCountDecrPerUnit;
		while(add_time -- ){
			Increment(index);
		}
		return ;
	}
	void Decrement(size_t index){
		auto& field = addition_[index];
		if(field.count > 0){
			field.count = static_cast<double>(field.count) / 2.0;
		}
		return ;
	}
	void Increment(size_t index){
		auto& field = addition_[index];
		if(field.count == kCountMax)return ;
		double random_checker = Random::getDoubleRand();
		double base = field.count - kCountInit;
		if(base < 0)base = 0;
		double p = 1.0 / (base * kBuzyVecLogFactor + 1);
		if(r < p) field.count ++;
		return ;
	}
	Status Retire(void){
		assert(size_ > 0);
		AdditionContainer v;
		size_t best_pickup_index = 0;
		AdditionField best_pickup;
		size_t index;
		for(int i = 0; i < kDefaultPickUpSize; i++){
			index = Random::getIntRand(size_);
			Check(index);
			field = addition_[index];
			if(field < best_pickup){
				best_pickup = field;
				best_pickup_index = index;
			}
		}
		size_ --;
		return Status::OK();
	}
};


#endif // UTIL_BUZY_VEC_HPP_