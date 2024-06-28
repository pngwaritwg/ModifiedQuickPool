#pragma once

#include <emp-tool/emp-tool.h>
#include <NTL/ZZ_p.h>
#include <vector>

using namespace NTL;

namespace quickpool {

template <class R>
class AddShare {
  // value_ will be additive share of my_id.
  R value_;
  
 public:
  AddShare() = default;
  AddShare(R value): value_{value} {}

  void randomize(emp::PRG& prg) {
    randomizeZZp(prg, value_.data(), sizeof(R));
  }  

  R& valueAt() { return value_; }

  void pushValue(R val) { value_ = val; }
  
  R valueAt() const { return value_; }

  // Arithmetic operators.
  AddShare<R>& operator+=(const AddShare<R>& rhs) {
    value_ += rhs.value_;
    return *this;
  }

  template <class T>
  friend AddShare<T> operator+(AddShare<T> lhs, const AddShare<T>& rhs) {
    lhs += rhs;
    return lhs;
  }

  AddShare<R>& operator-=(const AddShare<R>& rhs) {
    (*this) += (rhs * R(-1));
    return *this;
  }

  template <class T>
  friend AddShare<T> operator-(AddShare<T> lhs, const AddShare<T>& rhs) {
    lhs -= rhs;
    return lhs;
  }

  AddShare<R>& operator*=(const R& rhs) {
    value_ *= rhs;
    return *this;
  }

  template <class T>
  friend AddShare<T> operator*(AddShare<T> lhs, const T& rhs) {
    lhs *= rhs;
    return lhs;
  }

  AddShare<R>& operator<<=(const int& rhs) {
    uint64_t value = conv<uint64_t>(value_);
    value <<= rhs;
    value_ = value;
    return *this;
  }

  template <class T>
  friend AddShare<T> operator<<(AddShare<T> lhs, const int& rhs) {
    lhs <<= rhs;
    return lhs;
  }

  AddShare<R>& operator>>=(const int& rhs) {
    uint64_t value = conv<uint64_t>(value_);
    value >>= rhs;
    value_ = value;
    return *this;
  }

  template <class T>
  friend AddShare<T> operator>>(AddShare<T> lhs, const int& rhs) {
    lhs >>= rhs;
    return lhs;
  }

	//the part written below requires consideration
  AddShare<R>& add(R val, int pid) {
    if (pid == 1) {
      value_ += val;
    }    
    return *this;
  }

  AddShare<R>& addWithAdder(R val, int pid, int adder) {
    if (pid == adder) {
      value_ += val;
    }
    return *this;
  }

};

template <class R>
class TPShare {
  std::vector<R> values_;

  public:
  TPShare() = default;
  TPShare(std::vector<R> value) : values_{std::move(value)} {}

  // Access share elements.
  // idx = i retreives value common with party having i.
  R& operator[](size_t idx) { return values_.at(idx); }
  
  R operator[](size_t idx) const { return values_.at(idx); }

  R& commonValueWithParty(int pid) {
        return values_.at(pid);
    }

  [[nodiscard]] R commonValueWithParty(int pid) const {
    return values_.at(pid);
  }

  void pushValues(R val) { values_.push_back(val); }

  [[nodiscard]] R secret() const { 
    R res=values_[0];
    for (int i = 1; i < values_.size(); i++)
      res+=values_[i];
    return res;
  }

  // Arithmetic operators.
  TPShare<R>& operator+=(const TPShare<R>& rhs) {
    for (size_t i = 1; i < values_.size(); i++) {
      values_[i] += rhs.values_[i];
    }
    return *this;
  }

  template <class T>
  friend TPShare<T> operator+(TPShare<T> lhs, const TPShare<T>& rhs) {
    lhs += rhs;
    return lhs;
  }

  TPShare<R>& operator-=(const TPShare<R>& rhs) {
    (*this) += (rhs * R(-1));
    return *this;
  }

  template <class T>
  friend TPShare<T> operator-(TPShare<T> lhs, const TPShare<T>& rhs) {
    lhs -= rhs;
    return lhs;
  }

  TPShare<R>& operator*=(const R& rhs) {
    for(size_t i = 1; i < values_.size(); i++) {
      values_[i] *= rhs;
    }
    return *this;
  }

  template <class T>
  friend TPShare<T> operator*(TPShare<T> lhs, const T& rhs) {
    lhs *= rhs;
    return lhs;
  }

  TPShare<R>& operator<<=(const int& rhs) {
    for(size_t i = 1; i < values_.size(); i++) {
      uint64_t value = conv<uint64_t>(values_[i]);
      value <<= rhs;
      values_[i] = value;
    }
    return *this;
  }

  template <class T>
  friend TPShare<T> operator<<(TPShare<T> lhs, const int& rhs) {
    lhs <<= rhs;
    return lhs;
  }

  TPShare<R>& operator>>=(const int& rhs) {
    for(size_t i = 1; i < values_.size(); i++) {
      uint64_t value = conv<uint64_t>(values_[i]);
      value >>= rhs;
      values_[i] = value;
    }
    return *this;
  }

  template <class T>
  friend TPShare<T> operator>>(TPShare<T> lhs, const int& rhs) {
    lhs >>= rhs;
    return lhs;
  }

  AddShare<R> getAAS(size_t pid) {
    return AddShare<R>({values_.at(pid)});
  }
  
};

};  // namespace quickpool

