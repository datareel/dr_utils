# Quick reference

Every program and option on one page. On Windows the programs are `<name>.exe` with the same options. Dashboard options (`--drsm`, `--submit[=ID]`, `--now`, `--drra`, `--drra-config`) are marked **[D]**, see [Dashboard](dashboard-hooks.md).

## File tools ([details](file-tools.md))

| Program and options | Exit status |
|---|---|
| `file_hours [-s] filename numhours` | 0 YES or NOT, 1 ERR; with `-s`: 0 YES, 1 NOT, 2 ERR |
| `file_purge [-e \| -l \| -h<n> \| -d<n>] [-m"dir"] [-n] [-v] file ...` | 0 files checked, 1 usage error |
| `filetimes [-n \| -o] file ...` | 0, 1 if a file cannot be read |

## Network tools ([details](network-tools.md))

| Program and options | Exit status |
|---|---|
| `internet-test [-c n] [-w sec] [-t ms] [-p port] [-q] server` (defaults: `-c 0` until it passes, `-w 60`, `-t 5000`, `-p 80`) | 0 passed, 1 failed, 2 error |
| `pingscan [-t ms] [-u] range` (`-t 1000`) | 0 any answered, 1 none, 2 error |
| `dnsscan [-a] [-f] [-p] [-t ms] range` | 0 any name, 1 none, 2 error |
| `portscan [-t ms] [-n] hostname ports [services_file]` (`-t 2000`, up to 1024 ports) | 0 any open, 1 none, 2 error |
| `netpscan [-p ports] [-A] [-t ms] [-s] iplist_file [services_file]` (`-t 500`) | 0 ran, 2 error |
| `arpscan [-i dev] [-v vendor_file] [-o out_file] [-t ms] [-a] range` | 0 any MAC, 1 none, 2 error |
| `getmac hostname [ethernet_device]` | 0 MAC found, 1 no ARP entry, 2 error |

Ranges: `192.0.2` (.1 to .254), `192.0.2.7`, `192.0.2.10-40`.

## Benchmarks ([details](benchmarks.md))

| Program and options | Defaults |
|---|---|
| `cputest [--num-threads=N] [--buf-size=SIZE] [--repeat=N] [--datfile=f]` **[D]** | 1 thread, `100m`, 1 pass |
| `flops [--num-threads=N] [--dp] [--div] [--iterations=N] [--datfile=f]` **[D]** | 1 thread, single precision, `1g` |
| `memtest [--num-threads=N] [--num-inserts=N] [--datfile=f]` **[D]** | 1 thread, `25k` keys (max `1m`) |
| `disktest [--num-inserts=N] [--file=f] [--keep] [--datfile=f]` **[D]** | `1m` keys, `testfile.btx` |
| `tcpserv [-p port] [-l address] [--count=N] [--timeout=S] [--max-bytes=SIZE]` | port 53897, all addresses, until Ctrl-C, 30 s, `100g` |
| `tcptest [-b size] [-d] [-n clients] [-p port] [-s bytes] [--datfile=f]` **[D]** `server` | 1500, upload, 1 client, 53897, `150M` |
| `udpserv [-p port] [-l address] [--count=N]` | port 53897 |
| `udptest [-b size] [-n clients] [-p port] [-r rate] [-s bytes] [--datfile=f]` **[D]** `server` | 1500, 1 client, 53897, no rate limit, `150M` |
| `mcserv [-b size] [-g group] [-i address] [-p port] [-r rate] [-s bytes] [--seconds=N] [--ttl=N] [--no-loop]` | 1500, 239.255.10.10, 53898, `100M` bits/s, `150M`, TTL 1 |
| `mctest [-g group] [-i address] [-n receivers] [-p port] [--timeout=S] [--datfile=f]` **[D]** `[group]` | 239.255.10.10, 53898, 60 s |

Exit status: `tcptest` 0 if every client finished; `udptest` 0 if every client got a report; `mctest` 0 if every receiver saw the end of a run; 1 otherwise.

Sizes: `1k` = 1000, `1m` = 10^6, `1g` = 10^9. Rates (`-r`) are bits a second.

## sar reports ([details](sar-cpu-stats.md))

| Program and options | Exit status |
|---|---|
| `sar_cpu_stats [--csv [--all-cpus]] [--steal=PCT] [--iowait=PCT] [--list=N] [--alarm]` **[D]** `sar_file ...` | 0; 1 error or no samples; 3 with `--alarm` and findings |

## Scripts ([plots and reports](plots-and-reports.md), [dashboard](dashboard-hooks.md))

| Script | Options |
|---|---|
| `benchmarks/plot/dr_plot.py` | `[-o OUT] [--averages] [--cpu CPU] [--dpi DPI] files ...` |
| `benchmarks/plot/dr_report.py` | `-o OUT [--title T] [--csv CSV] [--no-runs] [--submit [ID]] [--now] [--drra P] [--drra-config INI] inputs ...` |
| `benchmarks/scripts/watch_system.sh` | `[-i sec] [-n count] [STATSONLY]` (Linux) |
| `benchmarks/scripts/watch_mem.sh` | `[-i sec] [-n count] [STATSONLY]` (Linux) |
| `benchmarks/scripts/watch_mem_usage.sh` | `[-i sec] [-n count] [STATSONLY]` (Linux) |
| `hooks/linux/install_hooks.sh` | `[--sar-cron] [--apply] all \| <id> ...` (dry run without `--apply`) |
| `hooks/linux/dru_sar_cpu_stats.sh` | `[--day=yesterday\|today\|YYYY-MM-DD] [--sa-dir=DIR] [--now] [--print] [sar_cpu_stats options]` |
| `hooks/windows/install_hooks.ps1` | `all \| <id> ... [-Apply]` (dry run without `-Apply`) |
| `build.sh` / `build.cmd` | `[clean]`, with `DATAREEL_DIR` set (and `ZLIB_DIR` on Windows) |
| `tests/run_tests.sh` / `tests\run_tests.ps1` | `run_tests.ps1 [-NoLan]` |

## Ports

| Port | Used by |
|---|---|
| TCP 53897 | `tcpserv` / `tcptest` |
| UDP 53897 | `udpserv` / `udptest` |
| UDP 53898, group 239.255.10.10 | `mcserv` / `mctest` |
