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

#include "./util.h"

// Function prototypes
static void merge_f(data_t* A, int p, int q, int r);
static void copy_f(data_t* source, data_t* dest, int n);
void sort_f(data_t* A, int p, int r);
void sort_f2(data_t* A, int p, int r);

extern void isort(data_t* begin, data_t* end);

static data_t* scratch = NULL;

inline void sort_f(data_t* A, int p, int r) {
  mem_alloc(&scratch, r - p + 2);
  if (scratch == NULL) {
    mem_free(&scratch);
    return;
  }

  sort_f2(A, p, r);

  mem_free(&scratch);
}

// A basic merge sort routine that sorts the subarray A[p..r]
inline void sort_f2(data_t* A, int p, int r) {
  assert(A);
  if (r - p < 32) {
    // base case with up to 32 elements
    // coarsened using an insertion sort
    isort(A + p, A + r);
  } else {
    int q = (p + r) / 2;
    sort_f2(A, p, q);
    sort_f2(A, q + 1, r);
    merge_f(A, p, q, r);
  }
}

// A merge routine. Merges the sub-arrays A [p..q] and A [q + 1..r].
// Uses two arrays 'left' and 'right' in the merge operation.
inline static void merge_f(data_t* A, int p, int q, int r) {
  assert(A);
  assert(p <= q);
  assert((q + 1) <= r);
  int n1 = q - p + 1;
  int n2 = r - q;

  data_t *left = 0, *right = 0;
  left = scratch;
  right = A + q;

  copy_f(&(A[p]), left, n1);
  copy_f(&(A[q + 1]), right, n2);
  left[n1] = UINT_MAX;
  right[n2] = UINT_MAX;

  int i = 0;
  int j = 0;

  for (int k = p; k <= r; k++) {
    if (left[i] <= right[j]) {
      A[k] = left[i];
      i++;
    } else {
      A[k] = right[j];
      j++;
    }
  }
}

inline static void copy_f(data_t* source, data_t* dest, int n) {
  assert(dest);
  assert(source);

  for (int i = 0; i < n; i++) {
    dest[i] = source[i];
  }
}
