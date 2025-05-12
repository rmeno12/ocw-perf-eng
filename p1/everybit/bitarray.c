/**
 * Copyright (c) 2012 MIT License by 6.172 Staff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 **/

// Implements the ADT specified in bitarray.h as a packed array of bits; a bit
// array containing bit_sz bits will consume roughly bit_sz/8 bytes of
// memory.

#include "./bitarray.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

// ********************************* Types **********************************

// Concrete data type representing an array of bits.
struct bitarray {
  // The number of bits represented by this bit array.
  // Need not be divisible by 8.
  size_t bit_sz;

  // The underlying memory buffer that stores the bits in
  // packed form (8 per byte).
  char* restrict buf;
};

// ******************** Prototypes for static functions *********************

// Rotates a subarray left by an arbitrary number of bits.
//
// bit_offset is the index of the start of the subarray
// bit_length is the length of the subarray, in bits
// bit_left_amount is the number of places to rotate the
//                    subarray left
//
// The subarray spans the half-open interval
// [bit_offset, bit_offset + bit_length)
// That is, the start is inclusive, but the end is exclusive.
static void bitarray_rotate_left(bitarray_t* const bitarray,
                                 const size_t bit_offset,
                                 const size_t bit_length,
                                 const size_t bit_left_amount);

// Rotates a subarray left by one bit.
//
// bit_offset is the index of the start of the subarray
// bit_length is the length of the subarray, in bits
//
// The subarray spans the half-open interval
// [bit_offset, bit_offset + bit_length)
// That is, the start is inclusive, but the end is exclusive.
static void bitarray_rotate_left_one(bitarray_t* const bitarray,
                                     const size_t bit_offset,
                                     const size_t bit_length);

// Portable modulo operation that supports negative dividends.
//
// Many programming languages define modulo in a manner incompatible with its
// widely-accepted mathematical definition.
// http://stackoverflow.com/questions/1907565/c-python-different-behaviour-of-the-modulo-operation
// provides details; in particular, C's modulo
// operator (which the standard calls a "remainder" operator) yields a result
// signed identically to the dividend e.g., -1 % 10 yields -1.
// This is obviously unacceptable for a function which returns size_t, so we
// define our own.
//
// n is the dividend and m is the divisor
//
// Returns a positive integer r = n (mod m), in the range
// 0 <= r < m.
static size_t modulo(const ssize_t n, const size_t m);

// Produces a mask which, when ANDed with a byte, retains only the
// bit_index th byte.
//
// Example: bitmask(5) produces the byte 0b00100000.
//
// (Note that here the index is counted from right
// to left, which is different from how we represent bitarrays in the
// tests.  This function is only used by bitarray_get and bitarray_set,
// however, so as long as you always use bitarray_get and bitarray_set
// to access bits in your bitarray, this reverse representation should
// not matter.
static char bitmask(const size_t bit_index);

static uint64_t reverse64(uint64_t x);
static uint64_t load64(const char* const restrict buf, const size_t bit_offset);
static void store64(char* const restrict buf, const size_t bit_offset,
                    const uint64_t val);
static void bitarray_reverse(bitarray_t* const bitarray,
                             const size_t bit_offset, const size_t bit_length);

// ******************************* Functions ********************************

bitarray_t* bitarray_new(const size_t bit_sz) {
  // Allocate an underlying buffer of ceil(bit_sz/8) bytes.
  char* const buf = calloc(1, (bit_sz + 7) / 8);
  if (buf == NULL) {
    return NULL;
  }

  // Allocate space for the struct.
  bitarray_t* const bitarray = malloc(sizeof(struct bitarray));
  if (bitarray == NULL) {
    free(buf);
    return NULL;
  }

  bitarray->buf = buf;
  bitarray->bit_sz = bit_sz;
  return bitarray;
}

void bitarray_free(bitarray_t* const bitarray) {
  if (bitarray == NULL) {
    return;
  }
  free(bitarray->buf);
  bitarray->buf = NULL;
  free(bitarray);
}

size_t bitarray_get_bit_sz(const bitarray_t* const bitarray) {
  return bitarray->bit_sz;
}

bool bitarray_get(const bitarray_t* const bitarray, const size_t bit_index) {
  assert(bit_index < bitarray->bit_sz);

  // We're storing bits in packed form, 8 per byte.  So to get the nth
  // bit, we want to look at the (n mod 8)th bit of the (floor(n/8)th)
  // byte.
  //
  // In C, integer division is floored explicitly, so we can just do it to
  // get the byte; we then bitwise-and the byte with an appropriate mask
  // to produce either a zero byte (if the bit was 0) or a nonzero byte
  // (if it wasn't).  Finally, we convert that to a boolean.
  return (bitarray->buf[bit_index / 8] & bitmask(bit_index)) ? true : false;
}

void bitarray_set(bitarray_t* const bitarray, const size_t bit_index,
                  const bool value) {
  assert(bit_index < bitarray->bit_sz);

  // We're storing bits in packed form, 8 per byte.  So to set the nth
  // bit, we want to set the (n mod 8)th bit of the (floor(n/8)th) byte.
  //
  // In C, integer division is floored explicitly, so we can just do it to
  // get the byte; we then bitwise-and the byte with an appropriate mask
  // to clear out the bit we're about to set.  We bitwise-or the result
  // with a byte that has either a 1 or a 0 in the correct place.
  bitarray->buf[bit_index / 8] =
      (bitarray->buf[bit_index / 8] & ~bitmask(bit_index)) |
      (value ? bitmask(bit_index) : 0);
}

