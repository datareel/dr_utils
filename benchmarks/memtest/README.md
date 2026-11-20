# memtest

Part of dr_utils 1.5. This file is made from [docs/benchmarks.md](../../docs/benchmarks.md#memtest) by docs/make_docs.py: edit that page, not this file. All the documentation: [docs/](../../docs/README.md), or the searchable HTML in `docs/html/index.html`.

Sizes take decimal suffixes (`1k` = 1000, `1m`, `1g`). The dashboard options (`--drsm`, `--submit`, `--now`, `--drra`, `--drra-config`) are on the [Dashboard](../../docs/dashboard-hooks.md) page.

Memory speed: each thread builds its own binary search tree of "Item n" string keys and times the insert, find, delete and insert-again phases.

```
memtest [--num-threads=NUM] [--num-inserts=NUM] [--datfile=fname] [dashboard options]
```

| Option | Meaning |
|---|---|
| `--num-threads=NUM` | threads (default 1, at most 1024) |
| `--num-inserts=NUM` | keys per thread: `1k`, `25k`, `100k` (default `25k`, at most `1m`) |
| `--datfile=fname` | write a plot data file |

The keys arrive nearly sorted, so the tree is deep and the time grows faster than the key count: compare runs with the same `--num-inserts`.

```
Threads: 1, 5000 key insertions per thread
Thread[0]: Inserted 5000 values in 0.013 seconds
Thread[0]: Found 5000 values in 0.011 seconds
Thread[0]: Deleted 5000 values in 0.007 seconds
Thread[0]: Reinserted 5000 values in 0.012 seconds
Average thread times: insert 0.013, find 0.011, delete 0.007, reinsert 0.012 seconds
Total elapsed time 0.044 seconds
```

Linux and Windows (the same command lines):

```
memtest
memtest --num-threads=4 --num-inserts=50k --datfile=mem.dat
```
