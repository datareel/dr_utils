# flops

Part of dr_utils 1.5. This file is made from [docs/benchmarks.md](../../docs/benchmarks.md#flops) by docs/make_docs.py: edit that page, not this file. All the documentation: [docs/](../../docs/README.md), or the searchable HTML in `docs/html/index.html`.

Sizes take decimal suffixes (`1k` = 1000, `1m`, `1g`). The dashboard options (`--drsm`, `--submit`, `--now`, `--drra`, `--drra-config`) are on the [Dashboard](../../docs/dashboard-hooks.md) page.

Floating point operations per second: multiply-add chains over a small array that stays in the CPU cache.

```
flops [--num-threads=NUM] [--dp] [--div] [--iterations=NUM] [--datfile=fname] [dashboard options]
```

| Option | Meaning |
|---|---|
| `--num-threads=NUM` | threads (default 1, at most 1024); the iterations are split between them |
| `--dp` | double precision (the default is single precision) |
| `--div` | the formula of the old c/flops program, `a*c + a/b`: a multiply, a divide and an add, counted as 3 operations. Code with divisions runs slower than multiply-add, so this gives the rate such code gets |
| `--iterations=NUM` | loop iterations in all: `1m`, `1g`, `10g` (default `1g`) |
| `--datfile=fname` | write a plot data file |

Output: each thread's time and the total rate in MFLOPS or GFLOPS. The rate depends on the compiler and its flags (vector units).

Linux:

```
flops
flops --num-threads=$(nproc) --dp --iterations=10g
flops --div --dp --datfile=flops_div.dat
```

Windows:

```
flops --num-threads=%NUMBER_OF_PROCESSORS% --dp --iterations=10g
flops --div --datfile=flops_div.dat
```
