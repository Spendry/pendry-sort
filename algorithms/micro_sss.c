/*
 * Micro SSS (Part 2): the tenet sort, insertion sort with reverse detection
 * Presentation copy, extracted verbatim from ../benchmarks/pendry_benchmark.c,
 * which is the runnable source of truth for every number in the paper.
 * Benchmark standing (35 patterns, 1M int32): 9/35 tested, 26 N/A; total 56,675 us on its tested patterns
 *
 * Copyright 2026 HalfHuman Draft - Pendry, S
 * Licensed under the Apache License, Version 2.0
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* --- Micro SSS (Post 3: Who Decided What Sorted Is) --- */

static void micro_sss(int32_t *a, int n) {
    if (n <= 1) return;
    int d = 0;
    for (int i = 0; i < 50 && i < n - 1; i += n / 50 + 1)
        if (a[i] > a[i + 1]) d++;
    if (d > 40)
        for (int i = 0; i < n / 2; i++) {
            int32_t t = a[i]; a[i] = a[n-1-i]; a[n-1-i] = t;
        }
    for (int i = 0; i < n - 1; i++) {
        if (a[i] <= a[i + 1]) continue;
        int32_t v = a[i + 1];
        int j = i;
        while (j >= 0 && a[j] > v) { a[j + 1] = a[j]; j--; }
        a[j + 1] = v;
    }
}
