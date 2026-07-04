/*
    fftbench.c

    Radix-2 Cooley-Tukey FFT benchmark.
    Iterative, in-place, decimation-in-time.
*/

#ifndef LINK
#include "main.ic"
#else
extern double bench_result;
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 2^N points. N=20 gives ~1M points, ~20M complex mults per FFT.
#define LOG2N 20
#define N     (1 << LOG2N)

static double *re;
static double *im;

// Bit-reverse permutation: swap a[i] with a[reverse(i)] for i < reverse(i).
static unsigned bit_reverse(unsigned x, unsigned bits) {
    unsigned r = 0;
    for (unsigned i = 0; i < bits; ++i) {
        r = (r << 1) | (x & 1u);
        x >>= 1;
    }
    return r;
}

static void fft(double *re, double *im, unsigned n, unsigned log2n) {
    // Bit-reverse permutation.
    for (unsigned i = 0; i < n; ++i) {
        unsigned j = bit_reverse(i, log2n);
        if (j > i) {
            double tr = re[i]; re[i] = re[j]; re[j] = tr;
            double ti = im[i]; im[i] = im[j]; im[j] = ti;
        }
    }

    // Butterflies. For each stage, len = 2, 4, 8, ..., n.
    for (unsigned len = 2; len <= n; len <<= 1) {
        unsigned half = len >> 1;
        double theta = -2.0 * M_PI / (double)len;
        double wpr = cos(theta);
        double wpi = sin(theta);
        for (unsigned i = 0; i < n; i += len) {
            double wr = 1.0, wi = 0.0;
            for (unsigned k = 0; k < half; ++k) {
                unsigned a = i + k;
                unsigned b = a + half;
                double tr = wr * re[b] - wi * im[b];
                double ti = wr * im[b] + wi * re[b];
                re[b] = re[a] - tr;
                im[b] = im[a] - ti;
                re[a] += tr;
                im[a] += ti;
                // w *= (wpr + i wpi)
                double nwr = wr * wpr - wi * wpi;
                double nwi = wr * wpi + wi * wpr;
                wr = nwr;
                wi = nwi;
            }
        }
    }
}

char const* name = " fftbench (radix-2 Cooley-Tukey, N=2^20)";

void init() {
    re = (double *)malloc(sizeof(double) * N);
    im = (double *)malloc(sizeof(double) * N);
    // Deterministic input: a mix of sines.
    for (unsigned i = 0; i < N; ++i) {
        double x = (double)i / (double)N;
        re[i] = sin(2.0 * M_PI * 3.0 * x) + 0.5 * sin(2.0 * M_PI * 17.0 * x);
        im[i] = 0.0;
    }
}

void run() {
    fft(re, im, N, LOG2N);
}

void clean() {
    // Prevent dead-code elimination: fold the ENTIRE spectrum into the
    // printed checksum. Observing only a couple of elements would in
    // principle let a compiler skip computing the unobserved outputs of
    // the final butterfly stages. Runs in clean(), outside the measured
    // region.
    double sum = 0.0;
    for (unsigned i = 0; i < N; ++i)
        sum += re[i] + im[i];
    bench_result = sum;
    free(re);
    free(im);
}
