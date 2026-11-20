# Plots, reports and watch scripts

## Plot data files (`--datfile`)

`cputest`, `disktest`, `memtest`, `flops`, `tcptest`, `udptest` and `mctest` take `--datfile=fname` and write their results as a small INI file: the settings, the main result (`total`), one value per thread, client, receiver or phase, and a `[HOSTINFO]` section (host, date, compiler). For example `cputest --num-threads=2 --datfile=cpu.dat`:

```
[BENCHMARK]
name=cputest
version=1.4
threads=2
buf_size=1e+07
repeat=1
total=40.8762
total_unit=MB/s
total_label=zlib compress + uncompress rate
elapsed=0.244641
items=Thread 0,Thread 1
item_values=0.243874,0.229364
item_unit=seconds

[HOSTINFO]
host=host1.example.com
date=11/19/2026
build=GCC
title=CPU Speed Benchmark
```

`disktest --datfile` writes the older disk benchmark format (`[DISKBENCHMARK]`, `[PLOTINFO]`, `[HOSTINFO]`) with the time at every 10% of each phase.

The files are the same on Windows and Linux, so results from both plot and report together.

## dr_plot.py

Plots `--datfile` files, `sar_stats --csv` and `sar_cpu_stats --csv` output and `watch_mem_usage.sh` logs. It tells the kind from the contents. Needs Python 3 and matplotlib.

```
dr_plot.py [-h] [-o OUT] [--averages] [--cpu CPU] [--dpi DPI] files [files ...]
```

| Option | Meaning |
|---|---|
| `files` | one or more input files |
| `-o OUT`, `--out OUT` | write this PNG file (without `-o` the plot opens in a window) |
| `--averages` | disktest: milliseconds per key instead of seconds per 10% of the keys |
| `--cpu CPU` | sar CSV: the CPU to plot (default `all`; needs `--csv --all-cpus` input) |
| `--section ID,...` | sar_stats CSV: only these sections (`cpu,mem,disk` ...) |
| `--item NAME,...` | sar_stats CSV: only these items (`all`, `eth0`, `sda` ...); every one is drawn |
| `--items N` | sar_stats CSV: the busiest items drawn per section (default 6) |
| `--dpi DPI` | PNG resolution |

| Input | Plot |
|---|---|
| one benchmark `--datfile` | a bar per thread, client, receiver or phase, the total in the title; UDP and multicast also show the loss per client |
| several files of one benchmark | one bar per run (host, date, threads or clients) for the main result: compare machines, builds or thread counts |
| `disktest --datfile` | insert, find, delete and reinsert times for every 10% of the keys; several files compare the phase totals |
| `sar_stats --csv` | every sar section: a chart per section and item (the whole system first, then the busiest items), the columns split into panels by unit; CPU use stacked like kSar. `-o file.pdf` writes one page per chart, `-o file.png` one file per chart (`file_<section>_<item>.png`). See [sar_stats](sar-stats.md) |
| `sar_cpu_stats --csv` | CPU use over time, stacked (user, nice, system, irq + soft, iowait, steal), one panel per host, gaps left empty and bad samples marked in red |
| `watch_mem_usage.sh` log | used memory over time |

Linux:

```
dr_plot.py cpu_host1.dat                                 # a window
dr_plot.py -o cpu.png cpu_host1.dat
dr_plot.py -o cpu_cmp.png cpu_host1.dat cpu_host2.dat    # compare runs
dr_plot.py --averages -o disk_ms.png disk.dat
sar_cpu_stats --csv sar_host1.txt > host1.csv && dr_plot.py -o host1_cpu.png host1.csv
watch_mem_usage.sh -i 5 > mem.log & memtest --num-threads=8; kill %1; dr_plot.py -o mem.png mem.log
```

Windows (`py` launcher, from the dr_utils folder):

```
py benchmarks\plot\dr_plot.py -o cpu.png cpu_host1.dat
py benchmarks\plot\dr_plot.py -o cpu_cmp.png results\cpu_*.dat
py benchmarks\plot\dr_plot.py --averages --dpi 150 -o disk.png disk.dat
```

On a Linux host with no display, use `-o`.

## dr_report.py

Builds one HTML report from a folder (or a list) of result files: `--datfile` files, `sar_stats --csv` and `sar_cpu_stats --csv` files and `watch_mem_usage.sh` logs. The charts are inside the HTML file, so the one file can be mailed, archived or printed.

```
dr_report.py [-h] -o OUT [--title TITLE] [--csv CSV] [--no-runs]
             [--submit [ID]] [--now] [--drra DRRA] [--drra-config DRRA_CONFIG]
             inputs [inputs ...]
```

| Option | Meaning |
|---|---|
| `inputs` | result files or folders |
| `-o OUT`, `--out OUT` | the HTML file to write (required) |
| `--title TITLE` | the report title (default "Benchmark report") |
| `--csv CSV` | also write the summary table as CSV |
| `--no-runs` | only the comparison charts, not one chart per run |
| `--sar-section ID,...` | sar_stats CSV: only these sections |
| `--sar-items N` | sar_stats CSV: charts of the busiest items per section (default 4) |
| `--sar-rows N` | sar_stats CSV: items in the per-item table of a section (default 20) |
| `--submit [ID]` | hand the report to the reporting agent, hook ID (default `dru_report`), see [Dashboard](dashboard-hooks.md) |
| `--now` | with `--submit`, the agent sends within seconds |
| `--drra DRRA` | agent program (default `drra`, or `%ProgramFiles%\DataReel\ReportingAgent\drra_svc.exe` on Windows) |
| `--drra-config DRRA_CONFIG` | agent settings file, `drra -c INI` (Linux, for tests) |

The report has:

- a summary table of every run (benchmark, host, date, compiler, settings, result);
- a section for each benchmark, with a comparison chart when there are several runs and a chart for each run;
- for each sar CSV, a table of averages and peaks per host (user, system, iowait, steal, lowest idle), the count of bad samples, and the CPU chart;
- the memory logs.

Files in a folder that are not results are listed as skipped.

Linux:

```
mkdir results
cputest --num-threads=8 --datfile=results/cpu.dat
disktest --file=/data/testfile.btx --datfile=results/disk.dat
sar_cpu_stats --csv sar_host1.txt > results/host1_sar.csv
dr_report.py -o host1_report.html --title "host1 sizing" --csv host1_summary.csv results
dr_report.py -o compare.html --no-runs host1_results host2_results
```

Windows:

```
py benchmarks\plot\dr_report.py -o report.html --title "file server sizing" results
py benchmarks\plot\dr_report.py -o compare.html --no-runs --csv compare.csv \\fileserver.example.com\bench\host1 \\fileserver.example.com\bench\host2
```

## Watch scripts (Linux)

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
