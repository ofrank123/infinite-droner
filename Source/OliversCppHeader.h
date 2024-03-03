#pragma once

#include <cassert>
#include <complex>
#include <cstddef>
#include <cstdint>

// Some convienience typedefs
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t  i8;

typedef size_t usize;

typedef float  f32;
typedef double f64;

typedef std::complex<f32> complex32;
typedef std::complex<f64> complex64;

#define global static
#define internal static
#define local_persist static

template <typename T>
struct Slice {
   T *ptr;
   usize len;
   T& operator [](usize idx) {
      return ptr[idx];
   }
   T operator [](usize idx) const {
      return ptr[idx];
   }
};

struct MArena {
   u8 *ptr;
   usize bytesAllocated;
   u8 *head;
};
