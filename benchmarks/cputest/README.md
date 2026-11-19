# cputest

Part of dr_utils 1.4. This file is made from [docs/benchmarks.md](../../docs/benchmarks.md#cputest) by docs/make_docs.py: edit that page, not this file. All the documentation: [docs/](../../docs/README.md), or the searchable HTML in `docs/html/index.html`.

Sizes take decimal suffixes (`1k` = 1000, `1m`, `1g`). The dashboard options (`--drsm`, `--submit`, `--now`, `--drra`, `--drra-config`) are on the [Dashboard](../../docs/dashboard-hooks.md) page.

CPU speed: zlib best compression (level 9) and uncompression of random data, checked after each pass.

```
cputest [--num-threads=NUM] [--buf-size=SIZE] [--repeat=NUM] [--datfile=fname] [dashboard options]
```

| Option | Meaning |
|---|---|
| `--num-threads=NUM` | threads (default 1, at most 1024) |
| `--buf-size=SIZE` | the buffer of random bytes, split between the threads: `24k`, `240m`, `2.4g` (default `100m`) |
| `--repeat=NUM` | passes over the buffer (default 1) |
| `--datfile=fname` | write a plot data file |

Output: each thread's time, the average, and the total time with the rate in MB/s.

```
Threads: 2, buffer 10M bytes, 1 pass(es)
Thread[1]: CPU speed test elapsed time 0.229 seconds
Thread[0]: CPU speed test elapsed time 0.244 seconds
Average thread time 0.237 seconds
Total elapsed time 0.245 seconds, 40.88 MB/s
```

Run it with one thread for single-core speed and one thread per core for the whole CPU.

Linux:

```
cputest
cputest --num-threads=$(nproc) --buf-size=1g --datfile=cpu_$(hostname -s).dat
cputest --num-threads=1 --repeat=5
```

Windows:

```
cputest
cputest --num-threads=%NUMBER_OF_PROCESSORS% --buf-size=1g --datfile=cpu_%COMPUTERNAME%.dat
```

PowerShell: `cputest --num-threads=$env:NUMBER_OF_PROCESSORS --buf-size=1g`
