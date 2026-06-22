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
#include <stdint.h>
#include <string.h>
#include <math.h>

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

typedef struct
{
    double x;
    double y;
    double z;
} v_t;

inline static double distance(const v_t a, const v_t b)
{
    return sqrt(((b.x - a.x) * (b.x - a.x)) + 
	        ((b.y - a.y) * (b.y - a.y)) + 
	        ((b.z - a.z) * (b.z - a.z)));
}

#define ARRAY_SIZE 4000

static v_t v1[ARRAY_SIZE];
static v_t v2[ARRAY_SIZE];
static double r[ARRAY_SIZE];

char const* name = "distbench (Std. C)";

void init() {
    // initialize
    for (int i = 0; i < ARRAY_SIZE; ++i)
    {
        v1[i].x = random_double();
        v1[i].y = random_double();
        v1[i].z = random_double();
        
        v2[i].x = random_double();
        v2[i].y = random_double();
        v2[i].z = random_double();
    }
}

void run() {
    // time this
    for (int i = 0; i < ARRAY_SIZE; ++i)
    {
        r[i] = 0.0;

        for (int j = 0; j < ARRAY_SIZE; ++j)
            r[i] += distance(v1[i], v2[j]);
    }

    // Prevent dead-code elimination: observe every result. Without this,
    // Clang sees that the static array r[] is never read and eliminates
    // the whole loop nest.
    double sum = 0.0;
    for (int i = 0; i < ARRAY_SIZE; ++i)
        sum += r[i];
    volatile double sink = sum;
    (void)sink;
}

void clean() {
}
