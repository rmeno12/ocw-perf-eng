// Copyright (c) 2015 MIT License by 6.172 Staff

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#define SIZE (1L << 16)

void test(uint8_t* restrict a, uint8_t* restrict b) {
  uint64_t i;

  a = __builtin_assume_aligned(a, 32);
  b = __builtin_assume_aligned(b, 32);

  for (i = 0; i < SIZE; i++) {
    a[i] += b[i];
  }
}
