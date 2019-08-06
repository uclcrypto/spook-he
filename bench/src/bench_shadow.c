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

#include "primitives.h"


#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <time.h>
#include <stdint.h>

#ifndef N_ITER
#define N_ITER (10*1000*1000)
#endif // N_ITER

typedef struct timespec timespec;

double time_per_iter(timespec* t0, timespec* tend, uint64_t n_iter) {
    int64_t dsec = tend->tv_sec - t0->tv_sec;
    int64_t dnsec = tend->tv_nsec - t0->tv_nsec + 1000*1000*1000*dsec;
    double nsec_per_iter = ((double) dnsec) / ((double) n_iter);
    return nsec_per_iter;
}


int main(void) {
    shadow_state state;
    memset(state, 0, sizeof(state));
    uint64_t n_iter = N_ITER;

    timespec t0, tend;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t0);

    for (uint64_t i=0; i<n_iter; i++) {
        shadow(state);
    }

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tend);
    double perf = time_per_iter(&t0, &tend, n_iter);
    printf("ns/iter: %.4g\n", perf);

    return 0;
}

