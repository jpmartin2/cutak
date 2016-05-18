#pragma once

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

} // namespace util
