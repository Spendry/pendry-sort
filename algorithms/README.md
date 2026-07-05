# The algorithms, standalone

Each file is a presentation copy extracted verbatim from [../benchmarks/pendry_benchmark.c](../benchmarks/pendry_benchmark.c), so the algorithm can be read on its own without the harness around it. The benchmark remains the runnable source of truth for every number. Each header states the algorithm's full benchmark standing, including its failures.

| File | What it is | Standing (35 patterns, 1M int32) |
|---|---|---|
| [safe_slot_sort.c](safe_slot_sort.c) | Windowed inversion repair, where the series started | 19/35 tested, 16 N/A |
| [micro_sss.c](micro_sss.c) | The tenet sort: insertion sort + reverse detection | 9/35 tested, 26 N/A |
| [gravity_sort.c](gravity_sort.c) | Counting sort, rediscovered and named as such | #1 by total time, 34/35, 10 wins |
| [cas_core.c](cas_core.c) | Consequence-as-Action Sort; this IS insertion sort | 9/35 tested, 7 wins |
| [cas_binary.c](cas_binary.c) | The cascade replaced by binary search + memmove | 9/35 tested, lowest per-pattern cost |
| [sieve_sort.c](sieve_sort.c) | Window detector + three-path routing, stable | #6, 35/35, 0 N/A, 0 FAIL |
| [pendry_sort.c](pendry_sort.c) | The capstone: scan-route-repair, counting, scatter | #2, 35/35, most wins (11), 0 FAIL |
| [little_pendry.c](little_pendry.c) | The 87-line in-place compression | 33/35, 2 FAILs |

The coverage numbers are the honest part of the table: five of these eight only work on structured data, and the paper explains why each limitation exists and what it taught.
