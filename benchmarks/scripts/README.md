# Watch scripts

Part of dr_utils 1.4. This file is made from [docs/plots-and-reports.md](../../docs/plots-and-reports.md#watch-scripts-linux) by docs/make_docs.py: edit that page, not this file. All the documentation: [docs/](../../docs/README.md), or the searchable HTML in `docs/html/index.html`.

In `benchmarks/scripts/`. Run them in a second terminal (or in the background) while a benchmark runs. All three take `-i seconds` (default 1), `-n count` (default 0: until Ctrl-C) and `STATSONLY`.

| Script | What it prints |
|---|---|
| `watch_system.sh [-i sec] [-n count] [STATSONLY]` | the release, kernel, CPU, memory, partitions, mounts and disk statistics from /proc, each between `START name` and `END name` lines, then `top -b` until Ctrl-C, and the statistics again at the end. `STATSONLY`: the statistics once, no top |
| `watch_mem.sh [-i sec] [-n count] [STATSONLY]` | `free` and /proc/meminfo every interval, and once more at the end. `STATSONLY`: /proc/meminfo once |
| `watch_mem_usage.sh [-i sec] [-n count] [STATSONLY]` | one `YYYY-MM-DD HH:MM:SS used-KB` line per interval, for `dr_plot.py` and `dr_report.py`. `STATSONLY`: one line |

```
benchmarks/scripts/watch_system.sh STATSONLY > host1_system.txt
benchmarks/scripts/watch_system.sh -i 5 > host1_during_disktest.txt &
disktest --file=/data/testfile.btx --datfile=results/disk.dat
kill %1

benchmarks/scripts/watch_mem_usage.sh -i 2 > results/mem.log &
memtest --num-threads=8 --num-inserts=100k --datfile=results/mem.dat
kill %1
dr_report.py -o mem_report.html results
```

On Windows use Performance Monitor or `typeperf "\Memory\Committed Bytes" -si 2 -o mem.csv` for the same kind of record (the scripts read /proc, so they are Linux only).
