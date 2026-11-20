# sar_stats

Reads every section of sar text reports, the way the old kSar tool did: CPU, memory, swap, paging, I/O, block devices, network interfaces, sockets, IP, ICMP, TCP and UDP, NFS, run queue and load, interrupts, power and file systems. For each section it prints the average, lowest and highest value of every column, with the time of the highest, for the whole system and for each item (each CPU, disk, network interface, file system ...). With `--csv` it writes every value, and `dr_plot.py` and `dr_report.py` chart every section from that file.

`sar_cpu_stats` stays for CPU use with steal and iowait spike alarms; `sar_stats` is for the whole report.

```
sar_stats [args] sar_file [sar_file ...]
```

| Option | Meaning |
|---|---|
| `--list` | list the sections found: id, sar option, items, samples, title and columns |
| `--csv` | every value as CSV: `host,time,section,item,metric,value` |
| `--section=ID[,ID]` | only these sections (ids from `--list`: `cpu`, `mem`, `disk`, `net_dev` ...) |
| `--item=NAME[,NAME]` | only these items: `all`, `0`, `eth0`, `sda`, `dev8-0`, a file system ... |
| `--items=N` | items listed per section in the summary (default 20); `--csv` always has every item |
| `--help`, `--version` | usage, version |
| `--drsm`, `--submit`, `--now`, `--drra`, `--drra-config` | dashboard output, see [Dashboard](dashboard-hooks.md) |

Exit status: 0; 1 on an error or when a file has no sar samples.

## Input

Any sar text report from RHEL 5 to RHEL 10 sysstat (and other distributions): one day or several days in a file, 12- or 24-hour times, LINUX RESTART lines. The full report is the most useful:

```
LC_ALL=C sar -A -f /var/log/sa/sa19 > sar_19.txt                  # every section of one day
LC_ALL=C sar -A -f /var/log/sa/sa20261119 > sar_19.txt            # sysstat set to long file names
for d in 17 18 19; do LC_ALL=C sar -A -f /var/log/sa/sa$d; done > sar_3days.txt
LC_ALL=C sar -d -p -f /var/log/sa/sa19 > disks_19.txt             # only block devices, with sda names
```

`LC_ALL=C` gives the dates and times the program reads. On Debian and Ubuntu the files are in `/var/log/sysstat/`. Newer sysstat collects some sections only when `sadc` is told to (`-S XALL` in `SADC_OPTIONS` of `/etc/sysconfig/sysstat`): per-CPU interrupts, file systems, power (fans, temperatures), IPv6 and PSI.

The reports are plain text, so the Windows build reads reports copied from Linux hosts the same way.

## Sections

Sections are found by their column names, so a sysstat version with new columns or a new section still reads; a section this program does not know is kept under its first column name (`other_<column>`).

| id | Title | sar option | Items |
|---|---|---|---|
| `cpu` | CPU utilization | `-u`, `-P ALL` | CPU (`all`, `0`, `1` ...) |
| `pcsw` | Task creation and context switches | `-w` | |
| `intr` | Interrupts | `-I` | INTR (`sum`, numbers) |
| `intr_cpu` | Interrupts per CPU | `-I ALL -P ALL` | CPU |
| `swap` | Swapping | `-W` | |
| `paging` | Paging | `-B` | |
| `io` | I/O and transfer rates | `-b` | |
| `memstat` | Memory page changes | `-R` (older sysstat) | |
| `mem` | Memory | `-r` | |
| `swpspace` | Swap space | `-S` | |
| `huge` | Huge pages | `-H` | |
| `ktables` | Kernel tables | `-v` | |
| `queue` | Run queue and load average | `-q` | |
| `psi_cpu`, `psi_io`, `psi_mem` | CPU, I/O and memory pressure (PSI) | `-q CPU`, `-q IO`, `-q MEM` | |
| `tty` | Serial lines | `-y` | TTY |
| `disk` | Block devices | `-d` | DEV |
| `net_dev`, `net_edev` | Network interfaces, interface errors | `-n DEV`, `-n EDEV` | IFACE |
| `nfs`, `nfsd` | NFS client, NFS server | `-n NFS`, `-n NFSD` | |
| `sock`, `sock6` | Sockets (IPv4, IPv6) | `-n SOCK`, `-n SOCK6` | |
| `ip`, `eip`, `ip6`, `eip6` | IPv4 and IPv6 traffic and errors | `-n IP`, `-n EIP`, `-n IP6`, `-n EIP6` | |
| `icmp`, `eicmp`, `icmp6`, `eicmp6` | ICMPv4 and ICMPv6 traffic and errors | `-n ICMP` ... | |
| `tcp`, `etcp` | TCPv4 traffic and errors | `-n TCP`, `-n ETCP` | |
| `udp`, `udp6` | UDPv4, UDPv6 traffic | `-n UDP`, `-n UDP6` | |
| `softnet` | Software network processing | `-n SOFT` | CPU |
| `fchost` | Fibre Channel hosts | `-n FC` | FCHOST |
| `cpufreq`, `cpuwfreq` | CPU frequency, weighted frequency | `-m CPU`, `-m FREQ` | CPU |
| `fan`, `temp`, `in` | Fans, temperatures, voltage inputs | `-m FAN`, `-m TEMP`, `-m IN` | sensor and device |
| `usb` | USB devices | `-m USB` | BUS |
| `fs` | File systems | `-F` | FILESYSTEM |

