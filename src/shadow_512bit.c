/* MIT License
 *
 * Copyright (c) 2019 Gaëtan Cassiers
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <immintrin.h>


#ifdef BENCH_IACA
#include "iacaMarks.h"
#else
#define IACA_START
#define IACA_END
#endif

#include "primitives.h"

#if SMALL_PERM==1
#error "Only 512 bit permutation is supported for 256 and 512 bit shadow implementations."
#endif // SMALL_PERM==1

typedef uint32_t row_set __attribute__ ((vector_size (16)));
typedef uint32_t drow_set __attribute__ ((vector_size (32)));
typedef uint32_t qrow_set __attribute__ ((vector_size (64)));
typedef union shadow_simd {
    qrow_set v;
    __m512i i;
} shadow_simd;

#define SHADOW_NS 6                   // Number of steps
#define SHADOW_NR 2 * SHADOW_NS       // Number of rounds

#define LS_ROWS 4      // Rows in the LS design
#define LS_ROW_BYTES 4 // number of bytes per row in the LS design
#define MLS_BUNDLES                                                            \
  (SHADOW_NBYTES / (LS_ROWS* LS_ROW_BYTES)) // Bundles in the mLS design


#define SEQ_4_OFF(i,n) i, i+n, i+(2*n), i+(3*n)
static const shadow_simd transpose = { v: { SEQ_4_OFF(0, 4), SEQ_4_OFF(1, 4), SEQ_4_OFF(2, 4), SEQ_4_OFF(3, 4) }};

#define CONST_BUNDLES(x) (x << 0), (x << 1), (x << 2), (x << 3)
#define CONST_STATE(w, x, y, z ) { CONST_BUNDLES(w), CONST_BUNDLES(x), CONST_BUNDLES(y), CONST_BUNDLES(z) }
static const shadow_simd shadow_simd_rc[SHADOW_NR] = {
    { v: CONST_STATE(1, 0, 0, 0) }, // 0
    { v: CONST_STATE(0, 1, 0, 0) }, // 1
    { v: CONST_STATE(0, 0, 1, 0) }, // 2
    { v: CONST_STATE(0, 0, 0, 1) }, // 3
    { v: CONST_STATE(1, 1, 0, 0) }, // 4
    { v: CONST_STATE(0, 1, 1, 0) }, // 5
    { v: CONST_STATE(0, 0, 1, 1) }, // 6
    { v: CONST_STATE(1, 1, 0, 1) }, // 7
    { v: CONST_STATE(1, 0, 1, 0) }, // 8
    { v: CONST_STATE(0, 1, 0, 1) }, // 9
    { v: CONST_STATE(1, 1, 1, 0) }, // 10
    { v: CONST_STATE(0, 1, 1, 1) }  // 11
};

#define ROT32(x, c) (((x) >> (c)) | ((x) << (32 - (c))))
static void lbox_layer_simd(shadow_simd* simd) {
  qrow_set a, b;
  qrow_set x = __builtin_shuffle(simd->v, transpose.v);
  a = x;
  a  = x ^ ROT32(x,12);
  a ^=     ROT32(a, 3);
  a ^=     ROT32(x,17);
  b  = a ^ ROT32(a,31);
  qrow_set exch_pairs = { 1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14 };
  qrow_set y = __builtin_shuffle(b, exch_pairs);
  qrow_set shift = { 26, 25, 26, 25, 26, 25, 26, 25, 26, 25, 26, 25, 26, 25, 26, 25 };
  a ^= ROT32(y, shift);
  a ^= ROT32(b,15);
  simd->v = __builtin_shuffle(a, transpose.v);
}

static const qrow_set sel_0 = { SEQ_4_OFF(0, 1), SEQ_4_OFF(0, 1), SEQ_4_OFF(0, 1), SEQ_4_OFF(0, 1) };
static const qrow_set sel_1 = { SEQ_4_OFF(4, 1), SEQ_4_OFF(4, 1), SEQ_4_OFF(4, 1), SEQ_4_OFF(4, 1) };
static const qrow_set sel_2 = { SEQ_4_OFF(8, 1), SEQ_4_OFF(8, 1), SEQ_4_OFF(8, 1), SEQ_4_OFF(8, 1) };
static const qrow_set sel_3 = { SEQ_4_OFF(12, 1), SEQ_4_OFF(12, 1), SEQ_4_OFF(12, 1), SEQ_4_OFF(12, 1) };
static const qrow_set dispatch0= { SEQ_4_OFF(0, 1), SEQ_4_OFF(16, 1), SEQ_4_OFF(0, 1), SEQ_4_OFF(16, 1) };
static const qrow_set dispatch1= { SEQ_4_OFF(0, 1), SEQ_4_OFF(4, 1), SEQ_4_OFF(16, 1), SEQ_4_OFF(20, 1) };
static const qrow_set sel_4 = { SEQ_4_OFF(0, 1), SEQ_4_OFF(4, 1), SEQ_4_OFF(0, 1), SEQ_4_OFF(4, 1) };
static const qrow_set sel_5 = { SEQ_4_OFF(8, 1), SEQ_4_OFF(12, 1), SEQ_4_OFF(8, 1), SEQ_4_OFF(12, 1) };
static void sbox_layer_simd(shadow_simd* simd) {
    __m128i x0 = _mm512_castsi512_si128(simd->i);
    __m128i x1 = _mm512_extracti32x4_epi32 (simd->i, 1);
    __m256i tmp = _mm512_extracti64x4_epi64(simd->i, 1);
    __m128i x2 = _mm256_castsi256_si128(tmp);
    __m128i x3 = _mm256_extracti128_si256(tmp, 1);
    //__m128i x2 = _mm512_extracti32x4_epi32 (simd->i, 2);
    //__m128i x3 = _mm512_extracti32x4_epi32 (simd->i, 3);
    __m128i y1 = (x0 & x1) ^ x2;
    __m128i y0 = (x0 & x3) ^ x1;
    __m128i y3 = (y1 & x3) ^ x0;
    __m128i y2 = (y0 & y1) ^ x3;
    __m512i res = _mm512_castsi128_si512(y0);
    res = _mm512_inserti32x4(res, y1, 1);
    res = _mm512_inserti32x4(res, y2, 2);
    res = _mm512_inserti32x4(res, y3, 3);
    //__m256i res1 = _mm256_castsi128_si256(y0);
    //res1 = _mm256_inserti32x4(res1, y1, 1);
    //__m256i res2 = _mm256_castsi128_si256(y2);
    //res2 = _mm256_inserti32x4(res2, y3, 1);
    //__m512i res = _mm512_castsi256_si512(res1);
    // res = _mm512_inserti64x4(res, res2, 1);
    simd->i = res;
    /*
    qrow_set x0 = __builtin_shuffle(simd->v, sel_0);
    qrow_set x1 = __builtin_shuffle(simd->v, sel_1);
    qrow_set x2 = __builtin_shuffle(simd->v, sel_2);
    qrow_set x3 = __builtin_shuffle(simd->v, sel_3);
    qrow_set y1 = (x0 & x1) ^ x2;
    qrow_set y0 = (x0 & x3) ^ x1;
    qrow_set y3 = (y1 & x3) ^ x0;
    qrow_set y2 = (y0 & y1) ^ x3;
    qrow_set t0 = __builtin_shuffle(y0, y1, dispatch0);
    qrow_set t1 = __builtin_shuffle(y2, y3, dispatch0);
    //shadow_simd t0 = __builtin_shuffle(x0, x1, dispatch0);
    //shadow_simd t1 = __builtin_shuffle(x2, x3, dispatch0);
    //shadow_simd t0 = __builtin_shuffle(*simd, sel_4);
    //shadow_simd t1 = __builtin_shuffle(*simd, sel_5);
    simd->v = __builtin_shuffle(t0, t1, dispatch1);
    */
}
static void add_rc_simd(shadow_simd* simd, unsigned int round) {
      simd->v ^= shadow_simd_rc[round].v;
}

