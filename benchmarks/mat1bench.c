/*
    mat1bench

    Written by Scott Robert Ladd.
    No rights reserved. This is public domain software, for use by anyone.

    A number-crunching benchmark that can be used as a fitness test for
    evolving optimal compiler options via genetic algorithm.
    
    Nothing special here -- just a brute-force matrix multiply.

    Note that the code herein is design for the purpose of testing 
    computational performance; error handling and other such "niceties"
    is virtually non-existent.

    Actual benchmark results can be found at:
            http://www.coyotegulch.com

    Please do not use this information or algorithm in any way that might
    upset the balance of the universe or otherwise result in gastric upset.
*/

#ifndef LINK
#include "main.ic"
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

// adjust size of test for environment
#define N 450

// embedded random number generator; ala Park and Miller
static       int32_t seed = 1325;
static const int32_t IA   = 16807;
static const int32_t IM   = 2147483647;
static const double  AM   = 4.65661287525E-10;
static const int32_t IQ   = 127773;
static const int32_t IR   = 2836;
static const int32_t MASK = 123459876;

static double random_double()
{
    int32_t k;
    double result;
    
    seed ^= MASK;
    k = seed / IQ;
    seed = IA * (seed - k * IQ) - IR * k;
    
    if (seed < 0)
        seed += IM;
    
    result = AM * seed;
    seed ^= MASK;
    
    return result;
}

double a[N][N];
double b[N][N];
double c[N][N];

char const* name = "mat1bench (Std. C)";

void init() {
    for (unsigned i = 0; i < N; ++i)
    {
        for (unsigned j = 0; j < N; ++j)
        {
            a[i][j] = random_double();
            b[i][j] = random_double();
            c[i][j] = 0.0;
        }
    }   
}

void run() {
    // generate mandelbrot set
    for (unsigned i = 0; i < N; ++i)
    {
        for (unsigned j = 0; j < N; ++j)
        {
            for (unsigned k = 0; k < N; ++k)
            {
                c[i][j] = c[i][j] + a[i][k] * b[k][j];
            }
        }
    }

    // Prevent dead-code elimination: observe the result matrix. Without
    // this, c[][] is written but never read and the multiply can be elided.
    double sum = 0.0;
    for (unsigned i = 0; i < N; ++i)
        for (unsigned j = 0; j < N; ++j)
            sum += c[i][j];
    volatile double sink = sum;
    (void)sink;
}

void clean() {
}
