/*
 * Little Pendry (Part 7): the 87-line in-place compression of the philosophy
 * Presentation copy, extracted verbatim from ../benchmarks/pendry_benchmark.c,
 * which is the runnable source of truth for every number in the paper.
 * Benchmark standing (35 patterns, 1M int32): 474,821 us total; 33/35 tested, 2 wins, 2 FAILs (Two Blocks, Outlier)
 *
 * Copyright 2026 HalfHuman Draft - Pendry, S
 * Licensed under the Apache License, Version 2.0
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* --- Little Pendry (Addendum: 87-line in-place variant) --- */

static void little_pendry(int32_t *a, int n) {
    if (n <= 1) return;
    int sinv = 0, step = n / 16;
    int32_t lo = a[0], hi = a[0];
    if (step < 1) step = 1;
    for (int i = 0; i < n - 1 && i < 16 * step; i += step) {
        if (a[i] > a[i+1]) sinv++;
        if (a[i] < lo) lo = a[i]; if (a[i] > hi) hi = a[i];
        if (a[i+1] < lo) lo = a[i+1]; if (a[i+1] > hi) hi = a[i+1];
    }
    if (sinv >= 14) {
        int rev = 1;
        for (int i = 0; i < n - 1; i++) if (a[i] < a[i+1]) { rev = 0; break; }
        if (rev) { for (int i = 0; i < n/2; i++) { int32_t t = a[i]; a[i] = a[n-1-i]; a[n-1-i] = t; } return; }
    }
    int inv = 0, wlo = n, whi = 0;
    for (int i = 0; i < n - 1; i++) {
        if (a[i] > a[i+1]) {
            inv++;
            int s = (i > 3) ? i - 3 : 0;
            int e = (i + 5 < n) ? i + 5 : n;
            if (s < wlo) wlo = s;
            if (e > whi) whi = e;
            if (a[i] < lo) lo = a[i]; if (a[i] > hi) hi = a[i];
            if (a[i+1] < lo) lo = a[i+1]; if (a[i+1] > hi) hi = a[i+1];
        }
    }
    if (inv == 0) return;
    int window = whi - wlo;
    if (inv <= n/64 && window < n/2) {
        for (int i = wlo; i < whi - 1; i++) {
            if (a[i] <= a[i+1]) continue;
            int32_t v = a[i+1];
            int left = wlo, right = i + 1;
            while (left < right) { int mid = (left + right) >> 1; if (a[mid] <= v) left = mid + 1; else right = mid; }
            memmove(&a[left+1], &a[left], (i+1-left) * sizeof(int32_t));
            a[left] = v;
        }
        if ((wlo > 0 && a[wlo-1] > a[wlo]) || (whi < n && a[whi-1] > a[whi])) {
            int xlo = (wlo > 0) ? wlo - 1 : 0;
            int xhi = (whi < n) ? whi + 1 : n;
            for (int i = xlo + 1; i < xhi; i++) {
                int32_t v = a[i]; int j = i - 1;
                while (j >= xlo && a[j] > v) { a[j+1] = a[j]; j--; }
                a[j+1] = v;
            }
        }
        return;
    }
    for (int i = 0; i < n; i++) { if (a[i] < lo) lo = a[i]; if (a[i] > hi) hi = a[i]; }
    long long range = (long long)hi - lo + 1;
    if (range > 0 && range <= (long long)n * 4) {
        int *cnt = (int *)calloc((int)range, sizeof(int));
        if (cnt) {
            for (int i = 0; i < n; i++) cnt[a[i] - lo]++;
            int pos = 0;
            for (int v = 0; v < (int)range; v++)
                for (int c = 0; c < cnt[v]; c++) a[pos++] = v + lo;
            free(cnt); return;
        }
    }
    /* Shellsort fallback (Ciura gaps) */
    {
        int gaps[] = {510774,227011,100894,44842,19930,8858,3937,1750,701,301,132,57,23,10,4,1};
        int ng = sizeof(gaps)/sizeof(gaps[0]);
        for (int g = 0; g < ng; g++) {
            int gap = gaps[g]; if (gap >= n) continue;
            for (int i = gap; i < n; i++) {
                int32_t v = a[i]; int j = i;
                while (j >= gap && a[j-gap] > v) { a[j] = a[j-gap]; j -= gap; }
                a[j] = v;
            }
        }
    }
}
