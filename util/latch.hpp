#ifndef SBASE_STORAGE_LATCH_HPP_
#define SBASE_STORAGE_LATCH_HPP_

#include "util/utility.hpp"
#include "util/error.hpp"

#include <thread>
#include <mutex>
#include <atomic> // try use no lock method
#include <condition_variable>
#include <iostream>

namespace sbase{

class Latch: public NoCopy{
	std::mutex mutex_;
	// reader
	std::atomic<size_t> readers_;
	std::condition_variable cond_r_;
	// writer
	std::atomic<size_t> strong_write_appliers_; // queue // atomic
	std::atomic<size_t> weak_write_appliers_;
	bool swriter_;
	std::condition_variable cond_sw_;
	bool writer_; // writer = strong + weak
	std::condition_variable cond_ww_;
public:
	Latch(): readers_(0),
		weak_write_appliers_(0),strong_write_appliers_(0),
		writer_(false),swriter_(false){ }
	~Latch(){ }
	void ReadLock(void){ 
		// LOG_FUNC();
		// cannot acquire when strong writer is applying
		// strong writer is banned
		std::unique_lock<std::mutex> local(mutex_);
		// wait means unlock and wait
		cond_r_.wait( local, [=]()->bool {return !swriter_;} );
		readers_ ++;
	}
	void WeakWriteLock(void){ // writer is banned
		// LOG_FUNC();
		std::unique_lock<std::mutex> local(mutex_);
		weak_write_appliers_ ++;
		cond_ww_.wait( local, [=]()->bool{ return !writer_; } );
		writer_ = true;
		weak_write_appliers_ --;
	}
	void WriteLock(void){ // writer and reader are banned
		// LOG_FUNC();
		std::unique_lock<std::mutex> local(mutex_);
		strong_write_appliers_ ++;
		cond_sw_.wait(local, [=]()->bool{return readers_ == 0 && !writer_;});
		swriter_ = true;
		writer_ = true;
		strong_write_appliers_ --;
	}
	void ReleaseReadLock(void){ // notify strong writer
		// LOG_FUNC();
		std::unique_lock<std::mutex> local(mutex_);
		if(--readers_ >= 1){
			// assert no writers
			if(strong_write_appliers_ > 0){
				cond_sw_.notify_one();
			}
		}
	}
	void ReleaseWeakWriteLock(void){ // notify weak writer
		// LOG_FUNC();
		std::unique_lock<std::mutex> local(mutex_);
		writer_ = false;
		// assert no strong writer
		if(weak_write_appliers_ > 0){
			cond_ww_.notify_one();
		}
		else if(readers_ >= 1 && strong_write_appliers_ > 0){
			cond_sw_.notify_one();
		}
	}
	void ReleaseWriteLock(void){ // notify weak writer -> writer -> all reader
		// LOG_FUNC();
		std::unique_lock<std::mutex> local(mutex_);
		writer_ = false;
		swriter_ = false;
		if(weak_write_appliers_ > 0){
			cond_ww_.notify_one();
			cond_r_.notify_all();
		}
		else if(strong_write_appliers_ > 0){
			cond_sw_.notify_one();
		}
		else{
			cond_r_.notify_all();
		}
	}
	void ReadLockLiftToWriteLock(void){
		// assert having acquire a read lock
		std::unique_lock<std::mutex> local(mutex_);
		strong_write_appliers_++;
		cond_sw_.wait(local, [=]()->bool{return readers_ == 1;});
		readers_--; // now release read lock
		swriter_ = true;
		writer_ = true;
		strong_write_appliers_ --;		
	}
	void WeakWriteLockLiftToWriteLock(void){
		std::unique_lock<std::mutex> local(mutex_);
		strong_write_appliers_ ++;
		cond_sw_.wait(local, [=]()->bool{return readers_ == 0;}); // already hold weak write lock
		swriter_ = true;
		strong_write_appliers_ --;
	}
	bool occupied(void){
		return readers_ > 0 || writer_;
	}
}; // class Latch

} // namespace sbase

#endif // SBASE_STORAGE_LATCH_HPP_