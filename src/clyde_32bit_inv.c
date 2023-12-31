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

#define CLYDE_128_NS 6                // Number of steps
#define CLYDE_128_NR 2 * CLYDE_128_NS // Number of rounds

// Round constants for Clyde-128
static const uint32_t clyde128_rc[CLYDE_128_NR][LS_ROWS] = {
  { 1, 0, 0, 0 }, // 0
  { 0, 1, 0, 0 }, // 1
  { 0, 0, 1, 0 }, // 2
  { 0, 0, 0, 1 }, // 3
  { 1, 1, 0, 0 }, // 4
  { 0, 1, 1, 0 }, // 5
  { 0, 0, 1, 1 }, // 6
  { 1, 1, 0, 1 }, // 7
  { 1, 0, 1, 0 }, // 8
  { 0, 1, 0, 1 }, // 9
  { 1, 1, 1, 0 }, // 10
  { 0, 1, 1, 1 }  // 11
};

// Apply a inverse S-box layer to a Clyde-128 state.
static void sbox_layer_inv(uint32_t* state) {
  uint32_t y3 = (state[0] & state[1]) ^ state[2];
  uint32_t y0 = (state[1] & y3) ^ state[3];
  uint32_t y1 = (y3 & y0) ^ state[0];
  uint32_t y2 = (y0 & y1) ^ state[1];
  state[0] = y0;
  state[1] = y1;
  state[2] = y2;
  state[3] = y3;
}

// Apply a inverse L-box to a pair of Clyde-128 rows.
#define ROT32(x,n) ((uint32_t)(((x)>>(n))|((x)<<(32-(n)))))
static void lbox_inv(uint32_t* x, uint32_t* y) {
  uint32_t a, b, c, d;
  a = *x ^ ROT32(*x, 25);
  b = *y ^ ROT32(*y, 25);
  c = *x ^ ROT32(a, 31);
  d = *y ^ ROT32(b, 31);
  c = c ^ ROT32(a, 20);
  d = d ^ ROT32(b, 20);
  a = c ^ ROT32(c, 31);
  b = d ^ ROT32(d, 31);
  c = c ^ ROT32(b, 26);
  d = d ^ ROT32(a, 25);
  a = a ^ ROT32(c, 17);
  b = b ^ ROT32(d, 17);
  a = ROT32(a, 16);
  b = ROT32(b, 16);
  *x = a;
  *y = b;
}

#define XORLS(DEST, OP) do { \
    (DEST)[0] ^= (OP)[0]; \
    (DEST)[1] ^= (OP)[1]; \
    (DEST)[2] ^= (OP)[2]; \
    (DEST)[3] ^= (OP)[3]; } while (0)

void clyde128_decrypt(clyde128_state state, const clyde128_state t, const unsigned char* k) {
  // Key schedule
  clyde128_state k_st;
  memcpy(k_st, k, CLYDE128_NBYTES);
  clyde128_state tk[3] = {
      { t[0], t[1], t[2], t[3] },
      { t[0] ^ t[2], t[1] ^ t[3], t[0], t[1] },
      { t[2], t[3], t[0] ^ t[2], t[1] ^ t[3] }
  };
  XORLS(tk[0], k);
  XORLS(tk[1], k);
  XORLS(tk[2], k);

  // Datapath
  for (unsigned int s = 0; s < CLYDE_128_NS; s++) {
      IACA_START
      unsigned int r = 2 * s;
      unsigned int off = (s+1) % 3;
      XORLS(state, tk[off]);

      XORLS(state, clyde128_rc[r+1]);
      lbox_inv(&state[0], &state[1]);
      lbox_inv(&state[2], &state[3]);
      sbox_layer_inv(state);
      XORLS(state, clyde128_rc[r]);
      lbox_inv(&state[0], &state[1]);
      lbox_inv(&state[2], &state[3]);
      sbox_layer_inv(state);
  }
  IACA_END
  XORLS(state, tk[0]);
}
