/*
 * CAS-Binary (Part 4): the cascade replaced by binary search + memmove
 * Presentation copy, extracted verbatim from ../benchmarks/pendry_benchmark.c,
 * which is the runnable source of truth for every number in the paper.
 * Benchmark standing (35 patterns, 1M int32): 9/35 tested, 26 N/A; lowest per-pattern cost in the suite, 18,205 us total
 *
 * Copyright 2026 HalfHuman Draft - Pendry, S
 * Licensed under the Apache License, Version 2.0
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* --- CAS Binary (Post 4) --- */

static void cas_binary(int32_t *a, int n) {
    if (n <= 1) return;
    int desc = 0;
    for (int i = 0; i < n - 1; i += n / 50 + 1)
        if (a[i] > a[i + 1]) desc++;
    if (desc > 40)
        for (int i = 0; i < n / 2; i++) {
            int32_t t = a[i]; a[i] = a[n-1-i]; a[n-1-i] = t;
        }
    for (int i = 0; i < n - 1; i++) {
        if (a[i] <= a[i + 1]) continue;
        int32_t v = a[i + 1];
        int left = 0, right = i + 1;
        while (left < right) {
            int mid = (left + right) >> 1;
            if (a[mid] <= v) left = mid + 1;
            else right = mid;
        }
        if (left <= i) {
            memmove(&a[left + 1], &a[left], (i + 1 - left) * sizeof(int32_t));
            a[left] = v;
        }
    }
}
