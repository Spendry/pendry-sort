/*
 * Safe-Slot Sort (Parts 1-2): windowed inversion repair
 * Presentation copy, extracted verbatim from ../benchmarks/pendry_benchmark.c,
 * which is the runnable source of truth for every number in the paper.
 * Benchmark standing (35 patterns, 1M int32): 19/35 tested, 16 N/A; total 194,628 us on its tested patterns
 *
 * Copyright 2026 HalfHuman Draft - Pendry, S
 * Licensed under the Apache License, Version 2.0
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int cmp_int32(const void *a, const void *b) {
    int32_t x = *(const int32_t *)a, y = *(const int32_t *)b;
    return (x > y) - (x < y);
}

/* --- Safe-Slot Sort C (Post 2: The C Revelation, base version) --- */

static void safe_slot_sort(int32_t *arr, int n) {
    if (n <= 1) return;

    /* Presort check */
    int desc = 0, samp = 0, step = n / 150;
    if (step < 1) step = 1;
    for (int i = 0; i < n - 1 && samp < 150; i += step) {
        if (arr[i] > arr[i + 1]) desc++;
        samp++;
    }
    if (samp > 0 && (double)desc / samp > 0.8)
        for (int i = 0; i < n / 2; i++) {
            int32_t t = arr[i]; arr[i] = arr[n-1-i]; arr[n-1-i] = t;
        }

    int *mask_buf = (int *)malloc(n * sizeof(int));
    int *inv_buf = (int *)malloc(n * sizeof(int));
    int *idx_buf = (int *)malloc(n * sizeof(int));
    int32_t *val_buf = (int32_t *)malloc(n * sizeof(int32_t));

    for (int iteration = 0; iteration < 100; iteration++) {
        int num_inv = 0;
        for (int i = 0; i < n - 1; i++)
            if (arr[i] > arr[i + 1])
                inv_buf[num_inv++] = i;
        if (num_inv == 0) break;

        memset(mask_buf, 0, n * sizeof(int));
        for (int i = 0; i < num_inv; i++) {
            int s = inv_buf[i] - 8 < 0 ? 0 : inv_buf[i] - 8;
            int e = inv_buf[i] + 10 > n ? n : inv_buf[i] + 10;
            for (int j = s; j < e; j++) mask_buf[j] = 1;
        }

        int cnt = 0;
        for (int i = 0; i < n; i++)
            if (mask_buf[i]) { idx_buf[cnt] = i; val_buf[cnt] = arr[i]; cnt++; }

        qsort(val_buf, cnt, sizeof(int32_t), cmp_int32);
        for (int i = 0; i < cnt; i++) arr[idx_buf[i]] = val_buf[i];
    }

    free(mask_buf); free(inv_buf); free(idx_buf); free(val_buf);
}
