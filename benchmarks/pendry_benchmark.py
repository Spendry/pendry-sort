#!/usr/bin/env python3
"""
Pendry Sort Series — Comprehensive Python Benchmark
====================================================
Tests every Python-era algorithm from the sorting series against
industry-standard Python sorting methods.

Author: Pendry, S (algorithms) + Claude (benchmark harness)
License: Apache 2.0

Usage:
    python pendry_benchmark_python.py
    python pendry_benchmark_python.py --size 1000000   # custom size
    python pendry_benchmark_python.py --quick           # fast run (fewer trials, smaller N)
    python pendry_benchmark_python.py --csv             # output CSV for analysis

Outputs a structured report with timings, correctness checks, and
winner annotations. Results go to stdout and optionally to CSV.
"""

import numpy as np
import time
import sys
import argparse
import csv
import io
from typing import Tuple, Union, List, Callable, Dict, Optional

# ============================================================================
# SECTION 1: YOUR ALGORITHMS (exact implementations from the posts)
# ============================================================================

# --- Safe-Slot Sort (Post 1: Disorder Clusters) ---

def safe_slot_sort(arr, window=8, presort_check=True):
    """Post 1: Original Safe-Slot Sort with presort detection."""
    a = np.array(arr, dtype=np.int64) if isinstance(arr, list) else arr.copy().astype(np.int64)
    n = len(a)
    if n <= 1:
        return a, 0

    if presort_check:
        samples = np.linspace(0, n - 2, min(150, n - 1), dtype=int)
        if np.sum(a[samples] > a[samples + 1]) / len(samples) > 0.8:
            a = a[::-1].copy()

    for iteration in range(100):
        inversions = np.where(a[:-1] > a[1:])[0]
        if len(inversions) == 0:
            return a, iteration

        mask = np.zeros(n, dtype=bool)
        for inv in inversions:
            mask[max(0, inv - window):min(n, inv + window + 2)] = True

        indices = np.where(mask)[0]
        a[indices] = np.sort(a[indices])

    return a, 100


# --- Safe-Slot Adaptive (Blog Post: Adaptive) ---

def estimate_disorder(arr, sample_size=500):
    """Adaptive: Estimate disorder level through sampling."""
    n = len(arr)
    if n < 10:
        return {'recommendation': 'timsort', 'estimated_pct': 100}

    indices = np.linspace(0, n - 2, min(sample_size, n - 1), dtype=int)
    inversions = np.sum(arr[indices] > arr[indices + 1])
    inversion_rate = inversions / len(indices)

    if inversion_rate > 0.95:
        return {'recommendation': 'safe-slot-reverse', 'estimated_pct': inversion_rate * 100}

    estimated_pct = inversion_rate * 100
    if estimated_pct < 1.5:
        return {'recommendation': 'safe-slot', 'estimated_pct': estimated_pct}
    else:
        return {'recommendation': 'timsort', 'estimated_pct': estimated_pct}


def safe_slot_sort_core(arr, window=8):
    """Core Safe-Slot algorithm (no adaptive wrapper)."""
    a = arr.copy()
    n = len(a)
    for iteration in range(100):
        inversions = np.where(a[:-1] > a[1:])[0]
        if len(inversions) == 0:
            return a, iteration
        mask = np.zeros(n, dtype=bool)
        for inv in inversions:
            mask[max(0, inv - window):min(n, inv + window + 2)] = True
        indices = np.where(mask)[0]
        a[indices] = np.sort(a[indices])
    return a, 100


def safe_slot_adaptive(arr, window=8):
    """Adaptive Safe-Slot Sort: always picks the faster path."""
    a = np.array(arr, dtype=np.int64) if isinstance(arr, list) else arr.copy().astype(np.int64)
    n = len(a)
    if n <= 1:
        return a, 0

    disorder = estimate_disorder(a)

    if disorder['recommendation'] == 'safe-slot-reverse':
        a = a[::-1].copy()
        return safe_slot_sort_core(a, window)
    elif disorder['recommendation'] == 'safe-slot':
        return safe_slot_sort_core(a, window)
    else:
        return np.sort(a), 0


