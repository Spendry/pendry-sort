/*
 * Sieve Sort v3 (Part 6): window detector + three-path routing, stable, type-agnostic in principle (int32 shown)
 * Presentation copy, extracted verbatim from ../benchmarks/pendry_benchmark.c,
 * which is the runnable source of truth for every number in the paper.
 * Benchmark standing (35 patterns, 1M int32): #6 of 12, 810,240 us; 35/35 tested, 4 wins, 0 N/A, 0 fails
 *
 * Copyright 2026 HalfHuman Draft - Pendry, S
 * Licensed under the Apache License, Version 2.0
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* --- Sieve Sort v3 (Post 6: Sieve Sort) --- */

static inline void sieve_isort(int32_t *a, int n) {
    for (int i = 1; i < n; i++) {
        int32_t key = a[i]; int j = i - 1;
        while (j >= 0 && a[j] > key) { a[j+1] = a[j]; j--; }
        a[j+1] = key;
    }
}

static void sieve_sift(int32_t *a, int s, int e) {
    int root = s;
    while (root * 2 + 1 <= e) {
        int child = root * 2 + 1, swap = root;
        if (a[swap] < a[child]) swap = child;
        if (child + 1 <= e && a[swap] < a[child+1]) swap = child + 1;
        if (swap == root) return;
        int32_t t = a[root]; a[root] = a[swap]; a[swap] = t;
        root = swap;
    }
}

static inline void sieve_hsort(int32_t *a, int n) {
    for (int s = (n-2)/2; s >= 0; s--) sieve_sift(a, s, n-1);
    for (int e = n-1; e > 0; e--) {
        int32_t t = a[0]; a[0] = a[e]; a[e] = t;
        sieve_sift(a, 0, e-1);
    }
}

static void sieve_intro_loop(int32_t *a, int n, int d) {
    while (n > 24) {
        if (d == 0) { sieve_hsort(a, n); return; }
        d--;
        int m = n/2;
        if (a[0] > a[m]) { int32_t t=a[0]; a[0]=a[m]; a[m]=t; }
        if (a[m] > a[n-1]) { int32_t t=a[m]; a[m]=a[n-1]; a[n-1]=t; }
        if (a[0] > a[m]) { int32_t t=a[0]; a[0]=a[m]; a[m]=t; }
        int32_t piv = a[m]; int i = 0, j = n-1;
        while (1) {
            while (a[i] < piv) i++;
            while (a[j] > piv) j--;
            if (i >= j) break;
            int32_t t = a[i]; a[i] = a[j]; a[j] = t; i++; j--;
        }
        int ls = j+1, rs = n-ls;
        if (ls < rs) { sieve_intro_loop(a, ls, d); a += ls; n = rs; }
        else { sieve_intro_loop(a+ls, rs, d); n = ls; }
    }
}

static inline void sieve_introsort(int32_t *a, int n) {
    if (n <= 1) return;
    int d = 0;
    for (int k = n; k > 1; k >>= 1) d++;
    d *= 2;
    sieve_intro_loop(a, n, d);
    sieve_isort(a, n);
}

static inline void sieve_counting_sort(int32_t *a, int n, int32_t lo, int32_t hi) {
    int range = hi - lo + 1;
    int *count = (int *)calloc(range, sizeof(int));
    for (int i = 0; i < n; i++) count[a[i] - lo]++;
    int k = 0;
    for (int i = 0; i < range; i++)
        for (int j = 0; j < count[i]; j++)
            a[k++] = i + lo;
    free(count);
}

static inline void sieve_merge(int32_t *a, int32_t *buf, int lo, int mid, int hi) {
    int len1 = mid - lo;
    memcpy(buf, a + lo, len1 * sizeof(int32_t));
    int i = 0, j = mid, k = lo;
    while (i < len1 && j < hi) {
        if (buf[i] <= a[j]) a[k++] = buf[i++];
        else a[k++] = a[j++];
    }
    while (i < len1) a[k++] = buf[i++];
}

static inline void sieve_reverse_run(int32_t *a, int lo, int hi) {
    hi--;
    while (lo < hi) {
        int32_t t = a[lo]; a[lo] = a[hi]; a[hi] = t;
        lo++; hi--;
    }
}

#define SIEVE_MAX_RUNS 4096

static void sieve_natural_merge_sort(int32_t *a, int n) {
    if (n <= 1) return;
    if (n <= 32) { sieve_isort(a, n); return; }

    int run_starts[SIEVE_MAX_RUNS];
    int nruns = 0, i = 0;

    while (i < n && nruns < SIEVE_MAX_RUNS - 1) {
        int start = i;
        if (i + 1 < n && a[i+1] < a[i]) {
            while (i + 1 < n && a[i+1] < a[i]) i++;
            i++;
            sieve_reverse_run(a, start, i);
        } else {
            while (i + 1 < n && a[i+1] >= a[i]) i++;
            i++;
        }
        int min_run = 32;
        if (i - start < min_run && i < n) {
            int end = start + min_run;
            if (end > n) end = n;
            sieve_isort(a + start, end - start);
            i = end;
        }
        run_starts[nruns++] = start;
    }
    run_starts[nruns] = n;

    if (nruns >= SIEVE_MAX_RUNS - 1) { sieve_introsort(a, n); return; }
    if (nruns <= 1) return;

    int32_t *buf = (int32_t *)malloc(n * sizeof(int32_t));
    while (nruns > 1) {
        int new_nruns = 0, new_starts[SIEVE_MAX_RUNS];
        for (int r = 0; r < nruns; r += 2) {
            new_starts[new_nruns] = run_starts[r];
            if (r + 1 < nruns)
                sieve_merge(a, buf, run_starts[r], run_starts[r+1],
                           (r + 2 < nruns + 1) ? run_starts[r+2] : n);
            new_nruns++;
        }
        new_starts[new_nruns] = n;
        memcpy(run_starts, new_starts, (new_nruns + 1) * sizeof(int));
        nruns = new_nruns;
    }
    free(buf);
}

static void sieve_sort(int32_t *a, int n) {
    if (n <= 1) return;

    int right = -1, rmax = a[0];
    int32_t gmin = a[0], gmax = a[0];
    for (int i = 1; i < n; i++) {
        if (a[i] < gmin) gmin = a[i];
        if (a[i] > gmax) gmax = a[i];
        if (a[i] < rmax) right = i;
        else rmax = a[i];
    }
    if (right == -1) return; /* already sorted */

    int left = 0;
    int32_t rmin = a[n-1];
    for (int i = n-2; i >= 0; i--) {
        if (a[i] > rmin) left = i;
        else rmin = a[i];
    }

    int wlen = right - left + 1;
    int32_t *w = a + left;

    if (wlen <= 32) { sieve_isort(w, wlen); return; }

    int32_t wmin, wmax;
    if (wlen >= n / 2) { wmin = gmin; wmax = gmax; }
    else {
        wmin = w[0]; wmax = w[0];
        for (int i = 1; i < wlen; i++) {
            if (w[i] < wmin) wmin = w[i];
            if (w[i] > wmax) wmax = w[i];
        }
    }

    long long range = (long long)wmax - wmin + 1;
    if (range > 0 && range <= wlen / 4 && range <= 1000000) {
        sieve_counting_sort(w, wlen, wmin, wmax);
        return;
    }

    sieve_natural_merge_sort(w, wlen);
}