#define OFFSET_44(w, x, y, z) w, x, y, z, w+4, x+4, y+4, z+4, w+8, x+8, y+8, z+8, w+12, x+12, y+12, z+12
static const qrow_set dbox_shuffle1 = { OFFSET_44(1, 0, 0, 0) };
static const qrow_set dbox_shuffle2 = { OFFSET_44(2, 2, 1, 1) };
static const qrow_set dbox_shuffle3 = { OFFSET_44(3, 3, 3, 2) };
static void dbox_mls_layer_simd(shadow_simd *simd) {
    qrow_set a = __builtin_shuffle(simd->v, dbox_shuffle1);
    qrow_set b = __builtin_shuffle(simd->v, dbox_shuffle2);
    qrow_set c = __builtin_shuffle(simd->v, dbox_shuffle3);
    simd->v = a ^ b ^ c;
}
// Shadow permutation. Updates x (array of SHADOW_NBYTES bytes).
void shadow(shadow_state state) {
    shadow_simd simd;
    memcpy(&simd, state, SHADOW_NBYTES);
    simd.v = __builtin_shuffle(simd.v, transpose.v);
  for (unsigned int s = 0; s < SHADOW_NS; s++) {
      IACA_START
    sbox_layer_simd(&simd);
    lbox_layer_simd(&simd);
    add_rc_simd(&simd, 2*s);
    sbox_layer_simd(&simd);
    dbox_mls_layer_simd(&simd);
    add_rc_simd(&simd, 2*s+1);
  }
      IACA_END
    simd.v = __builtin_shuffle(simd.v, transpose.v);
    memcpy(state, &simd, SHADOW_NBYTES);
}
