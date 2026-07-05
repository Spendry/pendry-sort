/*
 * CAS-Core (Part 4): Consequence-as-Action Sort; this IS insertion sort
 * Presentation copy, extracted verbatim from ../benchmarks/pendry_benchmark.c,
 * which is the runnable source of truth for every number in the paper.
 * Benchmark standing (35 patterns, 1M int32): 9/35 tested, 26 N/A; 7 pattern wins; total 55,067 us on its tested patterns
 *
 * Copyright 2026 HalfHuman Draft - Pendry, S
 * Licensed under the Apache License, Version 2.0
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* --- CAS Core (Post 4: Why Look) --- */

static void cas_core(int32_t *a, int n) {
    if (n <= 1) return;
    int desc = 0;
    for (int i = 0; i < n - 1; i += n / 50 + 1)
        if (a[i] > a[i + 1]) desc++;
    if (desc > 40)
        for (int i = 0; i < n / 2; i++) {
            int32_t t = a[i]; a[i] = a[n-1-i]; a[n-1-i] = t;
        }
    for (int i = 0; i < n - 1; i++)
        if (a[i] > a[i + 1]) {
            int32_t v = a[i + 1];
            int j = i;
            while (j >= 0 && a[j] > v) { a[j + 1] = a[j]; j--; }
            a[j + 1] = v;
        }
}
