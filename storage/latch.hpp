#ifndef SBASE_STORAGE_LATCH_HPP_
#define SBASE_STORAGE_LATCH_HPP_

#include <thread>
#include <mutex>
#include <atomic> // try use no lock method
#include <condition_variable>

namespace sbase{

class Latch{
	// reader
	std::atomic<size_t> readers_;
	std::mutex r_mutex_;
	std::condition_variable cond_r_;
	// strong writer
	std::atomic<size_t> strong_writers_; // queue // atomic
	bool strong_writer_;
	std::mutex w_mutex_;
	std::condition_variable cond_w_;
	// weak writer
	std::atomic<size_t> weak_writers_;
	bool weak_writer_; // writer = strong + weak
	std::mutex ww_mutex_;
	std::condition_variable cond_ww_;
public:
	Latch(): readers_(0),
		weak_writers_(0),strong_writers_(0),
		strong_writer_(false),weak_writer_(false){ }
	~Latch(){ }
	void ReadLock(void){ // strong writer
		std::unique_lock<std::mutex> local(r_mutex_);
		// wait means unlock and wait
		cond_r_.wait( local, [=]()->bool {return !strong_writer_;} );
		readers_ ++;
	}
	void WeakWriteLock(void){ // any writer
		weak_writers_ ++;
		std::unique_lock<std::mutex> local(ww_mutex_);
		cond_ww_.wait( local, [=]()->bool{ return !strong_writer_ && !weak_writer_; } );
		weak_writer_ = true;
	}
	void WriteLock(void){ // strong writer and reader
		strong_writers_ ++;
		std::unique_lock<std::mutex> local(w_mutex_);
		cond_w_.wait(local, [=]()->bool{return !strong_writer_ && readers_ == 0;});
		strong_writer_ = true;
	}
	void ReleaseReadLock(void){ // notify strong writer
		std::unique_lock<std::mutex> local(r_mutex_);
		if(--readers_ == 0){
			// assert no writers
			if(strong_writers_ > 0){
				cond_w_.notify_one();
			}
		}
	}
	void ReleaseWeakWriteLock(void){ // notify weak writer
		std::unique_lock<std::mutex> local(ww_mutex_);
		weak_writer_ = false;
		weak_writers_ --;
		// assert no strong writer
		if(weak_writers_ > 0){
			cond_ww_.notify_one();
		}
		else if(readers_ == 0 && strong_writers_ > 0){
			cond_w_.notify_one();
		}
	}
	void ReleaseWriteLock(void){ // notify weak writer -> writer -> all reader
		std::unique_lock<std::mutex> local(w_mutex_);
		strong_writer_ = false;
		strong_writers_ --;
		if(weak_writers_ > 0){
			cond_ww_.notify_one();
		}
		else if(strong_writers_ > 0){
			cond_w_.notify_one();
		}
		else{
			cond_r_.notify_all();
		}
	}
};

}

#endif // SBASE_STORAGE_LATCH_HPP_