void bitarray_randfill(bitarray_t* const bitarray) {
  int32_t* ptr = (int32_t*)bitarray->buf;
  for (int64_t i = 0; i < bitarray->bit_sz / 32 + 1; i++) {
    ptr[i] = rand();
  }
}

void bitarray_rotate(bitarray_t* const bitarray, const size_t bit_offset,
                     const size_t bit_length, const ssize_t bit_right_amount) {
  assert(bit_offset + bit_length <= bitarray->bit_sz);

  if (bit_length == 0) {
    return;
  }

  if (bit_right_amount == 0) {
    return;
  }

  // Convert a rotate left or right to a left rotate only, and eliminate
  // multiple full rotations.
  bitarray_rotate_left(bitarray, bit_offset, bit_length,
                       modulo(-bit_right_amount, bit_length));
}

static void bitarray_rotate_left(bitarray_t* const bitarray,
                                 const size_t bit_offset,
                                 const size_t bit_length,
                                 const size_t bit_left_amount) {
  bitarray_reverse(bitarray, bit_offset, bit_left_amount);
  bitarray_reverse(bitarray, bit_offset + bit_left_amount,
                   bit_length - bit_left_amount);
  bitarray_reverse(bitarray, bit_offset, bit_length);
}

static void bitarray_rotate_left_one(bitarray_t* const bitarray,
                                     const size_t bit_offset,
                                     const size_t bit_length) {
  // Grab the first bit in the range, shift everything left by one, and
  // then stick the first bit at the end.
  const bool first_bit = bitarray_get(bitarray, bit_offset);
  size_t i;
  for (i = bit_offset; i + 1 < bit_offset + bit_length; i++) {
    bitarray_set(bitarray, i, bitarray_get(bitarray, i + 1));
  }
  bitarray_set(bitarray, i, first_bit);
}

static size_t modulo(const ssize_t n, const size_t m) {
  const ssize_t signed_m = (ssize_t)m;
  assert(signed_m > 0);
  const ssize_t result = ((n % signed_m) + signed_m) % signed_m;
  assert(result >= 0);
  return (size_t)result;
}

static char bitmask(const size_t bit_index) { return 1 << (bit_index % 8); }

static inline uint64_t reverse64(uint64_t x) {
  x = ((x >> 1) & 0x5555555555555555ULL) | (x & 0x5555555555555555ULL) << 1;
  x = ((x >> 2) & 0x3333333333333333ULL) | (x & 0x3333333333333333ULL) << 2;
  x = ((x >> 4) & 0x0F0F0F0F0F0F0F0FULL) | (x & 0x0F0F0F0F0F0F0F0FULL) << 4;
  x = ((x >> 8) & 0x00FF00FF00FF00FFULL) | (x & 0x00FF00FF00FF00FFULL) << 8;
  x = ((x >> 16) & 0x0000FFFF0000FFFFULL) | (x & 0x0000FFFF0000FFFFULL) << 16;
  return (x >> 32) | (x << 32);
}

static inline __attribute__((always_inline)) uint64_t
load64(const char* const restrict buf, const size_t bit_offset) {
  size_t byte_offset = bit_offset >> 3;
  size_t subbyte_offset = bit_offset & 7;
  const uint64_t* const restrict buf64 = (uint64_t*)(buf + byte_offset);
  __builtin_prefetch(buf64);
  uint64_t w0 = buf64[0];
  uint64_t w1 = buf64[1];
  return (w0 >> subbyte_offset) | (w1 << (64 - subbyte_offset));
}

static inline __attribute__((always_inline)) void store64(
    char* restrict const buf, const size_t bit_offset, const uint64_t val) {
  size_t byte_offset = bit_offset >> 3;
  size_t subbyte_offset = bit_offset & 7;

  uint64_t* const restrict buf64 = (uint64_t*)(buf + byte_offset);
  __builtin_prefetch(buf64);
  uint64_t w0 = buf64[0];
  uint64_t w1 = buf64[1];

  uint64_t m0 = (~0ULL) << subbyte_offset;
  uint64_t m1 = (~0ULL) >> (64 - subbyte_offset);
  w0 = (w0 & ~m0) | ((val << subbyte_offset) & m0);
  w1 = (w1 & ~m1) | ((val >> (64 - subbyte_offset)) & m1);

  buf64[0] = w0;
  buf64[1] = w1;
}

static inline void bitarray_reverse(bitarray_t* const restrict bitarray,
                                    const size_t bit_offset,
                                    const size_t bit_length) {
  char* const restrict buf = bitarray->buf;
  size_t max_k = bit_length / 128;

  // first do as many full word copies as possible
  for (size_t k = 0; k < max_k; k++) {
    size_t i = bit_offset + k * 64;
    size_t j = bit_offset + bit_length - k * 64 - 64;
    uint64_t vi = load64(buf, i);
    uint64_t vj = load64(buf, j);
    store64(buf, i, reverse64(vj));
    store64(buf, j, reverse64(vi));
  }

  // then do single bit swaps to fill in the rest
  bool bit_i;
  bool bit_j;
  for (size_t k = 0; k < bit_length / 2 - max_k * 64; k++) {
    size_t i = bit_offset + max_k * 64 + k;
    size_t j = bit_offset + bit_length - max_k * 64 - 1 - k;
    bit_i = bitarray_get(bitarray, i);
    bit_j = bitarray_get(bitarray, j);
    bitarray_set(bitarray, i, bit_j);
    bitarray_set(bitarray, j, bit_i);
  }
}
