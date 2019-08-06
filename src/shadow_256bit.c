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

typedef union __attribute__((aligned(64))) shadow_simd {
    drow_set rows[2];
    row_set mixed_rows[4];
    __m256i rowsi[2];
} shadow_simd;

// drow_set[0] = { B0R0, B1R0, B2R0, B3R0, B0R2, B1R2, B2R2, B3R2 }
// drow_set[1] = { B0R1, B1R1, B2R1, B3R1, B0R3, B1R3, B2R3, B3R3 }

#define SHADOW_NS 6                   // Number of steps
#define SHADOW_NR 2 * SHADOW_NS       // Number of rounds

#define LS_ROWS 4      // Rows in the LS design
#define LS_ROW_BYTES 4 // number of bytes per row in the LS design
#define MLS_BUNDLES                                                            \
  (SHADOW_NBYTES / (LS_ROWS* LS_ROW_BYTES)) // Bundles in the mLS design

#define RS0 0, 0, 0, 0
#define RS1 1, 2, 4, 8
#define RS00 {RS0, RS0}
#define RS01 {RS0, RS1}
#define RS10 {RS1, RS0}
#define RS11 {RS1, RS1}
static const shadow_simd shadow_simd_rc[SHADOW_NR] = {
    {.rows =  { RS10, RS00 }}, // 0
    {.rows =  { RS00, RS10 }}, // 1
    {.rows =  { RS01, RS00 }}, // 2
    {.rows =  { RS00, RS01 }}, // 3
    {.rows =  { RS10, RS10 }}, // 4
    {.rows =  { RS01, RS10 }}, // 5
    {.rows =  { RS01, RS01 }}, // 6
    {.rows =  { RS10, RS11 }}, // 7
    {.rows =  { RS11, RS00 }}, // 8
    {.rows =  { RS00, RS11 }}, // 9
    {.rows =  { RS11, RS10 }}, // 10
    {.rows =  { RS01, RS11 }}, // 11
};

#define DROTR_SIMD(x, c) (((x) >> (c)) | ((x) << (32 - (c))))
static void dlbox_simd(drow_set* x, drow_set* y) {
  drow_set a, b, c, d;
  a = *x ^ DROTR_SIMD(*x, 12);
  b = *y ^ DROTR_SIMD(*y, 12);
  a = a ^ DROTR_SIMD(a, 3);
  b = b ^ DROTR_SIMD(b, 3);
  a = a ^ DROTR_SIMD(*x, 17);
  b = b ^ DROTR_SIMD(*y, 17);
  c = a ^ DROTR_SIMD(a, 31);
  d = b ^ DROTR_SIMD(b, 31);
  a = a ^ DROTR_SIMD(d, 26);
  b = b ^ DROTR_SIMD(c, 25);
  a = a ^ DROTR_SIMD(c, 15);
  b = b ^ DROTR_SIMD(d, 15);
  *x = a;
  *y = b;
}
static void lbox_layer_simd(shadow_simd* simd) {
    dlbox_simd(&simd->rows[0], &simd->rows[1]);
}

static const drow_set sel_low = { 0, 1, 2, 3, 0, 1, 2, 3 };
static const drow_set sel_high = { 4, 5, 6, 7, 4, 5, 6, 7 };
static const drow_set dispatch = { 0, 1, 2, 3, 12, 13, 14, 15 };
static void sbox_layer_simd(shadow_simd* simd) {
    __m128i x0 = _mm256_castsi256_si128(simd->rowsi[0]);
    __m128i x2 = _mm256_extracti128_si256(simd->rowsi[0], 1);
    __m128i x1 = _mm256_castsi256_si128(simd->rowsi[1]);
    __m128i x3 = _mm256_extracti128_si256(simd->rowsi[1], 1);
    __m128i y1 = (x0 & x1) ^ x2;
    __m128i y0 = (x0 & x3) ^ x1;
    __m128i y3 = (y1 & x3) ^ x0;
    __m128i y2 = (y0 & y1) ^ x3;
    /*
    __m256i res0 = _mm256_castsi128_si256(y0);
    res0 = _mm256_inserti32x4(res0, y2, 1);
    simd->rowsi[0] = res0;
    __m256i res1 = _mm256_castsi128_si256(y1);
    res1 = _mm256_inserti32x4(res1, y3, 1);
    simd->rowsi[1] = res1;
    */
    simd->rowsi[0] = _mm256_castsi128_si256(y0);
    //simd->rowsi[0] = _mm256_inserti32x4(simd->rowsi[0], y2, 1);
    simd->rowsi[0] = _mm256_inserti128_si256(simd->rowsi[0], y2, 1);
    simd->rowsi[1] = _mm256_castsi128_si256(y1);
    //simd->rowsi[1] = _mm256_inserti32x4(simd->rowsi[1], y3, 1);
    simd->rowsi[1] = _mm256_inserti128_si256(simd->rowsi[1], y3, 1);
}
static void add_rc_simd(shadow_simd* simd, unsigned int round) {
  for (unsigned int i = 0; i < 2; i++) {
      simd->rows[i] ^= shadow_simd_rc[round].rows[i];
  }
}
static const drow_set dbox_shuffle1 = { 1, 0, 0, 0, 5, 4, 4, 4 };
static const drow_set dbox_shuffle2 = { 2, 2, 1, 1, 6, 6, 5, 5 };
static const drow_set dbox_shuffle3 = { 3, 3, 3, 2, 7, 7, 7, 6 };
static void dbox_mls_layer_simd(shadow_simd *simd) {
  for (unsigned int row = 0; row < 2; row++) {
      drow_set a = __builtin_shuffle(simd->rows[row], dbox_shuffle1);
      drow_set b = __builtin_shuffle(simd->rows[row], dbox_shuffle2);
      drow_set c = __builtin_shuffle(simd->rows[row], dbox_shuffle3);
      simd->rows[row] = a ^ b ^ c;
  }
}
// drow_set[0] = { B0R0, B1R0, B2R0, B3R0, B0R2, B1R2, B2R2, B3R2 }
// drow_set[1] = { B0R1, B1R1, B2R1, B3R1, B0R3, B1R3, B2R3, B3R3 }
// Shadow permutation. Updates x (array of SHADOW_NBYTES bytes).
void shadow(shadow_state state) {
    shadow_simd simd = { .rows =
        {
            {
                state[0][0], state[1][0], state[2][0], state[3][0],
                state[0][2], state[1][2], state[2][2], state[3][2]
            },
            {
                state[0][1], state[1][1], state[2][1], state[3][1],
                state[0][3], state[1][3], state[2][3], state[3][3]
            },
        }
    };
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
    shadow_state state_end = {
        { simd.rows[0][0], simd.rows[1][0], simd.rows[0][4], simd.rows[1][4] },
        { simd.rows[0][1], simd.rows[1][1], simd.rows[0][5], simd.rows[1][5] },
        { simd.rows[0][2], simd.rows[1][2], simd.rows[0][6], simd.rows[1][6] },
        { simd.rows[0][3], simd.rows[1][3], simd.rows[0][7], simd.rows[1][7] },
    };
    memcpy(state, state_end, sizeof(state_end));
}
