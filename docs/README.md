# dr_utils 1.4 documentation

dr_utils is a set of command line file tools, network tools, system benchmarks and a sar report reader for Windows and Linux, built on the DataReel C++ library 6.4.1. These pages cover every program: what it does, every option, and command lines for both systems.

| Page | What it covers |
|---|---|
| [Building and installing](install.md) | DataReel 6.4.1, `build.sh` and `build.cmd`, zlib, putting the programs on the PATH, privileges, the test suites |
| [File tools](file-tools.md) | `file_hours`, `file_purge`, `filetimes` |
| [Network tools](network-tools.md) | `internet-test`, `pingscan`, `dnsscan`, `portscan`, `netpscan`, `arpscan`, `getmac` |
| [Benchmarks](benchmarks.md) | `cputest`, `disktest`, `memtest`, `flops`, `tcpserv`/`tcptest`, `udpserv`/`udptest`, `mcserv`/`mctest` |
| [sar reports, every section](sar-stats.md) | `sar_stats`: every section of sar -A reports (CPU, memory, swap, paging, I/O, disks, network, NFS, load, interrupts, power, file systems), kSar-style summaries, CSV and charts |
| [sar CPU statistics](sar-cpu-stats.md) | `sar_cpu_stats`: averages, peaks, steal and iowait spikes, gaps, CSV |
| [Plots, reports and watch scripts](plots-and-reports.md) | `--datfile` files, `dr_plot.py`, `dr_report.py`, `watch_system.sh`, `watch_mem.sh`, `watch_mem_usage.sh` |
| [Dashboard: reporting agent hooks](dashboard-hooks.md) | `--drsm`, `--submit`, the DataReel Reporting Agents for Linux and Windows, DRSM setup, the daily sar job |
| [Quick reference](quick-reference.md) | Every program and option on one page, with exit status |

## HTML version and folder READMEs

`docs/html/` has the same pages as HTML with a side menu and a search box (press `/` to search). Open `docs/html/index.html` in a browser; it works straight from disk on Windows and Linux, with no web server and no network, or copy the folder to a web server. A search can be linked as `index.html?q=words`.

Each program folder also has a README.md with that program's part of these pages (`tools/portscan/README.md`, `benchmarks/cputest/README.md`, `benchmarks/plot/README.md`, `system_stats/sar_cpu_stats/README.md` ...), so the text docs are next to the source.

The markdown pages here are the source. After editing them (or `hooks/README.md`), rebuild, and commit the pages, `docs/html` and the folder READMEs together:

```
python3 docs/make_docs.py            # Linux (needs python3-markdown, or pip install markdown)
py docs\make_docs.py                 # Windows
python3 docs/make_docs.py --check    # exit 1 if docs/html or a folder README is out of date
```

## Conventions in these pages

- Linux examples run in bash, with the programs on the PATH (or as `bin/<program>` from the dr_utils folder).
- Windows examples run in `cmd.exe` unless they say PowerShell, with the programs on the PATH (or as `bin\<program>.exe`). The programs are the same on both systems, with the same options.
- Addresses and names in the examples are documentation values: `192.0.2.x` and `example.com`. Put in your own.
- Use the network tools and network benchmarks only on networks and hosts you own or are allowed to test. The scan tools scan one /24 at most.

## Common behaviour

- Every program prints its version and usage with no arguments (the file and network tools), with `-?`, or with `--help` (the benchmarks and `sar_cpu_stats`). The usage text is the reference these pages were written from.
- Sizes and counts in the benchmarks take decimal suffixes: `1k` = 1000, `1m` = 1000000, `1g` = 1000000000 (upper case works too: `150M`, `1G`).
- Wildcards such as `*.log` work on Windows in `cmd.exe` and PowerShell, because the programs link `setargv.obj` and expand them themselves.
- Exit status: each page lists it. On Linux read `$?`, in `cmd.exe` read `%ERRORLEVEL%`, and in PowerShell read `$LASTEXITCODE`.
