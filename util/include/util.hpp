#pragma once

#include <algorithm>
#include <cstdint>

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
  option(T t) : valid(true), t(t) {}
  T* operator->() { return &t; }
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

extern const uint64_t base[64];
#if defined(__CUDACC__)
extern __constant__ uint64_t base_dev[64];
#endif

inline uint64_t get_base(size_t i) {
#if defined(__CUDA_ARCH__)
  return base_dev[i];
#else
  return base[i];
#endif
}

struct fnv64 {
private:
  const uint64_t prime = 0x00000100000001B3;
  uint64_t hash_;
public:
  inline fnv64(uint64_t seed) : hash_(seed) {}

  inline fnv64& hash(uint8_t byte) {
    hash_ ^= byte;
    hash_ *= prime;
    return *this;
  }

  inline fnv64& hash(uint16_t hword) {
    return hash((uint8_t)(hword)).hash((uint8_t)(hword>>8));
  }

  inline fnv64& hash(uint32_t word) {
    return hash((uint8_t)(word))
          .hash((uint8_t)(word>>8))
          .hash((uint8_t)(word>>16))
          .hash((uint8_t)(word>>24));
  }

  inline fnv64& hash(uint64_t dword) {
    return hash((uint8_t)(dword))
          .hash((uint8_t)(dword>>8))
          .hash((uint8_t)(dword>>16))
          .hash((uint8_t)(dword>>24))
          .hash((uint8_t)(dword>>32))
          .hash((uint8_t)(dword>>40))
          .hash((uint8_t)(dword>>48))
          .hash((uint8_t)(dword>>56));
  }

  inline uint64_t get() { return hash_; }
};

} // namespace util
