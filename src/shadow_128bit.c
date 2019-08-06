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

#ifdef BENCH_IACA
#include "iacaMarks.h"
#else
#define IACA_START
#define IACA_END
#endif

#include "primitives.h"

#define SHADOW_NS 6                   // Number of steps
#define SHADOW_NR 2 * SHADOW_NS       // Number of rounds

#define LS_ROWS 4      // Rows in the LS design
#define LS_ROW_BYTES 4 // number of bytes per row in the LS design
#define MLS_BUNDLES                                                            \
  (SHADOW_NBYTES / (LS_ROWS* LS_ROW_BYTES)) // Bundles in the mLS design

typedef uint32_t row_set __attribute__ ((vector_size (16)));

typedef struct __attribute__((aligned(64))) shadow_simd {
    row_set rows[4];
} shadow_simd;

static void sbox_layer_simd(shadow_simd* simd);
static void lbox_simd(row_set* x, row_set* y);
static void lbox_layer_simd(shadow_simd* simd);
static void add_rc_simd(shadow_simd* simd, unsigned int round);
static void dbox_mls_layer_simd(shadow_simd *simd);

#if SMALL_PERM==0
#define RS0 { 0, 0, 0, 0 }
#define RS1S { 1, 2, 4, 8 }
#else
#define RS0 { 0, 0, 0, 0 }
#define RS1S { 1, 2, 4, 0 }
#endif // SMALL_PERM==0
static const shadow_simd shadow_simd_rc[SHADOW_NR] = {
    {{ RS1S, RS0, RS0, RS0 }}, // 0
    {{ RS0, RS1S, RS0, RS0 }}, // 1
    {{ RS0, RS0, RS1S, RS0 }}, // 2
    {{ RS0, RS0, RS0, RS1S }}, // 3
    {{ RS1S, RS1S, RS0, RS0 }}, // 4
    {{ RS0, RS1S, RS1S, RS0 }}, // 5
    {{ RS0, RS0, RS1S, RS1S }}, // 6
    {{ RS1S, RS1S, RS0, RS1S }}, // 7
    {{ RS1S, RS0, RS1S, RS0 }}, // 8
    {{ RS0, RS1S, RS0, RS1S }}, // 9
    {{ RS1S, RS1S, RS1S, RS0 }}, // 10
    {{ RS0, RS1S, RS1S, RS1S }} // 11
};


static void sbox_layer_simd(shadow_simd* simd) {
  row_set y1 = (simd->rows[0] & simd->rows[1]) ^ simd->rows[2];
  row_set y0 = (simd->rows[3] & simd->rows[0]) ^ simd->rows[1];
  row_set y3 = (y1 & simd->rows[3]) ^ simd->rows[0];
  row_set y2 = (y0 & y1) ^ simd->rows[3];
  simd->rows[0] = y0;
  simd->rows[1] = y1;
  simd->rows[2] = y2;
  simd->rows[3] = y3;
}

#define ROT32(x,n) (((x)>>(n))|((x)<<(32-(n))))
static void lbox_simd(row_set* x, row_set* y) {
  row_set a, b, c, d;
  a = *x ^ ROT32(*x, 12);
  b = *y ^ ROT32(*y, 12);
  a = a ^ ROT32(a, 3);
  b = b ^ ROT32(b, 3);
  a = a ^ ROT32(*x, 17);
  b = b ^ ROT32(*y, 17);
  c = a ^ ROT32(a, 31);
  d = b ^ ROT32(b, 31);
  a = a ^ ROT32(d, 26);
  b = b ^ ROT32(c, 25);
  a = a ^ ROT32(c, 15);
  b = b ^ ROT32(d, 15);
  *x = a;
  *y = b;
}

static void lbox_layer_simd(shadow_simd* simd) {
  lbox_simd(&simd->rows[0], &simd->rows[1]);
  lbox_simd(&simd->rows[2], &simd->rows[3]);
}

static void add_rc_simd(shadow_simd* simd, unsigned int round) {
  for (unsigned int i = 0; i < LS_ROWS; i++) {
      simd->rows[i] ^= shadow_simd_rc[round].rows[i];
  }
}

#if SMALL_PERM==0
static const row_set dbox_shuffle1 = { 1, 0, 0, 0 };
static const row_set dbox_shuffle2 = { 2, 2, 1, 1 };
static const row_set dbox_shuffle3 = { 3, 3, 3, 2 };
#else
static const row_set dbox_shuffle1 = { 0, 0, 0, 3 };
static const row_set dbox_shuffle2 = { 1, 2, 1, 3 };
static const row_set dbox_shuffle3 = { 2, 3, 3, 3 };
#endif // SMALL_PERM==0
static void dbox_mls_layer_simd(shadow_simd *simd) {
  for (unsigned int row = 0; row < LS_ROWS; row++) {
      row_set a = __builtin_shuffle(simd->rows[row], dbox_shuffle1);
      row_set b = __builtin_shuffle(simd->rows[row], dbox_shuffle2);
      row_set c = __builtin_shuffle(simd->rows[row], dbox_shuffle3);
      simd->rows[row] = a ^ b ^ c;
  }
}
void shadow(shadow_state state) {
#if SMALL_PERM==0
    shadow_simd simd = {
        {
            { state[0][0], state[1][0], state[2][0], state[3][0] },
            { state[0][1], state[1][1], state[2][1], state[3][1] },
            { state[0][2], state[1][2], state[2][2], state[3][2] },
            { state[0][3], state[1][3], state[2][3], state[3][3] }
        }
    };
#else
    shadow_simd simd = {
        {
            { state[0][0], state[1][0], state[2][0], 0 },
            { state[0][1], state[1][1], state[2][1], 0 },
            { state[0][2], state[1][2], state[2][2], 0 },
            { state[0][3], state[1][3], state[2][3], 0 }
        }
    };
#endif // SMALL_PERM==0
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
    row_set res0 = { simd.rows[0][0], simd.rows[1][0], simd.rows[2][0], simd.rows[3][0] };
    row_set res1 = { simd.rows[0][1], simd.rows[1][1], simd.rows[2][1], simd.rows[3][1] };
    row_set res2 = { simd.rows[0][2], simd.rows[1][2], simd.rows[2][2], simd.rows[3][2] };
    row_set res3 = { simd.rows[0][3], simd.rows[1][3], simd.rows[2][3], simd.rows[3][3] };
    memcpy(state[0], &res0, sizeof(row_set));
    memcpy(state[1], &res1, sizeof(row_set));
    memcpy(state[2], &res2, sizeof(row_set));
#if SMALL_PERM==0
    memcpy(state[3], &res3, sizeof(row_set));
#endif // SMALL_PERM==0
}
