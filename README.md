# Pendry Sort

> Part of my [portfolio](https://github.com/Spendry/portfolio).

Twelve sorting algorithms from first principles, developed and benchmarked in the open. The headline: an adaptive integer sort that completes a 35-pattern benchmark at 1M elements **4.6x faster than introsort in aggregate**, with the most pattern wins of any algorithm tested, built through guided AI development by someone who cannot read C.

**Read the paper: [paper.md](paper.md)**

![Benchmark coverage by algorithm](assets/benchmark-coverage.svg)

## What's here

- [paper.md](paper.md): the consolidated paper. One observation about disorder, five axioms, three algorithm lineages, three rediscoveries named honestly, and the full family on one benchmark table.
- [algorithms/](algorithms/): each algorithm as a standalone file, extracted verbatim from the benchmark so it can be read without the harness. Every header states that algorithm's benchmark standing, failures included.
- [benchmarks/pendry_benchmark.c](benchmarks/pendry_benchmark.c): the C suite. All twelve algorithms, all 35 data patterns, correctness verified on every trial.
- [benchmarks/pendry_benchmark.py](benchmarks/pendry_benchmark.py): the Python suite from the series' first era.
- [assets/](assets/): the figures.

## Reproduce it

```
gcc -O2 -o pendry_bench benchmarks/pendry_benchmark.c -lm
./pendry_bench
```

The timings in the paper come from one machine; the ratios are the claims. Run it on yours and check.

## License

Text CC BY 4.0 · Code Apache 2.0
