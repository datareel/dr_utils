# dr_utils 1.4

Small command line file and network tools for Windows and Linux, built on the [DataReel](https://datareel.com) C++ library, version 6.4.1. They are open source (GNU LGPL 3) versions of tools based on work from 1999 to 2010. DataReel 7.4 does not already have them.

| Tool | What it does |
|---|---|
| `file_hours` | Says YES if a file was last changed N hours ago or more, else NOT: a stale file check for scripts |
| `file_purge` | Deletes or moves files N hours or days old, with a dry run and a listing mode |
| `filetimes` | Shows a file's access, change and creation times in local time and GMT, and its age; finds the newest or oldest of several files |
| `internet-test` | Waits until a web server answers over HTTP (DNS, connect and HTTP checked) |
| `pingscan` | Ping sweep of a /24 network or a range, with DNS names and reply times |
| `dnsscan` | Lists the DNS names in a /24 range, with forward/reverse checks and an optional ping |
| `portscan` | Checks one host: names, ping, TCP connect to a port, a list or a range, and service names |
| `netpscan` | Scans a list of addresses for open TCP ports (services file ports, or a list) |
| `arpscan` | Lists the MAC address (and vendor) of each host on a local /24 network |
| `getmac` | Shows the MAC address of one host on a local network |

It also has system benchmarks for CPU, disk, memory, TCP, UDP and multicast:

| Benchmark | What it measures |
|---|---|
| `cputest` | CPU speed: zlib best compression and uncompression of random data, in one or more threads |
| `disktest` | Disk speed for database index work: a DataReel B-tree file, with insert, find, delete and insert-again phases, and plot data |
| `memtest` | Memory speed: binary search tree insert, find and delete of string keys, in one or more threads |
| `tcpserv`, `tcptest` | TCP bandwidth, upload or download, with one or more clients |
| `udpserv`, `udptest` | UDP bandwidth and loss, with one or more clients |
| `mcserv`, `mctest` | Multicast rate, loss and order from one sender to one or more receivers |
| `flops` | Floating point operations per second, single or double precision, in one or more threads |

And a system statistics tool:

| Tool | What it does |
|---|---|
| `sar_cpu_stats` | Reads sar text reports: CPU averages and peaks, steal and iowait spikes, gaps and impossible samples, or every sample as CSV |

The benchmarks, `sar_cpu_stats` and `dr_report.py` can also send their results to the DataReel System Monitor dashboard through the DataReel Reporting Agents for Linux and Windows (`--submit`), see [Dashboard](#dashboard-reporting-agent-hooks).

Use the network tools and benchmarks only on networks and hosts you own or are allowed to test. The scan tools scan one /24 at most.

## License

GNU Lesser General Public License version 3 or later, the same as DataReel 6.4.2 and later. See [LICENSE](LICENSE). Each file says which original program it is based on.

## Building

You need a DataReel 6.4.1 source tree with its library built. Use the same build settings for the library and for dr_utils.

### Linux (RHEL 8, 9, 10 and other distributions with GCC)

```
cd /path/to/datareel-6.4.1/build && make && make install   # lib/libdatareel.a
cd /path/to/dr_utils
DATAREEL_DIR=/path/to/datareel-6.4.1 ./build.sh            # programs in bin/
tests/run_tests.sh
```

cputest needs zlib (zlib-devel). DataReel builds with OpenSSL by default (openssl-devel). Use `USE_OPEN_SSL=NO` for both the library and dr_utils on a system without it. One tool: `cd tools/portscan && make DATAREEL_DIR=...`.

### Windows (Visual Studio 2022 or 2026, x64)

Build the DataReel library in `%DATAREEL_DIR%\winslib` (gxcode64.lib, 64-bit release). Then, from an x64 Native Tools command prompt:

```
set DATAREEL_DIR=C:\path\to\datareel-6.4.1
set ZLIB_DIR=C:\path\to\zlib     (for cputest: zlib.h and a /MT zlib.lib)
cd \path\to\dr_utils
build.cmd
powershell -ExecutionPolicy Bypass -File tests\run_tests.ps1
```

`build.cmd` builds 64-bit release programs (`64BITCFG=1 FINAL=1`) into `bin\`. One tool: `cd tools\portscan` and `nmake -f msvc.mak`. For cputest, build zlib with the static runtime so it matches DataReel's /MT: in the zlib source folder, `nmake -f win32\Makefile.msc LOC=-MT zlib.lib`. The programs link `setargv.obj`, so wildcards such as `*.log` work in cmd.exe.

## Privileges

- **Ping** (pingscan, dnsscan -p, arpscan, and the informational pings of portscan, netpscan and getmac): on Linux an ICMP datagram socket is used when `net.ipv4.ping_group_range` includes the user's group (the RHEL default allows every group), else a raw socket, which needs root or `setcap cap_net_raw+ep`. On Windows `IcmpSendEcho` needs no administrator rights.
- **MAC addresses**: Linux reads `/proc/net/arp` and Windows uses `SendARP`. Neither needs root. Only hosts on a directly attached network have a MAC address.
- **TCP connects and DNS** need no privileges.

## Tool reference

Every tool prints its usage with no arguments or `-?`.

### file_hours

```
file_hours [-s] filename numhours
```

Prints `YES` if the file mod time is `numhours` old or older, `NOT` if it is newer, and `ERR - <reason>` if the file cannot be checked. `numhours` can be fractional (0.5 is 30 minutes). The exit status is 0 for YES or NOT and 1 for ERR. With `-s` it is 0 for YES, 1 for NOT and 2 for ERR.

```
if [ "$(file_hours /var/spool/feed/latest.dat 2)" = "YES" ]; then echo "feed is stale"; fi
```

### file_purge

```
file_purge [-e | -l | -h<n> | -d<n>] [-m"dir"] [-n] [-v] file [file ...]
```

- `-h<n>` or `-d<n>`: delete each file that is n hours or days old or older.
- `-m"dir"`: move the file instead (a copy and delete across file systems). An existing file in `dir` is never overwritten.
- `-e`: delete the file if it exists, with no time check.
- `-l`: list each file's age and change nothing.
- `-n`: dry run. Print what would be done (`DRY - ...`) and change nothing.

Directories are never purged. Each file gets one line: `YES`, `NOT`, `ERR`, `LST` or `DRY`. Run with `-n` first.

```
file_purge -n -d30 /var/log/myapp/*.log     # what would go
file_purge -d30 /var/log/myapp/*.log
file_purge -m"D:\archive" -h12 C:\spool\*.dat
```

### filetimes

```
filetimes [-n | -o] file [file ...]
```

Prints each file's access, modification and creation times (on Linux the inode change time) in local time and GMT, and how old it is. `-n` or `-o` prints only the name of the newest or oldest file.

### internet-test

```
internet-test [-c n] [-w sec] [-t ms] [-p port] [-q] server
```

Resolves `server`, connects to its HTTP port and reads a response header. Any HTTP status passes. By default it tries every 60 seconds until it passes (`-c 0`). Use `-c` for a fixed number of tries. `server` can be a host name, `host:port` or a URL. Exit status 0 passed, 1 failed, 2 usage error.

```
internet-test -c 10 -w 30 www.example.com && start_my_job
```

### pingscan

```
pingscan [-t ms] [-u] range
```

`range` is `a.b.c` (hosts .1 to .254), `a.b.c.d` or `a.b.c.x-y`. Each address is pinged and its DNS name looked up. `-u` lists only the hosts that answer. The output ends with "n of m hosts answered".

### dnsscan

```
dnsscan [-a] [-f] [-p] [-t ms] range
```

Prints one tab-separated line per named address: address, reverse DNS name, then the `-f` forward check (`fwd_ok`, or `fwd_mismatch` with what the name resolves to), then the `-p` ping result (`up (ms)` or `down`). `-a` also lists addresses without a name.

### portscan

```
portscan [-t ms] [-n] hostname ports [services_file]
```

`ports` is a port, a list or a range (`22`, `22,80,443`, `8000-8100`, up to 1024 ports). The tool shows the host's names and addresses, pings it (`-n` skips the ping), connects to each port with a timeout (default 2000 ms) and shows the services file entry of each open port. A host that drops ICMP is still scanned. Exit status 0 if any port is open, 1 if none, 2 on an error.

### netpscan

```
netpscan [-p ports] [-A] [-t ms] [-s] iplist_file [services_file]
```

`iplist_file` has one address or host name per line, with `#` comments. By default every address is scanned on the TCP ports below 1024 that are named in the services file. `-A` scans every TCP entry in the services file, and `-p` scans a port list instead. `-s` skips addresses that do not answer a ping. Each open port is printed as `port/tcp name aliases # comment`.

### arpscan

```
arpscan [-i dev] [-v vendor_file] [-o out_file] [-t ms] [-a] range
```

Pings each address, then reads its MAC address from the system ARP table. One tab-separated line per host: address, DNS name, MAC address and, with `-v`, the vendor. The vendor file has one `XXXXXX Vendor Name` line per MAC prefix (6 hex digits, the DataReel `FindVendor` format). An IEEE OUI list converts to it. `-i` (Linux) uses only entries on one interface. `-o` also writes the list to a file.

### getmac

```
getmac hostname [ethernet_device]
```

Shows the host's names, pings it and prints `MAC Address: XX:XX:XX:XX:XX:XX`. The exit status is 1 if there is no ARP entry. Windows has its own `getmac.exe`, which lists the local adapters. Run this one by its full path, or rename it, if both are on the PATH.

## Benchmark reference

Sizes and counts take decimal suffixes: `1k` is 1000, `1m` is 1000000, `1g` is 1000000000, the same units as the 2002-2019 benchmarks. Times are wall clock. Every benchmark prints its usage with `-?` or `--help`.

### cputest

```
cputest [--num-threads=NUM] [--buf-size=SIZE] [--repeat=NUM] [--datfile=fname]
```

Fills a buffer (default 100m) with random bytes and splits it between the threads (default 1). Each thread compresses its part with zlib at level 9, uncompresses it and checks that it matches. The output has each thread's time, the average, and the total time with the rate. Run it with one thread for single-core speed and one thread per core for the whole CPU.

### disktest

```
disktest [--num-inserts=NUM] [--file=fname] [--keep] [--datfile=fname]
```

Creates a DataReel B-tree index file (order 128, default `testfile.btx` in the current folder; use `--file` to put it on the disk under test). It then inserts, finds, deletes (checking each delete) and inserts again NUM keys (default 1m), with the time at every 10%. `--datfile` writes the times in the old disk benchmark plot format (`[DISKBENCHMARK]`, `[PLOTINFO]`, `[HOSTINFO]`). The file is removed at the end unless `--keep` is given, and an existing file is never overwritten.

### memtest

```
memtest [--num-threads=NUM] [--num-inserts=NUM] [--datfile=fname]
```

Each thread builds its own unbalanced binary search tree of "Item n" strings (default 25k keys, at most 1m) and times the insert, find, delete and insert-again phases. The keys arrive nearly sorted, so the tree is deep and the time grows faster than the key count. Compare runs that use the same `--num-inserts`.

### tcpserv and tcptest

```
tcpserv [-p port] [-l address] [--count=N] [--timeout=S] [--max-bytes=SIZE]
tcptest [-b size] [-d] [-n clients] [-p port] [-s bytes] [--datfile=fname] server
```

Start `tcpserv` on one host (port 53897 by default, all addresses unless `-l` is given), then run `tcptest` on another. Each client connects and uploads `-s` bytes (default 150M) in `-b` byte writes (default 1500). With `-d` it downloads them instead. The server confirms the byte count, and each client and the total print their rates. The server runs until Ctrl-C, or until `--count` transfers are done. Clients cannot stop or restart it.

### udpserv and udptest

```
udpserv [-p port] [-l address] [--count=N]
udptest [-b size] [-n clients] [-p port] [-r rate] [-s bytes] [--datfile=fname] server
```

The clients send numbered datagrams of `-b` bytes (default 1500, at most 65507) to `udpserv`. They send as fast as the host can, or at `-r` bits a second per client (for example `-r500M`). At the end the server reports what arrived, and each client prints its send rate, its receive rate and the loss.

### mcserv and mctest

```
mctest [-g group] [-i address] [-n receivers] [-p port] [--timeout=S] [--datfile=fname]
mcserv [-b size] [-g group] [-i address] [-p port] [-r rate] [-s bytes] [--seconds=N] [--ttl=N] [--no-loop]
```

Start one or more `mctest` receivers, then run `mcserv`. The sender sends `-s` bytes (default 150M) of numbered datagrams to the group (default 239.255.10.10, port 53898) at `-r` bits a second (default 100M), then sends end-of-run datagrams. Each receiver prints what it got: datagrams, loss, out-of-order count and rate. The defaults are cautious: TTL 1 keeps the traffic on the local network, and the group is administratively scoped. `-i` picks the interface; `-i127.0.0.1` on both ends keeps a test on one host.

Allow the benchmark ports (TCP 53897, UDP 53897 and 53898) in the host firewalls for tests between hosts.

### flops

```
flops [--num-threads=NUM] [--dp] [--div] [--iterations=NUM] [--datfile=fname]
```

`--div` runs the formula of the old c/flops program: a multiply, a divide and an add per element (`a*c + a/b`), counted as 3 operations. The old program counted 5, and its loop was removed by the compiler. Code with divisions runs slower than multiply-add, so this mode gives the rate such code gets.

Runs multiply-add chains over a small array that stays in the CPU cache, in single precision (or double with `--dp`). The iterations (default 1g) are split between the threads. It prints MFLOPS or GFLOPS. The rate depends on the compiler and its flags (vector units), so compare machines with the same build.

### Plot data files and dr_plot.py

Every benchmark takes `--datfile=fname` (disktest, cputest, memtest, flops, tcptest, udptest, mctest) and writes its results as a small INI file: the settings, the main result (`total`), one value per thread, client, receiver or phase, and a `[HOSTINFO]` section (host, date, compiler). `benchmarks/plot/dr_plot.py` (Python 3 with matplotlib) plots those files, `sar_cpu_stats --csv` output and `watch_mem_usage.sh` logs. It tells the kind from the contents:

| Input | Plot |
|---|---|
| one benchmark `--datfile` | a bar per thread, client, receiver or phase, the total in the title; UDP and multicast also show the loss per client |
| several files of one benchmark | one bar per run (host, date, threads or clients) for the main result: compare machines, builds or thread counts |
| `disktest --datfile` | insert, find, delete and reinsert times for every 10% of the keys (`--averages` for milliseconds per key); several files compare the phase totals |
| `sar_cpu_stats --csv` | CPU use over time, stacked (user, nice, system, irq + soft, iowait, steal), one panel per host, gaps left empty and bad samples marked in red; `--cpu N` for one CPU (with `--all-cpus`) |
| `watch_mem_usage.sh` log | used memory over time |

```
cputest --num-threads=8 --datfile=cpu_host1.dat
dr_plot.py -o cpu.png cpu_host1.dat            # one run
dr_plot.py -o cpu_cmp.png cpu_host1.dat cpu_host2.dat   # compare runs
sar_cpu_stats --csv sar_host1.txt > host1.csv
dr_plot.py -o host1_cpu.png host1.csv
```

Without `-o` the plot opens in a window.

### Reports: dr_report.py

`benchmarks/plot/dr_report.py` builds one HTML report from a folder (or a list) of result files: `--datfile` files, `sar_cpu_stats --csv` files and `watch_mem_usage.sh` logs. The report has:

- a summary table of every run (benchmark, host, date, compiler, settings, result);
- a section for each benchmark, with a comparison chart when there are several runs (machines, builds, thread counts) and a chart for each run;
- for each sar file, a table of averages and peaks per host (user, system, iowait, steal, lowest idle), the count of bad samples, and the CPU chart;
- the memory logs.

The charts are inside the HTML file, so the one file can be mailed, archived or printed. Files in the folder that are not results are listed as skipped.

```
mkdir results
cputest --num-threads=8 --datfile=results/cpu.dat
disktest --file=/data/testfile.btx --datfile=results/disk.dat
sar_cpu_stats --csv sar_host1.txt > results/host1_sar.csv
dr_report.py -o host1_report.html --title "host1 sizing" --csv host1_summary.csv results
```

`--csv` also writes the summary table as CSV, and `--no-runs` keeps only the comparison charts. dr_plot.py and dr_report.py are tested on Linux (`tests/run_tests.sh`); the result files from Windows runs plot the same way.

### Dashboard (reporting agent hooks)

Every benchmark and `sar_cpu_stats` take `--drsm=fname` and `--submit[=ID] [--now]`, and `dr_report.py` takes `--submit [ID] [--now]`. `--drsm` writes the results as DRSM lines: an INFO line with the main result, a value for each setting and result, and a table of the threads, clients, receivers, phases or sar files. `--submit` hands them to the DataReel Reporting Agent (`drra submit --hook dru_<program>` on Linux, `drra_svc.exe submit` on Windows), which sends them to the report server; DRSM then shows them on the host's page, with alert rules and trends. `dr_report.py --submit` sends the whole HTML report, shown by DRSM's html display.

```
cputest --num-threads=8 --submit --now
sar_cpu_stats --submit sar_yesterday.txt
dr_report.py -o host_report.html --submit results/
```

The agents' upload hook files, the DRSM manifests, install scripts (dry run by default) and a daily sar cron job are in [hooks/](hooks/README.md), with the setup steps and the values each program sends. No change to the agents or DRSM is needed.

### Watch scripts

- `benchmarks/scripts/watch_system.sh [-i sec] [-n count] [STATSONLY]` records the release, kernel, CPU, memory and disk statistics from /proc (between `START name` and `END name` lines), then runs `top -b` while a benchmark runs (Linux).
- `benchmarks/scripts/watch_mem.sh` prints `free` and /proc/meminfo every interval. `watch_mem_usage.sh` prints one "date time used-KB" line per interval, for plotting (Linux).

## sar_cpu_stats

```
sar_cpu_stats [--csv [--all-cpus]] [--steal=PCT] [--iowait=PCT] [--list=N] [--alarm] sar_file [sar_file ...]
```

Reads sar text reports (`LC_ALL=C sar -u -f /var/log/sa/saDD > file.txt`, or `sar -P ALL`, or a whole `sar -A` report, one day or many days in a file). It works with RHEL 5 to RHEL 10 sysstat: 12- or 24-hour times, `%user` or `%usr`, with or without the newer columns. For the "all" CPU rows it prints:

- the average and peak of user, nice, system, iowait, steal, irq, soft, guest, gnice and idle (the lowest idle), with the peak times;
- steal and iowait spikes over the thresholds (default 10% and 20%);
- gaps: a missing run of samples inside a day, or a day whose samples stop early while later days follow (a host or VM that stopped writing);
- bad samples: a percent over 100, or parts that do not add up to about 100. A VM the hypervisor stopped can write steal of 241%. Bad samples are listed and left out of the averages.

`--csv` writes every sample instead, as `host,time,cpu,user,...,idle,ok|bad`. `--all-cpus` adds the per-CPU rows. `--alarm` sets exit status 3 when there are spikes, gaps or bad samples, for cron. The reports are plain text, so the tool reads them on Windows too. `tests/data/sar` has short example reports.

## Layout

| Path | What |
|---|---|
| `tools/<name>/` | one folder per tool: source, `makefile` (Linux), `msvc.mak` (Windows), `project.mak` (objects for both) |
| `benchmarks/<name>/` | one folder per benchmark program, laid out the same way; `benchmarks/plot` and `benchmarks/scripts` |
| `system_stats/sar_cpu_stats/` | the sar report tool |
| `tests/data/sar/` | short sar reports for the tests |
| `common/` | `drnet.h/.cpp` (ping, TCP connect with a timeout, DNS, ARP, ranges, services files), `drsock.h/.cpp` (benchmark sockets and wire formats), `drbench.h` (timers, sizes, pacing, plot files), `drhook.h` (DRSM lines and the agent hand-off) and `drutil.h` (file ages) |
| `hooks/` | reporting agent upload hook files, DRSM manifests and install scripts for the dashboard output |
| `env/` | `linux.mak` and `msvc.mak`, the build settings on top of DataReel's |
| `tests/` | `run_tests.sh` (Linux), `run_tests.ps1` (Windows) and `hooks_e2e_test.sh` (dashboard output through a real agent and DRSM probe) |

## History

| Tool | Based on |
|---|---|
| file_hours, file_purge, filetimes | the 2005 file tools |
| internet-test | the 2005 Internet connectivity test |
| pingscan, dnsscan, portscan, netpscan, arpscan, getmac | the 1999-2010 network scan programs |
| cputest, disktest, memtest, tcpserv/tcptest, udpserv/udptest, mcserv/mctest | the 2002-2010 bmarks programs and their 2016-2019 versions in work_programs/benchmarks |
| flops, dr_plot.py, watch scripts | work_programs/benchmarks (2009-2019); dr_plot.py grew from the 2016 disk plotter |
| sar_cpu_stats | the 2018 system_stats_analysis_tool read_cpu_stats (unfinished there) |

The 1.0 versions keep the original switches and output lines, and fix these problems in the old versions:

- Ages under one hour or one day counted as zero, so `-h0.5` never matched.
- A filtered port waited for the system connect timeout, about two minutes.
- One failed ping stopped a sweep.
- The Windows ARP lookup ran `arp -a` into a file in the current folder.
- Windows needed administrator rights for ICMP.

In the benchmarks (1.1 and 1.2):

- Times were clock() sums, which on Linux count the CPU time of every thread.
- UDP and multicast loss was never measured.
- A client could shut down or restart a server over the network.
- The multicast sender flooded the network until it was stopped.
- The old flops loop's result was never used, so an optimizing compiler removed the loop and the test timed nothing.