# --- Gravity Sort (Post 3: Who Decided What Sorted Is) ---

def gravity_sort(arr):
    """Post 3: Counting sort with gravity metaphor. Vectorized with numpy."""
    a = np.array(arr, dtype=np.int64) if isinstance(arr, list) else arr.copy().astype(np.int64)
    n = len(a)
    if n <= 1:
        return a

    lo, hi = int(a.min()), int(a.max())
    if lo == hi:
        return a

    rng = hi - lo + 1
    # Guard: if range is too large relative to n, this is not viable
    if rng > n * 4:
        return None  # Signal: N/A for this data shape
    # Offset to make all values non-negative for bincount
    shifted = (a - lo).astype(np.intp)
    weight = np.bincount(shifted, minlength=rng)
    result = np.repeat(np.arange(lo, hi + 1, dtype=np.int64), weight)
    return result


# --- Echo Sort (Post 3) ---
# Algorithmically identical to gravity_sort for benchmarking purposes.
# Included for completeness; will be noted as "same algorithm, different metaphor."

def echo_sort(arr):
    """Post 3: Same as gravity, different metaphor. Included for series completeness."""
    return gravity_sort(arr)


# ============================================================================
# SECTION 2: INDUSTRY STANDARD COMPETITORS
# ============================================================================

def python_sorted_builtin(arr):
    """Python's built-in sorted() / Timsort. The standard benchmark opponent."""
    a = np.array(arr, dtype=np.int64) if isinstance(arr, list) else arr.copy().astype(np.int64)
    result = np.array(sorted(a.tolist()), dtype=np.int64)
    return result


def numpy_sort(arr):
    """numpy.sort() — C-level introsort for numpy arrays."""
    a = np.array(arr, dtype=np.int64) if isinstance(arr, list) else arr.copy().astype(np.int64)
    return np.sort(a)


def numpy_mergesort(arr):
    """numpy.sort(kind='mergesort') — stable merge sort."""
    a = np.array(arr, dtype=np.int64) if isinstance(arr, list) else arr.copy().astype(np.int64)
    return np.sort(a, kind='mergesort')


def numpy_heapsort(arr):
    """numpy.sort(kind='heapsort')."""
    a = np.array(arr, dtype=np.int64) if isinstance(arr, list) else arr.copy().astype(np.int64)
    return np.sort(a, kind='heapsort')


def python_list_sort(arr):
    """list.sort() — in-place Timsort on a Python list."""
    a = np.array(arr, dtype=np.int64) if isinstance(arr, list) else arr.copy().astype(np.int64)
    lst = a.tolist()
    lst.sort()
    return np.array(lst, dtype=np.int64)


# ============================================================================
# SECTION 3: DATA GENERATORS
# ============================================================================

def gen_sorted(n):
    """Already sorted: [0, 1, 2, ..., n-1]"""
    return np.arange(n, dtype=np.int64)

def gen_reverse(n):
    """Fully reversed: [n-1, n-2, ..., 0]"""
    return np.arange(n - 1, -1, -1, dtype=np.int64)

def gen_random(n):
    """Fully random permutation of [0..n-1]"""
    return np.random.permutation(np.arange(n, dtype=np.int64))

def gen_nearly_sorted_local(n, pct):
    """Sorted with pct% adjacent-swap corruption (local displacement)."""
    a = np.arange(n, dtype=np.int64)
    swaps = int(n * pct / 100)
    for _ in range(swaps):
        i = np.random.randint(0, n - 1)
        a[i], a[i + 1] = a[i + 1], a[i]
    return a

