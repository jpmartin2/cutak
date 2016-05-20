#pragma once

#include <algorithm>

#if defined(__CUDACC__)
#define CUDA_CALLABLE __host__ __device__
#else
#define CUDA_CALLABLE
#endif

namespace util {
#if defined(__CUDACC__)
template<typename T>
CUDA_CALLABLE inline T min(T a, T b) { return ::min(a,b); }
template<typename T>
CUDA_CALLABLE inline T max(T a, T b) { return ::max(a,b); }
#else
template<typename T>
inline T min(T a, T b) { return std::min(a,b); }
template<typename T>
inline T max(T a, T b) { return std::max(a,b); }
#endif

template<typename T>
class option {
public:
  static const option None;
  explicit operator bool() const { return valid; }
  T& operator*() { return t; }
  option& operator=(T val) { t = val; return *this; }
  option() : valid(false) {}
  option(const option& other) : valid(other.valid) { if(other) t = other.t; }
private:
  bool valid;
  T t;
};

template<typename T>
const option<T> option<T>::None = option();

/*
// Optimization for references.
// Stores the reference as a pointer and
// uses a null pointer to represent None
template<typename T>
class option<T&> {
public:
  static const option None = option();
  operator bool() const { return t != nullptr; }
  T& operator*() { return *t; }
  operator=(T& val) { t = &val; }
  option() : valid(false) {}
private:
  T* t;
};
*/

} // namespace util
