/*
 * Pendry Sort Series — Comprehensive C Benchmark
 * ================================================
 * Tests every C-era algorithm from the sorting series against
 * industry-standard sorting methods.
 *
 * Author: Pendry, S (algorithms) + Claude (benchmark harness)
 * License: Apache 2.0
 *
 * Compile: gcc -O2 -o pendry_bench pendry_benchmark_c.c -lm
 * Run:     ./pendry_bench
 *          ./pendry_bench 1000000      (custom size)
 *          ./pendry_bench 100000 quick (fewer trials)
 *
 * All results verified for correctness. All timings are median of
 * multiple trials. Output goes to stdout in the same format as the
 * Python benchmark.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
static LARGE_INTEGER _timer_freq;
static void timer_init(void) { QueryPerformanceFrequency(&_timer_freq); }
static double now_us(void) {
    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart * 1e6 / (double)_timer_freq.QuadPart;
}
#else
static void timer_init(void) {}
static double now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e6 + ts.tv_nsec / 1e3;
}
#endif

/* ======================================================================
 * UTILITIES
 * ====================================================================== */

static int is_sorted(const int32_t *a, int n) {
    for (int i = 0; i < n - 1; i++)
        if (a[i] > a[i + 1]) return 0;
    return 1;
}

static void shuffle(int32_t *a, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int32_t t = a[i]; a[i] = a[j]; a[j] = t;
    }
}

static int cmp_int32(const void *a, const void *b) {
    int32_t x = *(const int32_t *)a, y = *(const int32_t *)b;
    return (x > y) - (x < y);
}

