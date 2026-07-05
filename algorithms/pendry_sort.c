/*
 * Pendry Sort (Parts 5/7): merged scan-route-repair, direct-fill counting, bucket scatter + disorder repair
 * Presentation copy, extracted verbatim from ../benchmarks/pendry_benchmark.c,
 * which is the runnable source of truth for every number in the paper.
 * Benchmark standing (35 patterns, 1M int32): #2 of 12 by total, 359,174 us; 35/35 tested, most wins (11), 0 N/A, 0 fails; 4.6x introsort aggregate
 *
 * Copyright 2026 HalfHuman Draft - Pendry, S
 * Licensed under the Apache License, Version 2.0
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* --- Pendry Sort (Full, from paper v5b) --- */
/* Included as a header-style block */

static inline void pendry__isort(int32_t *a, int n) {
    for (int i = 1; i < n; i++) {
        int32_t key = a[i]; int j = i - 1;
        while (j >= 0 && a[j] > key) { a[j+1] = a[j]; j--; }
        a[j+1] = key;
    }
}

static void pendry__sift_down(int32_t *a, int start, int end) {
    int root = start;
    while (root * 2 + 1 <= end) {
        int child = root * 2 + 1, swap = root;
        if (a[swap] < a[child]) swap = child;
        if (child + 1 <= end && a[swap] < a[child+1]) swap = child+1;
        if (swap == root) return;
        int32_t t = a[root]; a[root] = a[swap]; a[swap] = t;
        root = swap;
    }
}
static void pendry__heapsort(int32_t *a, int n) {
    for (int s = (n-2)/2; s >= 0; s--) pendry__sift_down(a, s, n-1);
    for (int e = n-1; e > 0; e--) {
        int32_t t = a[e]; a[e] = a[0]; a[0] = t;
        pendry__sift_down(a, 0, e-1);
    }
}

static void pendry__introsort_loop(int32_t *a, int n, int depth) {
    while (n > 16) {
        if (depth == 0) { pendry__heapsort(a, n); return; }
        depth--;
        int32_t pivot;
        { int32_t x=a[0], y=a[n/2], z=a[n-1];
          if(x>y){int32_t t=x;x=y;y=t;}
          if(y>z){int32_t t=y;y=z;z=t;}
          if(x>y){int32_t t=x;x=y;y=t;}
          pivot=y; }
        int i = 0, j = n - 1;
        while (1) {
            while (a[i] < pivot) i++;
            while (a[j] > pivot) j--;
            if (i >= j) break;
            int32_t t = a[i]; a[i] = a[j]; a[j] = t; i++; j--;
        }
        int left = j+1, right = n-left;
        if (left < right) { pendry__introsort_loop(a, left, depth); a += left; n = right; }
        else { pendry__introsort_loop(a+left, right, depth); n = left; }
    }
}
static void pendry__introsort(int32_t *a, int n) {
    if (n <= 1) return;
    int depth = 0;
    for (int k = n; k > 1; k >>= 1) depth++;
    depth *= 2;
    pendry__introsort_loop(a, n, depth);
    pendry__isort(a, n);
}

static inline void pendry__disorder_repair(int32_t *a, int n) {
    if (n <= 1) return;
    int first_inv = -1;
    for (int i = 0; i < n-1; i++)
        if (a[i] > a[i+1]) { first_inv = i; break; }
    if (first_inv < 0) return;
    if (n <= 24) { pendry__isort(a, n); return; }
    const int W = 4;
    int lo = n, hi = 0;
    for (int i = first_inv; i < n-1; i++) {
        if (a[i] > a[i+1]) {
            int s = (i-W >= 0) ? i-W : 0;
            int e = (i+W+2 <= n) ? i+W+2 : n;
            if (s < lo) lo = s;
            if (e > hi) hi = e;
        }
    }
    if (hi > lo) pendry__isort(a + lo, hi - lo);
    for (int i = 0; i < n-1; i++)
        if (a[i] > a[i+1]) { pendry__isort(a, n); return; }
}

static void pendry__bucket_scatter(const int32_t *src, int32_t *dst, int n,
                                    int32_t lo, uint64_t range) {
    int nb = n / 8;
    if (nb < 16) nb = 16;
    double scale = (double)nb / (double)range;
    int *bc = (int *)calloc(nb, sizeof(int));
    for (int i = 0; i < n; i++) {
        int b = (int)(((double)src[i] - (double)lo) * scale);
        if (b < 0) b = 0; if (b >= nb) b = nb - 1;
        bc[b]++;
    }
    int *off = (int *)malloc(nb * sizeof(int));
    int *cur = (int *)malloc(nb * sizeof(int));
    off[0] = 0;
    for (int i = 1; i < nb; i++) off[i] = off[i-1] + bc[i-1];
    memcpy(cur, off, nb * sizeof(int));
    for (int i = 0; i < n; i++) {
        int b = (int)(((double)src[i] - (double)lo) * scale);
        if (b < 0) b = 0; if (b >= nb) b = nb - 1;
        dst[cur[b]++] = src[i];
    }
    for (int b = 0; b < nb; b++) {
        int len = bc[b];
        if (len <= 1) continue;
        if (len > 64) pendry__introsort(dst + off[b], len);
        else pendry__disorder_repair(dst + off[b], len);
    }
    free(bc); free(off); free(cur);
}