## Summary output

For each section: the whole-system item (CPU `all`, interrupts `sum`, or the only item) with the average, lowest and highest value of each column and the time of the highest; then, for sections with items, the average of each column per item and the highest value of each column with its item and time. A column that never changed has `-` for the time.

```
== cpu: CPU utilization (sar -u, -P ALL), 9 CPU, 14 samples
  CPU all:
  column            average       lowest      highest  highest at
  %usr                  10.76         3.49        21.66  2026-11-20 22:45:37
  %iowait                0.68         0.00         3.96  2026-11-20 22:45:51
  %idle                 70.22        58.07        80.83  2026-11-20 22:45:55
  averages per CPU:
  CPU                     %usr       %nice        %sys     %iowait ...
  0                       6.51       17.48        3.07        0.18 ...
  highest:
  %usr                  83.92  1 at 2026-11-20 22:45:37

== disk: Block devices (sar -d), 13 DEV, 14 samples
  averages per DEV:
  DEV                      tps       rkB/s       wkB/s ...       %util
  sda                   132.32      199.86     7836.27 ...        4.22
  highest:
  wkB/s              40374.50  sda at 2026-11-20 22:45:45
```

(Some rows are left out above.)

## CSV output

One row per value: `host,time,section,item,metric,value`. The item is empty for sections without items. Times are `YYYY-MM-DD HH:MM:SS` (24-hour).

```
host,time,section,item,metric,value
node1.example.com,2026-11-20 22:45:31,cpu,all,%usr,18.02
node1.example.com,2026-11-20 22:45:31,mem,,kbmemfree,6577840
node1.example.com,2026-11-20 22:45:31,disk,sda,%util,1.45
```

A month of 10-minute samples of a 4-CPU host with a few disks is about a million rows (50 MB).

## Charts and reports

```
sar_stats --csv sar_19.txt > host1_19.csv
dr_plot.py -o host1_19.pdf host1_19.csv                        # a PDF page per section and item
dr_plot.py --section=cpu,mem,disk -o host1.png host1_19.csv    # host1_cpu_all.png, host1_mem.png, host1_disk_sda.png ...
dr_plot.py --section=net_dev --item=eth0 -o eth0.png host1_19.csv
dr_report.py -o host1_19.html --title "host1 2026-11-19" host1_19.csv
```

Each chart splits the columns of a section by unit (percent, per second, kB and MB sizes, load, other counts) into stacked panels; CPU use is drawn stacked like kSar (user, nice, system, iowait, steal, irq, soft). For sections with items, the whole-system item and the busiest items are drawn (`--items`, default 6 in dr_plot.py and 4 in dr_report.py); idle items (all zeros) are left out; `--item` draws the items named. See [Plots and reports](plots-and-reports.md).

## Examples

Linux:

```
LC_ALL=C sar -A -f /var/log/sa/sa$(date -d yesterday +%d) > sar_yesterday.txt
sar_stats --list sar_yesterday.txt
sar_stats sar_yesterday.txt | less
sar_stats --section=mem,swpspace,paging sar_yesterday.txt
sar_stats --section=disk --item=sda,dm-0 sar_yesterday.txt
sar_stats --csv sar_yesterday.txt > host1.csv && dr_report.py -o host1.html host1.csv
sar_stats --submit --now sar_yesterday.txt                     # to the dashboard
```

A month of reports from a host, then one report:

```
for f in /var/log/sa/sa[0-9][0-9]; do LC_ALL=C sar -A -f $f; done > sar_month.txt
sar_stats --csv sar_month.txt > month.csv
dr_report.py -o month.html --title "host1, the last month" month.csv
```

Windows, with reports copied from Linux hosts:

```
sar_stats --list C:\reports\sar\node1_19.txt
sar_stats --section=cpu,mem,disk C:\reports\sar\node1_19.txt > node1_19_summary.txt
sar_stats --csv C:\reports\sar\node1_19.txt > node1_19.csv
py C:\src\dr_utils\benchmarks\plot\dr_plot.py -o node1_19.pdf node1_19.csv
py C:\src\dr_utils\benchmarks\plot\dr_report.py -o node1_19.html node1_19.csv
```

Short example reports are in `tests/data/sar/` (`sar_A_rhel5.txt`, `sar_A_rhel7.txt`, `sar_A_rhel10.txt`).