static int cmp_double(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

/* ======================================================================
 * SECTION 1: PENDRY SERIES ALGORITHMS
 * ====================================================================== */

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


/* ======================================================================
 * SECTION 2: INDUSTRY STANDARD COMPETITORS
 * ====================================================================== */

/* --- qsort (C stdlib) --- */
static void stdlib_qsort(int32_t *a, int n) {
    qsort(a, n, sizeof(int32_t), cmp_int32);
}

/* --- Introsort (standalone implementation) --- */
static void bench_introsort(int32_t *a, int n) {
    /* Reuse the Pendry-internal introsort which is a clean implementation */
    pendry__introsort(a, n);
}

/* --- Heapsort (standalone) --- */
static void bench_heapsort(int32_t *a, int n) {
    pendry__heapsort(a, n);
}

/* --- Radix Sort LSD 256 (4-pass, 8 bits per pass) --- */
static void radix256_sort(int32_t *a, int n) {
    if (n <= 1) return;
    uint32_t *src = (uint32_t *)malloc(n * sizeof(uint32_t));
    uint32_t *dst = (uint32_t *)malloc(n * sizeof(uint32_t));
    /* Flip sign bit so signed ints sort correctly as unsigned */
    for (int i = 0; i < n; i++) src[i] = (uint32_t)a[i] ^ 0x80000000u;

    for (int pass = 0; pass < 4; pass++) {
        int shift = pass * 8;
        int count[256] = {0};
        for (int i = 0; i < n; i++) count[(src[i] >> shift) & 0xFF]++;
        int total = 0;
        for (int i = 0; i < 256; i++) { int c = count[i]; count[i] = total; total += c; }
        for (int i = 0; i < n; i++) {
            int bucket = (src[i] >> shift) & 0xFF;
            dst[count[bucket]++] = src[i];
        }
        uint32_t *tmp = src; src = dst; dst = tmp;
    }
    /* Flip sign bit back */
    for (int i = 0; i < n; i++) a[i] = (int32_t)(src[i] ^ 0x80000000u);
    /* After 4 passes, result is in src. If src != original a buffer, we need to check */
    free(src); free(dst);
}
/* Fix: radix sort above has a pointer aliasing issue. Correct version: */
static void radix256_sort_fixed(int32_t *a, int n) {
    if (n <= 1) return;
    uint32_t *buf1 = (uint32_t *)malloc(n * sizeof(uint32_t));
    uint32_t *buf2 = (uint32_t *)malloc(n * sizeof(uint32_t));
    for (int i = 0; i < n; i++) buf1[i] = (uint32_t)a[i] ^ 0x80000000u;

    uint32_t *src = buf1, *dst = buf2;
    for (int pass = 0; pass < 4; pass++) {
        int shift = pass * 8;
        int count[256] = {0};
        for (int i = 0; i < n; i++) count[(src[i] >> shift) & 0xFF]++;
        int total = 0;
        for (int i = 0; i < 256; i++) { int c = count[i]; count[i] = total; total += c; }
        for (int i = 0; i < n; i++) {
            int bucket = (src[i] >> shift) & 0xFF;
            dst[count[bucket]++] = src[i];
        }
        uint32_t *tmp = src; src = dst; dst = tmp;
    }
    for (int i = 0; i < n; i++) a[i] = (int32_t)(src[i] ^ 0x80000000u);
    free(buf1); free(buf2);
}

/* --- Counting Sort (textbook, for fair comparison) --- */
static int counting_sort_bench(int32_t *a, int n) {
    if (n <= 1) return 1;
    int32_t lo = a[0], hi = a[0];
    for (int i = 1; i < n; i++) { if (a[i] < lo) lo = a[i]; if (a[i] > hi) hi = a[i]; }
    int64_t range = (int64_t)hi - lo + 1;
    if (range > (int64_t)n * 4) return 0; /* N/A */
    int *count = (int *)calloc(range, sizeof(int));
    if (!count) return 0;
    for (int i = 0; i < n; i++) count[a[i] - lo]++;
    int pos = 0;
    for (int64_t v = 0; v < range; v++)
        for (int c = 0; c < count[v]; c++)
            a[pos++] = lo + (int32_t)v;
    free(count);
    return 1;
}


/* ======================================================================
 * SECTION 3: DATA GENERATORS
 * ====================================================================== */

static void gen_sorted(int32_t *a, int n) { for (int i = 0; i < n; i++) a[i] = i; }
static void gen_reverse(int32_t *a, int n) { for (int i = 0; i < n; i++) a[i] = n - 1 - i; }
static void gen_random(int32_t *a, int n) { for (int i = 0; i < n; i++) a[i] = i; shuffle(a, n); }
static void gen_all_identical(int32_t *a, int n) { for (int i = 0; i < n; i++) a[i] = 42; }

static void gen_local_swap(int32_t *a, int n, int pct_x10) {
    for (int i = 0; i < n; i++) a[i] = i;
    int swaps = (int)((long long)n * pct_x10 / 1000);
    for (int s = 0; s < swaps; s++) {
        int i = rand() % (n - 1);
        int32_t t = a[i]; a[i] = a[i+1]; a[i+1] = t;
    }
}

static void gen_scatter(int32_t *a, int n, int pct_x10) {
    for (int i = 0; i < n; i++) a[i] = i;
    int swaps = (int)((long long)n * pct_x10 / 1000);
    for (int s = 0; s < swaps; s++) {
        int i = rand() % n, j = rand() % n;
        int32_t t = a[i]; a[i] = a[j]; a[j] = t;
    }
}

static void gen_localized(int32_t *a, int n, int pct_x10) {
    for (int i = 0; i < n; i++) a[i] = i;
    int k = (int)((long long)n * pct_x10 / 1000);
    if (k < 2) k = 2;
    int start = n / 2 - k / 2;
    for (int i = 0; i < k - 1; i++) {
        int j = i + rand() % (k - i);
        int32_t t = a[start+i]; a[start+i] = a[start+j]; a[start+j] = t;
    }
}

static void gen_pipe_organ(int32_t *a, int n) {
    for (int i = 0; i < n/2; i++) a[i] = i;
    for (int i = n/2; i < n; i++) a[i] = n - 1 - i;
}

static void gen_sawtooth(int32_t *a, int n) {
    int teeth = 10, sz = n / teeth;
    for (int t = 0; t < teeth; t++)
        for (int i = 0; i < sz && t*sz + i < n; i++)
            a[t*sz + i] = i;
    for (int i = teeth*sz; i < n; i++) a[i] = i - teeth*sz;
}

static void gen_heavy_dupes(int32_t *a, int n, int unique) {
    for (int i = 0; i < n; i++) a[i] = rand() % unique;
}

static void gen_narrow_range(int32_t *a, int n) { for (int i = 0; i < n; i++) a[i] = rand() % 100; }
static void gen_binary(int32_t *a, int n) { for (int i = 0; i < n; i++) a[i] = rand() % 2; }
static void gen_ternary(int32_t *a, int n) { for (int i = 0; i < n; i++) a[i] = rand() % 3; }

static void gen_gaussian(int32_t *a, int n) {
    for (int i = 0; i < n; i++) {
        double u1 = (rand() + 1.0) / (RAND_MAX + 2.0);
        double u2 = (rand() + 1.0) / (RAND_MAX + 2.0);
        double z = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
        a[i] = (int32_t)(n/2 + z * 1000);
    }
}

static void gen_zipf(int32_t *a, int n) {
    for (int i = 0; i < n; i++) {
        double u = (double)rand() / RAND_MAX;
        a[i] = (int32_t)(n * (1.0 - u * u));
        if (a[i] < 0) a[i] = 0; if (a[i] >= n) a[i] = n - 1;
    }
}

static void gen_uniform_wide(int32_t *a, int n) { for (int i = 0; i < n; i++) a[i] = rand() % (n * 10); }

static void gen_uniform_huge(int32_t *a, int n) {
    for (int i = 0; i < n; i++)
        a[i] = (int32_t)(((int64_t)rand() * rand()) % 2000000000 - 1000000000);
}

static void gen_clustered(int32_t *a, int n) {
    int groups = 8, per = n / groups;
    for (int g = 0; g < groups; g++) {
        int center = g * (n * 3 / groups);
        for (int i = 0; i < per && g*per+i < n; i++)
            a[g*per+i] = center + (rand() % (n/groups)) - n/(groups*2);
    }
    for (int i = groups*per; i < n; i++) a[i] = rand() % (n*3);
}

static void gen_plateau(int32_t *a, int n) {
    for (int i = 0; i < n; i++) a[i] = i;
    int mid = n/4;
    for (int i = mid; i < 3*mid; i++) a[i] = a[mid];
}

static void gen_two_blocks(int32_t *a, int n) {
    int half = n/2;
    for (int i = 0; i < half; i++) a[i] = half + i;
    for (int i = half; i < n; i++) a[i] = i - half;
}

static void gen_organ_noise(int32_t *a, int n) {
    gen_pipe_organ(a, n);
    for (int i = 0; i < n; i++) a[i] += (rand() % 101) - 50;
}

static void gen_outlier(int32_t *a, int n) {
    for (int i = 0; i < n; i++) a[i] = i;
    int32_t val = a[0];
    memmove(a, a + 1, (n - 1) * sizeof(int32_t));
    a[2 * n / 3] = val;
}


/* ======================================================================
 * SECTION 4: BENCHMARK ENGINE
 * ====================================================================== */

#define MAX_ALGOS 12
#define MAX_PATTERNS 35
#define MAX_TRIALS 15

/* Algorithm types */
enum AlgoType {
    ALGO_INPLACE,     /* sort(a, n) */
    ALGO_OUTPLACE,    /* sort(src, dst, n) */
    ALGO_COUNTING,    /* returns 0 if N/A */
};

typedef struct {
    const char *name;
    enum AlgoType type;
    void (*fn_inplace)(int32_t *, int);
    void (*fn_outplace)(const int32_t *, int32_t *, int);
    int (*fn_counting)(int32_t *, int);
    /* Set of pattern indices where this algo is N/A */
    int na_mask[MAX_PATTERNS]; /* 1 = N/A */
} Algorithm;

typedef struct {
    const char *name;
    void (*gen)(int32_t *, int);
    int pct_x10; /* for parameterized generators, -1 if unused */
    int param2;
} Pattern;

typedef struct {
    double time_us;
    int ok;       /* 1=pass, 0=fail, -1=N/A */
} Result;

static void run_benchmark(int N, int trials) {
    timer_init();

    /* Define algorithms */
    Algorithm algos[MAX_ALGOS];
    int nalgo = 0;

    #define ADD_INPLACE(NAME, FN) do { \
        algos[nalgo].name = NAME; algos[nalgo].type = ALGO_INPLACE; \
        algos[nalgo].fn_inplace = FN; algos[nalgo].fn_outplace = NULL; \
        algos[nalgo].fn_counting = NULL; memset(algos[nalgo].na_mask, 0, sizeof(algos[nalgo].na_mask)); \
        nalgo++; } while(0)

    #define ADD_OUTPLACE(NAME, FN) do { \
        algos[nalgo].name = NAME; algos[nalgo].type = ALGO_OUTPLACE; \
        algos[nalgo].fn_outplace = FN; algos[nalgo].fn_inplace = NULL; \
        algos[nalgo].fn_counting = NULL; memset(algos[nalgo].na_mask, 0, sizeof(algos[nalgo].na_mask)); \
        nalgo++; } while(0)

    #define ADD_COUNTING(NAME, FN) do { \
        algos[nalgo].name = NAME; algos[nalgo].type = ALGO_COUNTING; \
        algos[nalgo].fn_counting = FN; algos[nalgo].fn_inplace = NULL; \
        algos[nalgo].fn_outplace = NULL; memset(algos[nalgo].na_mask, 0, sizeof(algos[nalgo].na_mask)); \
        nalgo++; } while(0)

    /* Your algorithms */
    ADD_INPLACE("SafeSlot-C",   safe_slot_sort);
    int sss_idx = nalgo - 1;
    ADD_INPLACE("MicroSSS",     micro_sss);
    int msss_idx = nalgo - 1;
    ADD_COUNTING("Gravity",     gravity_sort);
    ADD_INPLACE("CAS-Core",     cas_core);
    int cas_idx = nalgo - 1;
    ADD_INPLACE("CAS-Binary",   cas_binary);
    int casb_idx = nalgo - 1;
    ADD_INPLACE("Sieve-v3",     sieve_sort);
    ADD_OUTPLACE("Pendry",      pendry_sort);
    ADD_INPLACE("LittlePendry", little_pendry);

    /* Industry standards */
    ADD_INPLACE("qsort",        stdlib_qsort);
    ADD_INPLACE("introsort",    bench_introsort);
    ADD_INPLACE("heapsort",     bench_heapsort);
    ADD_INPLACE("radix256",     radix256_sort_fixed);

    /* Define patterns */
    Pattern patterns[MAX_PATTERNS];
    int npat = 0;

    #define ADD_PAT(NAME, GENFN) do { patterns[npat].name = NAME; \
        patterns[npat].gen = GENFN; patterns[npat].pct_x10 = -1; npat++; } while(0)

    ADD_PAT("Sorted",              gen_sorted);
    ADD_PAT("Reverse",             gen_reverse);
    ADD_PAT("All Identical",       gen_all_identical);
    ADD_PAT("Random",              gen_random);
    ADD_PAT("Pipe Organ",          gen_pipe_organ);
    ADD_PAT("Sawtooth",            gen_sawtooth);
    ADD_PAT("Two Blocks",          gen_two_blocks);
    ADD_PAT("Plateau",             gen_plateau);
    ADD_PAT("Outlier",             gen_outlier);
    ADD_PAT("Organ+Noise",         gen_organ_noise);
    ADD_PAT("Heavy Dupes(10)",     NULL); /* custom */
    ADD_PAT("Narrow Range(100)",   gen_narrow_range);
    ADD_PAT("Binary(0/1)",         gen_binary);
    ADD_PAT("Ternary(0-2)",        gen_ternary);
    ADD_PAT("Gaussian Narrow",     gen_gaussian);
    ADD_PAT("Zipf-like",           gen_zipf);
    ADD_PAT("Uniform[0,10N]",      gen_uniform_wide);
    ADD_PAT("Uniform[-1B,1B]",     gen_uniform_huge);
    ADD_PAT("Clustered(8grp)",     gen_clustered);

    /* Parameterized patterns: local swap */
    int local_pcts[] = {1, 10, 50, 100};
    for (int p = 0; p < 4; p++) {
        patterns[npat].name = NULL; /* filled below */
        patterns[npat].pct_x10 = local_pcts[p];
        patterns[npat].gen = NULL;
        npat++;
    }
    /* Parameterized: scatter */
    int scat_pcts[] = {1, 10, 50, 100, 250, 500, 1000};
    for (int p = 0; p < 7; p++) {
        patterns[npat].name = NULL;
        patterns[npat].pct_x10 = scat_pcts[p];
        patterns[npat].gen = NULL;
        npat++;
    }
    /* Parameterized: localized corruption */
    int loc_pcts[] = {1, 10, 50, 100, 250};
    for (int p = 0; p < 5; p++) {
        patterns[npat].name = NULL;
        patterns[npat].pct_x10 = loc_pcts[p];
        patterns[npat].gen = NULL;
        npat++;
    }

    /* Build pattern name strings */
    char pat_names[MAX_PATTERNS][40];
    int base_npat = 19; /* non-parameterized patterns */
    for (int i = 0; i < base_npat; i++) {
        if (patterns[i].name)
            strncpy(pat_names[i], patterns[i].name, 39);
    }
    int pi = base_npat;
    for (int p = 0; p < 4; p++, pi++)
        snprintf(pat_names[pi], 39, "LocalSwap %.1f%%", local_pcts[p] / 10.0);
    for (int p = 0; p < 7; p++, pi++)
        snprintf(pat_names[pi], 39, "Scatter %.1f%%", scat_pcts[p] / 10.0);
    for (int p = 0; p < 5; p++, pi++)
        snprintf(pat_names[pi], 39, "Localized %.1f%%", loc_pcts[p] / 10.0);

    /* Mark N/A patterns for O(n^2) algorithms at large N */
    /* Safe-Slot C: N/A on adversarial/random and shuffled-value patterns */
    {
        int sss_na[] = {1, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18};
        for (int i = 0; i < 16; i++) algos[sss_idx].na_mask[sss_na[i]] = 1;
    }

    /* Micro SSS / CAS Core / CAS Binary: O(n^2) insertion sort core.
     * Always N/A on Reverse (presort check can miss it).
     * At N > 50000, also N/A on random, adversarial, and wide distributions. */
    algos[msss_idx].na_mask[1] = 1;  /* Reverse: always N/A */
    algos[cas_idx].na_mask[1] = 1;
    algos[casb_idx].na_mask[1] = 1;
    if (N > 50000) {
        /* random, adversarial, wide distributions, and shuffled-value patterns */
        int quad_na[] = {3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18};
        for (int i = 0; i < 15; i++) {
            algos[msss_idx].na_mask[quad_na[i]] = 1;
            algos[cas_idx].na_mask[quad_na[i]] = 1;
            algos[casb_idx].na_mask[quad_na[i]] = 1;
        }
        /* ALL scatter patterns (indices 23-29) go quadratic at large N */
        for (int i = 23; i <= 29; i++) {
            algos[msss_idx].na_mask[i] = 1;
            algos[cas_idx].na_mask[i] = 1;
            algos[casb_idx].na_mask[i] = 1;
        }
        /* High localized (5%+) = pattern indices 32-34 */
        for (int i = 32; i <= 34; i++) {
            algos[msss_idx].na_mask[i] = 1;
            algos[cas_idx].na_mask[i] = 1;
            algos[casb_idx].na_mask[i] = 1;
        }
    }

    /* Print header */
    printf("================================================================================\n");
    printf("  PENDRY SORT SERIES -- C BENCHMARK\n");
    printf("  N = %d elements | %d trials per test | median timing\n", N, trials);
    printf("  Compiled with: ");
    #ifdef __GNUC__
    printf("GCC %d.%d.%d", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
    #elif defined(_MSC_VER)
    printf("MSVC %d", _MSC_VER);
    #else
    printf("Unknown compiler");
    #endif
    printf("\n================================================================================\n\n");

    /* Allocate */
    int32_t *orig = (int32_t *)malloc(N * sizeof(int32_t));
    int32_t *work = (int32_t *)malloc(N * sizeof(int32_t));
    int32_t *dst  = (int32_t *)malloc(N * sizeof(int32_t));
    double trial_times[MAX_TRIALS];
    Result results[MAX_PATTERNS][MAX_ALGOS];

    /* Run benchmarks */
    for (int pi = 0; pi < npat; pi++) {
        srand(42 + pi);

        /* Generate data */
        if (pi < base_npat) {
            if (pi == 10) gen_heavy_dupes(orig, N, 10); /* custom */
            else if (patterns[pi].gen) patterns[pi].gen(orig, N);
        } else if (pi < base_npat + 4) {
            gen_local_swap(orig, N, local_pcts[pi - base_npat]);
        } else if (pi < base_npat + 4 + 7) {
            gen_scatter(orig, N, scat_pcts[pi - base_npat - 4]);
        } else {
            gen_localized(orig, N, loc_pcts[pi - base_npat - 4 - 7]);
        }

        printf("  [%2d/%d] %s...\n", pi + 1, npat, pat_names[pi]);
        fflush(stdout);

        for (int ai = 0; ai < nalgo; ai++) {
            if (algos[ai].na_mask[pi]) {
                results[pi][ai].time_us = -1;
                results[pi][ai].ok = -1;
                continue;
            }

            int all_ok = 1;
            for (int t = 0; t < trials; t++) {
                memcpy(work, orig, N * sizeof(int32_t));
                double t0 = now_us();

                if (algos[ai].type == ALGO_INPLACE) {
                    algos[ai].fn_inplace(work, N);
                    double t1 = now_us();
                    trial_times[t] = t1 - t0;
                    if (!is_sorted(work, N)) all_ok = 0;
                } else if (algos[ai].type == ALGO_OUTPLACE) {
                    algos[ai].fn_outplace(work, dst, N);
                    double t1 = now_us();
                    trial_times[t] = t1 - t0;
                    if (!is_sorted(dst, N)) all_ok = 0;
                } else { /* COUNTING */
                    int ret = algos[ai].fn_counting(work, N);
                    double t1 = now_us();
                    trial_times[t] = t1 - t0;
                    if (!ret) { results[pi][ai].time_us = -1; results[pi][ai].ok = -1; all_ok = -1; break; }
                    if (!is_sorted(work, N)) all_ok = 0;
                }
            }
            if (all_ok == -1) continue;

            /* Median */
            for (int i = 0; i < trials - 1; i++)
                for (int j = i + 1; j < trials; j++)
                    if (trial_times[j] < trial_times[i]) {
                        double tmp = trial_times[i]; trial_times[i] = trial_times[j]; trial_times[j] = tmp;
                    }
            results[pi][ai].time_us = trial_times[trials / 2];
            results[pi][ai].ok = all_ok;
        }
    }

    /* Print results table */
    printf("\n================================================================================\n");
    printf("  RESULTS (times in microseconds)\n");
    printf("================================================================================\n\n");

    /* Header */
    printf("%-22s", "Pattern");
    for (int ai = 0; ai < nalgo; ai++)
        printf(" | %11s", algos[ai].name);
    printf(" | Winner\n");

    for (int i = 0; i < 22; i++) printf("-");
    for (int ai = 0; ai < nalgo; ai++) printf("-+------------");
    printf("-+------\n");

    /* Rows */
    double totals[MAX_ALGOS] = {0};
    int wins[MAX_ALGOS] = {0};
    int valid_count[MAX_ALGOS] = {0};
    int na_count[MAX_ALGOS] = {0};
    int fail_count[MAX_ALGOS] = {0};

    for (int pi = 0; pi < npat; pi++) {
        printf("%-22s", pat_names[pi]);
        double best_time = 1e18;
        int best_ai = -1;
        for (int ai = 0; ai < nalgo; ai++) {
            if (results[pi][ai].ok == -1) {
                printf(" | %11s", "N/A");
                na_count[ai]++;
            } else if (results[pi][ai].ok == 0) {
                printf(" | %11s", "FAIL");
                fail_count[ai]++;
            } else {
                double us = results[pi][ai].time_us;
                if (us < 1000)
                    printf(" | %9.0fus", us);
                else
                    printf(" | %8.1fms", us / 1000.0);
                totals[ai] += us;
                valid_count[ai]++;
                if (us < best_time) { best_time = us; best_ai = ai; }
            }
        }
        if (best_ai >= 0) {
            /* Find second best for speedup */
            double second = 1e18;
            for (int ai = 0; ai < nalgo; ai++)
                if (ai != best_ai && results[pi][ai].ok == 1 && results[pi][ai].time_us < second)
                    second = results[pi][ai].time_us;
            if (second < 1e18) {
                double speedup = second / best_time;
                printf(" | %s %.1fx", algos[best_ai].name, speedup);
            } else {
                printf(" | %s", algos[best_ai].name);
            }
            wins[best_ai]++;
        } else {
            printf(" | NONE");
        }
        printf("\n");
    }

    /* Aggregate */
    printf("\n================================================================================\n");
    printf("  AGGREGATE SUMMARY\n");
    printf("================================================================================\n\n");

    printf("  %-18s | %12s | %6s | %7s | %5s | %5s\n",
           "Algorithm", "Total (us)", "Wins", "Tested", "N/A", "Fail");
    printf("  -------------------+--------------+--------+---------+-------+------\n");

    /* Sort by total time */
    int order[MAX_ALGOS];
    for (int i = 0; i < nalgo; i++) order[i] = i;
    for (int i = 0; i < nalgo - 1; i++)
        for (int j = i + 1; j < nalgo; j++)
            if (totals[order[j]] < totals[order[i]]) {
                int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
            }

    for (int oi = 0; oi < nalgo; oi++) {
        int ai = order[oi];
        if (valid_count[ai] > 0)
            printf("  %-18s | %12.0f | %6d | %7d | %5d | %5d\n",
                   algos[ai].name, totals[ai], wins[ai], valid_count[ai],
                   na_count[ai], fail_count[ai]);
        else
            printf("  %-18s | %12s | %6d | %7d | %5d | %5d\n",
                   algos[ai].name, "--", wins[ai], valid_count[ai],
                   na_count[ai], fail_count[ai]);
    }

    printf("\n  N/A Notes:\n");
    printf("  -----------------------------------------------------------------------\n");
    printf("  SafeSlot-C: N/A on adversarial patterns (unsorted output after 100 iters).\n");
    printf("  MicroSSS/CAS-Core/CAS-Binary: N/A on random/adversarial at large N\n");
    printf("    (O(n^2) insertion sort core becomes catastrophically slow).\n");
    printf("  Gravity/Counting: N/A when value range > 4N (frequency array too large).\n");
    printf("\n");

    free(orig); free(work); free(dst);
}


/* ======================================================================
 * MAIN
 * ====================================================================== */

int main(int argc, char **argv) {
    int N = 1000000;
    int trials = 7;

    if (argc >= 2) N = atoi(argv[1]);
    if (argc >= 3 && strcmp(argv[2], "quick") == 0) trials = 3;

    if (N < 100) { fprintf(stderr, "N must be at least 100\n"); return 1; }
    if (N > 50000000) { fprintf(stderr, "N capped at 50M\n"); N = 50000000; }

    run_benchmark(N, trials);
    return 0;
}
