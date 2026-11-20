# disktest

Part of dr_utils 1.5. This file is made from [docs/benchmarks.md](../../docs/benchmarks.md#disktest) by docs/make_docs.py: edit that page, not this file. All the documentation: [docs/](../../docs/README.md), or the searchable HTML in `docs/html/index.html`.

Sizes take decimal suffixes (`1k` = 1000, `1m`, `1g`). The dashboard options (`--drsm`, `--submit`, `--now`, `--drra`, `--drra-config`) are on the [Dashboard](../../docs/dashboard-hooks.md) page.

Disk speed for database index work: a DataReel B-tree index file (order 128) gets NUM key inserts, finds, deletes (each delete checked) and inserts again, with the time at every 10%.

```
disktest [--num-inserts=NUM] [--file=fname] [--keep] [--datfile=fname] [dashboard options]
```

| Option | Meaning |
|---|---|
| `--num-inserts=NUM` | keys: `1k`, `1m`, `1g` (default `1m`) |
| `--file=fname` | the B-tree test file (default `testfile.btx` in the current folder). Put it on the disk you want to test. An existing file is never overwritten |
| `--keep` | keep the test file (by default it is removed at the end) |
| `--datfile=fname` | write the times in the disk benchmark plot format (`[DISKBENCHMARK]`, `[PLOTINFO]`, `[HOSTINFO]`) |

The output has a line at every 10% of each phase, the phase total with keys a second, and the B-tree statistics:

```
Reinserted 10000 keys: 0.072 seconds, 137998 keys a second

Root address =      54264
Number of entries = 10000
B-Tree order =      128
B-Tree height =     3
Disk speed test elapsed time 0.271 seconds
```

Linux:

```
disktest --file=/data/testfile.btx
disktest --num-inserts=10m --file=/mnt/nfs01/testfile.btx --datfile=disk_nfs01.dat
```

Windows:

```
disktest --file=D:\bench\testfile.btx
disktest --num-inserts=10m --file=\\fileserver.example.com\share\testfile.btx --datfile=disk_share.dat
```