static void pendry__full_sort(const int32_t *src, int32_t *dst, int n,
                               int32_t sample_lo, int32_t sample_hi) {
    uint64_t sample_range = (uint64_t)sample_hi - sample_lo + 1;
    if (sample_range > (uint64_t)n * 8) {
        /* fast bucket path */
        int32_t lo = src[0], hi = src[0];
        for (int i = 1; i < n; i++) { if (src[i] < lo) lo = src[i]; if (src[i] > hi) hi = src[i]; }
        uint64_t range = (uint64_t)hi - lo + 1;
        if (range <= (uint64_t)n * 4) {
            int *count = (int *)calloc(range, sizeof(int));
            for (int i = 0; i < n; i++) count[src[i] - lo]++;
            int pos = 0;
            for (uint64_t v = 0; v < range; v++) { int c = count[v]; for (int j = 0; j < c; j++) dst[pos++] = lo + (int32_t)v; }
            free(count); return;
        }
        pendry__bucket_scatter(src, dst, n, lo, range);
        return;
    }
    int32_t lo = src[0], hi = src[0];
    for (int i = 1; i < n; i++) { if (src[i] < lo) lo = src[i]; if (src[i] > hi) hi = src[i]; }
    uint64_t range = (uint64_t)hi - lo + 1;
    if (range <= 1) { for (int i = 0; i < n; i++) dst[i] = lo; return; }
    if (range <= (uint64_t)n * 4) {
        int *count = (int *)calloc(range, sizeof(int));
        for (int i = 0; i < n; i++) count[src[i] - lo]++;
        int pos = 0;
        for (uint64_t v = 0; v < range; v++) { int c = count[v]; for (int j = 0; j < c; j++) dst[pos++] = lo + (int32_t)v; }
        free(count); return;
    }
    pendry__bucket_scatter(src, dst, n, lo, range);
}

static void pendry__window_carry_sort(const int32_t *src, int32_t *dst, int n,
    int window_lo, int window_hi, int bail_pos,
    int32_t sample_lo, int32_t sample_hi) {
    int32_t lo, hi;
    if (window_lo > 0) { lo = src[0]; hi = src[window_lo - 1]; }
    else { lo = src[0]; hi = src[0]; }
    int scan_start = (window_lo > 0) ? window_lo : 0;
    for (int i = scan_start; i < n; i++) { if (src[i] < lo) lo = src[i]; if (src[i] > hi) hi = src[i]; }
    uint64_t range = (uint64_t)hi - lo + 1;
    if (range <= 1) { for (int i = 0; i < n; i++) dst[i] = lo; return; }
    if (range <= (uint64_t)n * 4) {
        int *count = (int *)calloc(range, sizeof(int));
        for (int i = 0; i < n; i++) count[src[i] - lo]++;
        int pos = 0;
        for (uint64_t v = 0; v < range; v++) { int c = count[v]; for (int j = 0; j < c; j++) dst[pos++] = lo + (int32_t)v; }
        free(count); return;
    }
    pendry__bucket_scatter(src, dst, n, lo, range);
}

static void pendry_sort(const int32_t *src, int32_t *dst, int n) {
    if (n <= 1) { if (n == 1) dst[0] = src[0]; return; }
    if (n < 64) { memcpy(dst, src, n * sizeof(int32_t)); pendry__isort(dst, n); return; }

    int sample_inv = 0;
    int32_t sample_lo = src[0], sample_hi = src[0];
    int step = n / 16; if (step < 1) step = 1;
    for (int i = 0; i < n-1 && i < 16 * step; i += step) {
        int32_t a = src[i], b = src[i+1];
        if (a > b) sample_inv++;
        if (a < sample_lo) sample_lo = a; if (a > sample_hi) sample_hi = a;
        if (b < sample_lo) sample_lo = b; if (b > sample_hi) sample_hi = b;
    }

    if (sample_inv >= 14) {
        int is_rev = 1;
        for (int i = 0; i < n-1; i++) if (src[i] < src[i+1]) { is_rev = 0; break; }
        if (is_rev) { for (int i = 0; i < n; i++) dst[i] = src[n-1-i]; return; }
        pendry__full_sort(src, dst, n, sample_lo, sample_hi); return;
    }

    if (sample_inv <= 8) {
        const int W = 4;
        int inv_count = 0, inv_limit = n / 64;
        int window_lo = n, window_hi = 0;
        for (int i = 0; i < n-1; i++) {
            if (src[i] > src[i+1]) {
                inv_count++;
                if (inv_count > inv_limit) {
                    if (window_lo >= n / 8 && window_lo < n) {
                        pendry__window_carry_sort(src, dst, n, window_lo,
                            window_hi > 0 ? window_hi : i, i, sample_lo, sample_hi);
                        return;
                    }
                    pendry__full_sort(src, dst, n, sample_lo, sample_hi); return;
                }
                int s = (i-W >= 0) ? i-W : 0;
                int e = (i+W+2 <= n) ? i+W+2 : n;
                if (s < window_lo) window_lo = s;
                if (e > window_hi) window_hi = e;
            }
        }
        if (inv_count == 0) { memcpy(dst, src, n * sizeof(int32_t)); return; }
        int region = window_hi - window_lo;
        if (region > n / 2) {
            if (window_lo >= n / 8) {
                pendry__window_carry_sort(src, dst, n, window_lo, window_hi, n-1, sample_lo, sample_hi);
                return;
            }
            pendry__full_sort(src, dst, n, sample_lo, sample_hi); return;
        }
        memcpy(dst, src, n * sizeof(int32_t));
        if (region > 64) pendry__introsort(dst + window_lo, region);
        else pendry__isort(dst + window_lo, region);
        int ok = 1;
        if (window_lo > 0 && dst[window_lo-1] > dst[window_lo]) ok = 0;
        if (window_hi < n && dst[window_hi-1] > dst[window_hi]) ok = 0;
        if (!ok) pendry__introsort(dst, n);
        return;
    }

    pendry__full_sort(src, dst, n, sample_lo, sample_hi);
}