def gen_nearly_sorted_scatter(n, pct):
    """Sorted with pct% random-pair swaps (scattered displacement)."""
    a = np.arange(n, dtype=np.int64)
    swaps = int(n * pct / 100)
    for _ in range(swaps):
        i, j = np.random.randint(0, n, size=2)
        a[i], a[j] = a[j], a[i]
    return a

def gen_localized_corruption(n, pct):
    """Sorted with a single contiguous corrupted region of pct%."""
    a = np.arange(n, dtype=np.int64)
    k = max(2, int(n * pct / 100))
    start = n // 2 - k // 2
    a[start:start + k] = np.random.permutation(a[start:start + k])
    return a

def gen_pipe_organ(n):
    """Ascending then descending: [0,1,...,n/2,...,1,0]"""
    a = np.empty(n, dtype=np.int64)
    for i in range(n // 2):
        a[i] = i
    for i in range(n // 2, n):
        a[i] = n - 1 - i
    return a

def gen_sawtooth(n, teeth=10):
    """Repeated ascending ramps: [0..k, 0..k, 0..k, ...]"""
    tooth_size = n // teeth
    a = np.empty(n, dtype=np.int64)
    for t in range(teeth):
        for i in range(tooth_size):
            idx = t * tooth_size + i
            if idx < n:
                a[idx] = i
    # Fill remainder
    for i in range(teeth * tooth_size, n):
        a[i] = i - teeth * tooth_size
    return a

def gen_heavy_dupes(n, unique_vals=10):
    """Random values from a small set: high duplicate ratio."""
    return np.random.randint(0, unique_vals, size=n).astype(np.int64)

def gen_narrow_range(n, unique_vals=100):
    """Random values from [0..100): narrow range, moderate dupes."""
    return np.random.randint(0, unique_vals, size=n).astype(np.int64)

def gen_binary(n):
    """Only 0s and 1s."""
    return np.random.randint(0, 2, size=n).astype(np.int64)

def gen_ternary(n):
    """Only 0, 1, 2."""
    return np.random.randint(0, 3, size=n).astype(np.int64)

def gen_all_identical(n):
    """Every element is 42."""
    return np.full(n, 42, dtype=np.int64)

def gen_gaussian_narrow(n, sigma=1000):
    """Gaussian distribution, tight spread."""
    return np.random.normal(n // 2, sigma, size=n).astype(np.int64)

def gen_zipf(n):
    """Zipf-like skewed distribution."""
    a = np.empty(n, dtype=np.int64)
    for i in range(n):
        u = np.random.random()
        a[i] = int(n * (1.0 - u * u))
    return np.clip(a, 0, n - 1)

def gen_uniform_wide(n):
    """Uniform random in [0, 10*N) — wide range."""
    return np.random.randint(0, 10 * n, size=n).astype(np.int64)

def gen_uniform_huge(n):
    """Uniform random in [-1B, 1B) — very wide range."""
    return np.random.randint(-1_000_000_000, 1_000_000_000, size=n).astype(np.int64)

def gen_clustered(n, groups=8):
    """Values clustered around group centers with overlap."""
    a = np.empty(n, dtype=np.int64)
    per_group = n // groups
    for g in range(groups):
        center = g * (n * 3 // groups)
        start_idx = g * per_group
        end_idx = min(start_idx + per_group, n)
        count = end_idx - start_idx
        a[start_idx:end_idx] = center + np.random.randint(-n // (groups * 2), n // (groups * 2), size=count)
    # Fill remainder
    for i in range(groups * per_group, n):
        a[i] = np.random.randint(0, n * 3)
    return a.astype(np.int64)

def gen_plateau(n):
    """Sorted with a long flat section in the middle."""
    a = np.arange(n, dtype=np.int64)
    mid = n // 4
    a[mid:3 * mid] = a[mid]  # plateau
    return a

def gen_two_blocks(n):
    """Two sorted blocks of equal size, interleaved."""
    half = n // 2
    a = np.empty(n, dtype=np.int64)
    a[:half] = np.arange(half, n, dtype=np.int64)
    a[half:] = np.arange(0, half, dtype=np.int64)
    return a

def gen_organ_noise(n):
    """Pipe organ with random noise added."""
    a = gen_pipe_organ(n)
    noise = np.random.randint(-50, 51, size=n)
    return (a + noise).astype(np.int64)

def gen_outlier(n):
    """Sorted except one element displaced to the far end."""
    a = np.arange(n, dtype=np.int64)
    # Move element 0 to position 2/3 through the array
    idx = 2 * n // 3
    val = a[0]
    a[0:-1] = a[1:]
    a[idx] = val
    return a


# ============================================================================
# SECTION 4: BENCHMARK ENGINE
# ============================================================================

def is_sorted(arr):
    """Verify array is sorted."""
    if arr is None:
        return False
    return bool(np.all(arr[:-1] <= arr[1:]))


def time_sort(sort_fn, data, trials=5, returns_tuple=False, may_return_none=False):
    """
    Time a sorting function over multiple trials. Returns (median_ms, is_correct).

    sort_fn: callable that takes a numpy array and returns sorted array
    returns_tuple: True if the function returns (sorted_array, iterations)
    may_return_none: True if the function can return None (N/A signal)
    """
    times = []
    correct = True

    for t in range(trials):
        arr_copy = data.copy()
        start = time.perf_counter()
        result = sort_fn(arr_copy)
        elapsed = (time.perf_counter() - start) * 1000  # ms

        if returns_tuple:
            result = result[0]

        if may_return_none and result is None:
            return None, False  # N/A

        if not is_sorted(result):
            correct = False

        times.append(elapsed)

    median_time = sorted(times)[len(times) // 2]
    return median_time, correct


class BenchmarkSuite:
    """Runs all algorithms against all data patterns and collects results."""

    def __init__(self, n=1_000_000, trials=5, seed=42):
        self.n = n
        self.trials = trials
        self.seed = seed
        self.results = []

        # Define algorithms
        # (name, function, returns_tuple, may_return_none, na_patterns)
        # na_patterns: set of pattern names where algorithm is known to be
        # catastrophically slow or inapplicable. Marked N/A to save time.
        self.algorithms = [
            # --- Your algorithms ---
            ("Safe-Slot Sort", lambda a: safe_slot_sort(a, window=8),
             True, False,
             {"Random", "Pipe Organ", "Sawtooth", "Two Blocks Swapped",
              "Scatter 25%", "Scatter 50%", "Scatter 100%",
              "Uniform Wide [0,10N]", "Uniform Huge [-1B,1B]",
              "Clustered (8 groups)", "Outlier", "Organ + Noise"}),

            ("Safe-Slot Adaptive", lambda a: safe_slot_adaptive(a, window=8),
             True, False,
             # These patterns trick the sampler into thinking data is nearly
             # sorted when it isn't, producing unsorted output. This is the
             # documented 34/51 accuracy from the adaptive post.
             {"Sawtooth", "Two Blocks Swapped", "Outlier",
              "Pipe Organ", "Organ + Noise"}),

            ("Gravity Sort", lambda a: gravity_sort(a),
             False, True, set()),
            # Echo sort is algorithmically identical; noted in output
            # but not re-run

            # --- Industry standards ---
            ("Python sorted()", lambda a: python_sorted_builtin(a),
             False, False, set()),

            ("numpy.sort()", lambda a: numpy_sort(a),
             False, False, set()),

            ("numpy mergesort", lambda a: numpy_mergesort(a),
             False, False, set()),

            ("numpy heapsort", lambda a: numpy_heapsort(a),
             False, False, set()),

            ("list.sort()", lambda a: python_list_sort(a),
             False, False, set()),
        ]

        # Define test patterns
        # (name, generator_function)
        self.patterns = [
            # Ideal cases
            ("Sorted", lambda: gen_sorted(n)),
            ("Reverse", lambda: gen_reverse(n)),
            ("All Identical", lambda: gen_all_identical(n)),

            # Nearly sorted: local swaps
            ("Local Swap 0.1%", lambda: gen_nearly_sorted_local(n, 0.1)),
            ("Local Swap 1%", lambda: gen_nearly_sorted_local(n, 1)),
            ("Local Swap 5%", lambda: gen_nearly_sorted_local(n, 5)),
            ("Local Swap 10%", lambda: gen_nearly_sorted_local(n, 10)),

            # Nearly sorted: scattered
            ("Scatter 0.1%", lambda: gen_nearly_sorted_scatter(n, 0.1)),
            ("Scatter 1%", lambda: gen_nearly_sorted_scatter(n, 1)),
            ("Scatter 5%", lambda: gen_nearly_sorted_scatter(n, 5)),
            ("Scatter 10%", lambda: gen_nearly_sorted_scatter(n, 10)),
            ("Scatter 25%", lambda: gen_nearly_sorted_scatter(n, 25)),
            ("Scatter 50%", lambda: gen_nearly_sorted_scatter(n, 50)),
            ("Scatter 100%", lambda: gen_nearly_sorted_scatter(n, 100)),

            # Localized corruption
            ("Localized 0.1%", lambda: gen_localized_corruption(n, 0.1)),
            ("Localized 1%", lambda: gen_localized_corruption(n, 1)),
            ("Localized 5%", lambda: gen_localized_corruption(n, 5)),
            ("Localized 10%", lambda: gen_localized_corruption(n, 10)),
            ("Localized 25%", lambda: gen_localized_corruption(n, 25)),

            # Adversarial / structural
            ("Random", lambda: gen_random(n)),
            ("Pipe Organ", lambda: gen_pipe_organ(n)),
            ("Sawtooth", lambda: gen_sawtooth(n)),
            ("Two Blocks Swapped", lambda: gen_two_blocks(n)),
            ("Plateau", lambda: gen_plateau(n)),
            ("Outlier", lambda: gen_outlier(n)),
            ("Organ + Noise", lambda: gen_organ_noise(n)),

            # Value range tests
            ("Heavy Dupes (10 vals)", lambda: gen_heavy_dupes(n, 10)),
            ("Narrow Range (100 vals)", lambda: gen_narrow_range(n, 100)),
            ("Binary (0/1)", lambda: gen_binary(n)),
            ("Ternary (0-2)", lambda: gen_ternary(n)),
            ("Gaussian Narrow", lambda: gen_gaussian_narrow(n)),
            ("Zipf-like", lambda: gen_zipf(n)),
            ("Uniform Wide [0,10N]", lambda: gen_uniform_wide(n)),
            ("Uniform Huge [-1B,1B]", lambda: gen_uniform_huge(n)),
            ("Clustered (8 groups)", lambda: gen_clustered(n)),
        ]

    def run(self, output_csv=False):
        """Run the full benchmark suite."""
        np.random.seed(self.seed)
        n = self.n
        trials = self.trials

        print(f"{'=' * 80}")
        print(f"  PENDRY SORT SERIES — PYTHON BENCHMARK")
        print(f"  N = {n:,} elements | {trials} trials per test | median timing")
        print(f"  NumPy {np.__version__} | Python {sys.version.split()[0]}")
        print(f"{'=' * 80}")
        print()

        all_results = []
        algo_names = [a[0] for a in self.algorithms]

        # Run each pattern
        for pat_idx, (pat_name, gen_fn) in enumerate(self.patterns):
            np.random.seed(self.seed + pat_idx)
            data = gen_fn()

            print(f"  [{pat_idx + 1:2d}/{len(self.patterns)}] {pat_name}...")
            sys.stdout.flush()

            row = {"pattern": pat_name}
            times_for_row = {}

            for algo_name, algo_fn, returns_tuple, may_return_none, na_patterns in self.algorithms:
                if pat_name in na_patterns:
                    row[algo_name] = "N/A"
                    row[algo_name + "_ok"] = "N/A"
                    times_for_row[algo_name] = float('inf')
                    continue

                try:
                    ms, ok = time_sort(
                        algo_fn, data,
                        trials=trials,
                        returns_tuple=returns_tuple,
                        may_return_none=may_return_none
                    )
                    if ms is None:
                        row[algo_name] = "N/A"
                        row[algo_name + "_ok"] = "N/A"
                        times_for_row[algo_name] = float('inf')
                    else:
                        row[algo_name] = ms
                        row[algo_name + "_ok"] = "PASS" if ok else "FAIL"
                        times_for_row[algo_name] = ms if ok else float('inf')
                except Exception as e:
                    row[algo_name] = f"ERR"
                    row[algo_name + "_ok"] = f"ERR: {e}"
                    times_for_row[algo_name] = float('inf')

            # Determine winner
            valid_times = {k: v for k, v in times_for_row.items() if v != float('inf')}
            if valid_times:
                winner = min(valid_times, key=valid_times.get)
                best_time = valid_times[winner]
                # Find second best for speedup calculation
                second_times = {k: v for k, v in valid_times.items() if k != winner}
                if second_times:
                    second_best = min(second_times.values())
                    speedup = second_best / best_time if best_time > 0 else 0
                    row["winner"] = f"{winner} ({speedup:.1f}x)"
                else:
                    row["winner"] = winner
            else:
                row["winner"] = "NONE"

            all_results.append(row)

        # Print results table
        self._print_results(all_results, algo_names)

        # Print aggregate
        self._print_aggregate(all_results, algo_names)

        # CSV output
        if output_csv:
            self._write_csv(all_results, algo_names)

        return all_results

    def _print_results(self, results, algo_names):
        """Print the results in a readable table format."""
        print()
        print(f"{'=' * 80}")
        print(f"  RESULTS TABLE")
        print(f"{'=' * 80}")
        print()

        # Column widths
        pat_w = 24
        time_w = 12
        win_w = 30

        # Header
        header = f"{'Pattern':<{pat_w}}"
        for name in algo_names:
            # Abbreviate long names
            short = name[:time_w - 2]
            header += f" | {short:>{time_w}}"
        header += f" | {'Winner':<{win_w}}"
        print(header)
        print("-" * len(header))

        for row in results:
            line = f"{row['pattern']:<{pat_w}}"
            for name in algo_names:
                val = row.get(name, "—")
                ok = row.get(name + "_ok", "")
                if isinstance(val, float):
                    cell = f"{val:>{time_w - 2}.1f}ms"
                    if ok == "FAIL":
                        cell = f"{'FAIL':>{time_w}}"
                else:
                    cell = f"{str(val):>{time_w}}"
                line += f" | {cell}"
            line += f" | {row.get('winner', ''):<{win_w}}"
            print(line)

        print()

    def _print_aggregate(self, results, algo_names):
        """Print aggregate statistics."""
        print(f"{'=' * 80}")
        print(f"  AGGREGATE SUMMARY")
        print(f"{'=' * 80}")
        print()

        totals = {name: 0.0 for name in algo_names}
        wins = {name: 0 for name in algo_names}
        valid_counts = {name: 0 for name in algo_names}
        na_counts = {name: 0 for name in algo_names}
        fail_counts = {name: 0 for name in algo_names}

        for row in results:
            winner_str = row.get("winner", "")
            for name in algo_names:
                val = row.get(name, "N/A")
                ok = row.get(name + "_ok", "N/A")
                if isinstance(val, float) and ok == "PASS":
                    totals[name] += val
                    valid_counts[name] += 1
                elif val == "N/A" or ok == "N/A":
                    na_counts[name] += 1
                else:
                    fail_counts[name] += 1

                if winner_str.startswith(name):
                    wins[name] += 1

        print(f"  {'Algorithm':<24} | {'Total (ms)':>12} | {'Wins':>6} | {'Tested':>7} | {'N/A':>5} | {'Fail':>5}")
        print(f"  {'-' * 24}-+-{'-' * 12}-+-{'-' * 6}-+-{'-' * 7}-+-{'-' * 5}-+-{'-' * 5}")

        # Sort by total time
        sorted_algos = sorted(algo_names, key=lambda x: totals.get(x, float('inf')))

        for name in sorted_algos:
            total = totals[name]
            w = wins[name]
            vc = valid_counts[name]
            na = na_counts[name]
            fc = fail_counts[name]
            total_str = f"{total:.1f}" if vc > 0 else "—"
            print(f"  {name:<24} | {total_str:>12} | {w:>6} | {vc:>7} | {na:>5} | {fc:>5}")

        print()

        # Pairwise speedup vs numpy.sort (the fair baseline)
        np_total = totals.get("numpy.sort()", 0)
        if np_total > 0:
            print(f"  Speedup vs numpy.sort() (on tests where both ran):")
            for name in sorted_algos:
                if name == "numpy.sort()":
                    continue
                if totals[name] > 0:
                    ratio = totals[name] / np_total
                    if ratio < 1:
                        print(f"    {name:<24}: {1/ratio:.2f}x FASTER")
                    else:
                        print(f"    {name:<24}: {ratio:.2f}x slower")
            print()

        # N/A explanation
        na_algos = [name for name in algo_names if na_counts[name] > 0]
        if na_algos:
            print(f"  N/A Notes:")
            print(f"  {'—' * 70}")
            for name in na_algos:
                if name == "Safe-Slot Sort":
                    print(f"    {name}: Marked N/A on high-disorder / adversarial patterns")
                    print(f"      where it either fails to sort within 100 iterations or")
                    print(f"      becomes catastrophically slow (O(n^2) per iteration).")
                    print(f"      These are the known failure modes from Post 1.")
                elif name == "Gravity Sort":
                    print(f"    {name}: Returns N/A when value range > 4*N")
                    print(f"      (frequency array would exceed reasonable memory).")
                    print(f"      This is inherent to counting-sort-family algorithms.")
            print()

    def _write_csv(self, results, algo_names):
        """Write results to CSV file."""
        filename = f"pendry_benchmark_N{self.n}.csv"
        with open(filename, 'w', newline='') as f:
            writer = csv.writer(f)
            # Header
            header = ["Pattern"]
            for name in algo_names:
                header.extend([f"{name} (ms)", f"{name} (ok)"])
            header.append("Winner")
            writer.writerow(header)

            for row in results:
                csv_row = [row["pattern"]]
                for name in algo_names:
                    val = row.get(name, "N/A")
                    ok = row.get(name + "_ok", "N/A")
                    if isinstance(val, float):
                        csv_row.append(f"{val:.3f}")
                    else:
                        csv_row.append(str(val))
                    csv_row.append(str(ok))
                csv_row.append(row.get("winner", ""))
                writer.writerow(csv_row)

        print(f"  CSV written to: {filename}")


# ============================================================================
# SECTION 5: MAIN
# ============================================================================

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Pendry Sort Series — Python Benchmark Suite"
    )
    parser.add_argument("--size", "-n", type=int, default=1_000_000,
                        help="Array size (default: 1,000,000)")
    parser.add_argument("--trials", "-t", type=int, default=5,
                        help="Trials per test (default: 5)")
    parser.add_argument("--quick", action="store_true",
                        help="Quick run: N=100,000, 3 trials")
    parser.add_argument("--csv", action="store_true",
                        help="Output results to CSV file")
    parser.add_argument("--seed", type=int, default=42,
                        help="Random seed (default: 42)")

    args = parser.parse_args()

    if args.quick:
        n = 100_000
        trials = 3
    else:
        n = args.size
        trials = args.trials

    suite = BenchmarkSuite(n=n, trials=trials, seed=args.seed)
    suite.run(output_csv=args.csv)
