// Copyright (c) 2024 Oliver Frank
// Licensed under the GNU Public License (https://www.gnu.org/licenses/)

#pragma once

#include <cassert>
#include <complex>
#include <cstddef>
#include <cstdint>

//------------------------------
//~ ojf: this file includes a bunch of simple typedefs
// and utility structs that i like to use when writing
// c++

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;

typedef size_t usize;

typedef float f32;
typedef double f64;

typedef std::complex<f32> c32;
typedef std::complex<f64> c64;

#define global static
#define internal static
#define local_persist static

constexpr f64 PI = 3.1415926535897932384626433832795028841971693993751;
constexpr f64 TWO_PI = PI * 2;

/**
 * "fat pointer" audio buffer, to store a length alongside
 * the underlying data
 */
struct Buffer
{
    f32* ptr; // pointer to data
    usize len; // length of data
    f32& operator[] (usize idx)
    {
        assert (idx < len);
        return ptr[idx];
    }
    f32 operator[] (usize idx) const
    {
        assert (idx < len);
        return ptr[idx];
    }
};

/**
 * write zeros to a buffer
 * @param slice to clear
 */
void clearSlice (Buffer slice);

/**
 * allocate a new zeroed buffer
 * @param length of buffer
 */
Buffer createSlice (usize len);

/**
 * stereo buffer
 */
struct StereoBuffer
{
    Buffer leftBuffer;
    Buffer rightBuffer;
};

/**
 * write zeros to a stereo buffer
 * @param buffer to clear
 */
void clearStereoBuffer (StereoBuffer buffer);

/**
 * allocate a new zeroed stereo buffer
 */
StereoBuffer createStereoBuffer (usize len);
