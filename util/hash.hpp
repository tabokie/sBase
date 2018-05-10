#ifndef SBASE_UTIL_HASH_HPP_
#define SBASE_UTIL_HASH_HPP_

#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <functional>

using std::shared_ptr;
using std::make_shared;
using std::string;

using std::cout;
using std::endl;


template<typename T>
struct _hash{ 
  size_t operator()(const T& key) const{
    return std::hash<T>{}(key);
  }
};
// Hash function for string
template<> struct _hash<std::string>{
  typedef size_t result_type;
  typedef std::string argument_type;
  size_t operator()(const std::string& str) const{
    const char* ptr = (str.c_str());
    unsigned int seed = 131;
    unsigned int hash = 0;
    size_t cur = 0;
    while(ptr[cur]){
      hash = hash * seed + (ptr[cur++]);
    }
    return hash;  
  }
};


template <typename _KT, typename _ET>
class HashMap{
 private:
  using ElementType = _ET;
  using KeyType = _KT;
  struct Wrapper_{
    KeyType key;
    ElementType element;
    bool deleted;
    Wrapper_(KeyType k, ElementType e):key(k),element(e),deleted(false){ }
    Wrapper_(Wrapper_& that):key(that.key),element(that.element),deleted(false){ }
    ~Wrapper_(){ } 
  };
  using PairPtr = shared_ptr<Wrapper_>;
  using PairType = Wrapper_;
  size_t size_;
  size_t occupied_;
  PairPtr* table_;
  
 public:  
  HashMap(size_t size = 50)
    :size_(size),occupied_(0){ 
    table_ = new PairPtr[size_];
    for(int i = 0; i < size_; i++)table_[i] = nullptr;
  }
  ~HashMap(){ } // no delete ptr_object

  // insert element
  bool Insert(KeyType key, ElementType element){
    PairPtr p = make_shared<PairType>(key, element);
    return Insert(key, p);
  }
  bool Get(KeyType key, ElementType& element) const{
    unsigned int hash_val  = _hash<KeyType>{}(key) % size_;
    unsigned int cur = hash_val;
    int offset = 1;
    // quadratic proding
    do{
      if(!table_[cur])break;
      if( table_[cur]->key == key)break;
      cur = (cur+ 2*offset - 1) % size_;
      offset++;
    }while(cur != hash_val);
    if(table_[cur] && table_[cur]->key == key && !table_[cur]->deleted){
      element = table_[cur]->element;
      return true;
    }
    return false;
  }

  bool Delete(KeyType key){
    unsigned int hash_val  = _hash<KeyType>{}(key) % size_;
    unsigned int cur = hash_val;
    int offset = 1;
    // quadratic proding
    do{
      if(!table_[cur] || table_[cur]->deleted)break;
      if( table_[cur]->key == key)break;
      cur = (cur+ 2*offset - 1) % size_;
      offset++;
    }while(cur != hash_val);
    if(table_[cur] && !table_[cur]->deleted && table_[cur]->key==key){
      table_[cur]->deleted = true;
      occupied_ --;
      return true;
    }
    return false;
  }

  bool DeleteOnGet(KeyType key, ElementType& element){
    unsigned int hash_val  = _hash<KeyType>{}(key) % size_;
    unsigned int cur = hash_val;
    int offset = 1;
    // quadratic proding
    do{
      if(!table_[cur] || table_[cur]->deleted)break;
      if( table_[cur]->key == key)break;
      cur = (cur+ 2*offset - 1) % size_;
      offset++;
    }while(cur != hash_val);
    if(table_[cur] && !table_[cur]->deleted && table_[cur]->key==key){
      table_[cur]->deleted = true;
      element = table_[cur]->element;
      occupied_ --;
      return true;
    }
    return false;
  }

  bool Clear(void){
    for(int i = 0; i < size_; i++)table_[i] = nullptr;
    occupied_ = 0;
  }

  size_t occupied(void){return occupied_;}
  size_t size(void){return size_;}

 private:
  bool Insert(KeyType key, PairPtr p){
    unsigned int hash_val  = _hash<KeyType>{}(key) % size_;
    unsigned int cur = hash_val;
    int offset = 1;
    // quadratic proding
    do{
      if(!table_[cur] || table_[cur]->deleted)break;
      if( table_[cur]->key == key)break;
      cur = (cur+ 2*offset - 1) % size_;
      offset++;
    }while(cur != hash_val);
    // no vacant
    if( offset > 2 && cur == hash_val ){
      Rehash_();
      return Insert(key, p);
    }
    // vacant
    if( !table_[cur] || table_[cur]->deleted ){
      table_[cur] = p;
      occupied_ ++;
      return true;
    }
    // duplicate
    return false;
  }
  // Rehash routine
  bool Rehash_(void){
    int old_size = size_;
    size_ *= 2;
    PairPtr* new_table = new PairPtr[size_];
    for(int i =0; i< size_; i++)new_table[i] = nullptr;
    PairPtr* old_table = table_;
    table_ = new_table;
    occupied_ = 0;
    for(int i = 0; i < old_size; i++){
      if(old_table[i] && !old_table[i]->deleted)
        Insert(old_table[i]->key, old_table[i]);
    }
    delete [] old_table;
    return true;
  }

}; // class Hash

#endif // SBASE_UTIL_HASH_HPP_