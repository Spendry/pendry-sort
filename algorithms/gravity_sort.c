/*
 * Gravity Sort (Part 3): counting sort, rediscovered and named as such
 * Presentation copy, extracted verbatim from ../benchmarks/pendry_benchmark.c,
 * which is the runnable source of truth for every number in the paper.
 * Benchmark standing (35 patterns, 1M int32): #1 of 12 by total time, 241,526 us; 34/35 tested, 10 wins, 0 fails
 *
 * Copyright 2026 HalfHuman Draft - Pendry, S
 * Licensed under the Apache License, Version 2.0
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* --- Gravity Sort / Counting Sort (Post 3) --- */

static int gravity_sort(int32_t *a, int n) {
    if (n <= 1) return 1;
    int32_t lo = a[0], hi = a[0];
    for (int i = 1; i < n; i++) {
        if (a[i] < lo) lo = a[i];
        if (a[i] > hi) hi = a[i];
    }
    if (lo == hi) return 1;
    int64_t range = (int64_t)hi - lo + 1;
    if (range > (int64_t)n * 4) return 0; /* N/A: range too large */
    int *weight = (int *)calloc(range, sizeof(int));
    if (!weight) return 0;
    for (int i = 0; i < n; i++) weight[a[i] - lo]++;
    int pos = 0;
    for (int64_t w = 0; w < range; w++)
        for (int c = 0; c < weight[w]; c++)
            a[pos++] = lo + (int32_t)w;
    free(weight);
    return 1;
}
