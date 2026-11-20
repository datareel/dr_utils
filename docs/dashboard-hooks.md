# Dashboard: reporting agent hooks

The benchmarks, the file and network tools, `sar_cpu_stats` and `dr_report.py` can send their results to the DataReel System Monitor (DRSM) dashboard through the DataReel Reporting Agent for Linux (1.0-beta2 and later) or Windows (3.3-beta2 and later). They use the agents' **hook uploads**:

1. The program writes its results as DRSM lines (or, for `dr_report.py`, the HTML report).
2. It hands the file to the agent: `drra submit --hook <id> file` on Linux, `drra_svc.exe submit --hook <id> file` on Windows.
3. The agent sends it, signed, to `hook.php` on the report server, with its next report or, with `--now`, within seconds.
4. A DRSM upload plugin (a manifest from `hooks/drsm/uploads/`) shows it on the host's page, with alert rules and trends.

Nothing in the agents or in DRSM changes; dr_utils only adds the hook files for them. The reference for the files is [hooks/README.md](../hooks/README.md).

## Options

Every benchmark except the three servers (`tcpserv`, `udpserv`, `mcserv`), every file and network tool, and `sar_cpu_stats` take:

| Option | Meaning |
|---|---|
| `--drsm=fname` | write the DRSM lines to a file (to look at them, or to send later by hand) |
| `--submit[=ID]` | hand the results to the agent, upload hook ID (default `dru_<program>`, for example `dru_cputest`). An ID is `a-z`, `0-9` and `_` |
| `--now` | with `--submit`: the agent sends within seconds, not with its next report |
| `--drra=path` | the agent program. Linux: `drra` in the PATH, then `/usr/sbin/drra`. Windows: `%ProgramFiles%\DataReel\ReportingAgent\drra_svc.exe` |
| `--drra-config=ini` | Linux only: the agent settings file (`drra -c ini`), for tests. A Windows test copy of the agent reads the `drra.ini` next to it |

`dr_report.py` takes `--submit [ID]` (default `dru_report`), `--now`, `--drra` and `--drra-config` the same way.

- With both `--drsm` and `--submit`, the `--drsm` file is the one submitted. With `--submit` alone the lines go through a private temporary folder that is removed afterwards.
- The exit status is 1 when the agent refuses the file or cannot be run, so a cron job or scheduled task sees it.
- `drra submit` needs root or the `drra-upload` group on Linux, and an administrator on Windows.

## What each program sends

| Program | Hook ID | Values for rules and trends | Table |
|---|---|---|---|
| `cputest` | `dru_cputest` | `total` (MB/s), `elapsed`, `threads`, `buf_size` | seconds per thread |
| `disktest` | `dru_disktest` | `total` (seconds), `insert_time`, `search_time`, `delete_time`, `reinsert_time`, `num_inserts` | seconds and ms per key per phase |
| `memtest` | `dru_memtest` | `total` (seconds), `threads`, `num_inserts` | seconds per phase |
| `flops` | `dru_flops` | `total` (GFLOPS), `elapsed`, `threads`, `precision`, `mode` | seconds per thread |
| `tcptest` | `dru_tcptest` | `total` (MB/s), `clients`, `bytes`, `direction` | MB/s per client |
| `udptest` | `dru_udptest` | `total` (MB/s), `loss_max_pct`, `clients`, `rate_limit` | MB/s and % lost per client |
| `mctest` | `dru_mctest` | `total` (MB/s per receiver), `loss_max_pct`, `receivers` | MB/s and % lost per receiver |
| `sar_cpu_stats` | `dru_sar_cpu_stats` | `busy_avg`, `<field>_avg` and `<field>_peak`, `idle_min`, `steal_spikes`, `iowait_spikes`, `gaps`, `bad_samples`, `files_failed` | one row per sar file |
| `sar_stats` | `dru_sar_stats` | `<section>_<column>_avg` and `_max` of the whole system (`cpu_pct_idle_avg`, `mem_pct_memused_max`, `queue_ldavg_1_max` ...), `<section>_<column>_max` over the items of the other sections (`disk_pct_util_max`, `net_dev_rxkb_s_max`, `fs_pct_fsused_max` ...), `<section>_items`, `sections`, `files_failed` | one row per sar section |
| `file_hours` | `dru_file_hours` | `stale` (1 when the file is the limit old or older), `age_hours`, `limit_hours`, `result` (YES, NOT, ERR), `file` | the file |
| `file_purge` | `dru_file_purge` | `purged`, `would_purge` (`-n`), `kept`, `listed` (`-l`), `errors`, `files`, `mode`, `dry_run` | a row per file: result and details |
| `filetimes` | `dru_filetimes` | `files`, `errors`, `newest_file`, `newest_age_hours`, `oldest_file`, `oldest_age_hours` | modification time and age per file |
| `internet-test` | `dru_internet_test` | `passed` (1 or 0), `tries`, `seconds`, `result`, `server`, `port` | |
| `pingscan` | `dru_pingscan` | `up`, `down`, `tried`, `range` | address, name, status, reply ms (`-u`: hosts up only) |
| `dnsscan` | `dru_dnsscan` | `named`, `tried`, `fwd_ok` and `fwd_mismatch` (`-f`), `up` (`-p`) | address, name, forward check, ping |
| `portscan` | `dru_portscan` | `open`, `ports`, `ping`, `ping_ms`, `host`, `address`, and `port_<n>` (`open` or `closed`, 64 ports or fewer) | open ports (every port when there are 16 or fewer) |
| `netpscan` | `dru_netpscan` | `open_ports`, `scanned`, `up`, `skipped` (`-s`), `not_resolved`, `ports_per_host` | a row per address: ping, open count, port list |
| `arpscan` | `dru_arpscan` | `found`, `tried`, `range` | address, name, MAC address, vendor |
| `getmac` | `dru_getmac` | `found` (1 or 0), `mac`, `ping`, `host`, `address` | the host |
| `dr_report.py` | `dru_report` | the whole HTML report, charts included | |

Each result also has an INFO line with the main result and settings, and the values `benchmark` (or `program`), `version`, `build` and `collected`. What `cputest --num-threads=2 --drsm=cpu.drsm` writes:

```
INFO - CPU Speed Benchmark (cputest 1.5): zlib compress + uncompress rate 40.8762 MB/s, threads=2 buf_size=10000000 repeat=1
DETAIL benchmark=cputest
DETAIL version=1.5
DETAIL build=GCC
DETAIL threads=2
DETAIL buf_size=10000000
DETAIL repeat=1
DETAIL total=40.8762
DETAIL total_unit=MB/s
DETAIL elapsed=0.244641
DETAIL collected=2026-11-19T13:11:41-0500
UPLOAD columns|Thread|seconds
UPLOAD row|Thread 0|0.243874
UPLOAD row|Thread 1|0.229364
```

## Setting it up

### 1. Monitor host (DRSM)

Copy the manifests you want to DRSM's upload plugins:

```
cp -r hooks/drsm/uploads/dru_* /path/to/drsm/plugins/uploads/
```

The report server's `hook.php` reads them from `[hooks] manifest_dir` in its `server.ini` (the same folder on a DRSM install). Each host needs `UPLOADS_DIR` and `UPLOADS=auto` (or a list of hook IDs) in its DRSM host file. This is the same setup as every other upload hook; see "Hook uploads" in DRSM's PLUGINS.md. For `dru_report`, the report server's `[hooks] max_bytes` must allow the report size (reports with many charts can be several MB), and the dashboard needs `$UPLOADSdir` for the html display.

### 2. Agent host, Linux

The installer is a dry run unless `--apply` is given. It copies `hooks/uploads.d/<id>.ini` to `/etc/drra/uploads.d/`.

```
sudo hooks/linux/install_hooks.sh all                     # what it would do
sudo hooks/linux/install_hooks.sh --apply all             # install every hook file
sudo hooks/linux/install_hooks.sh --apply cputest disktest   # only these
sudo hooks/linux/install_hooks.sh --help
```

A user who runs the benchmarks without sudo needs the `drra-upload` group:

```
sudo usermod -aG drra-upload benchuser     # then log in again
```

### 3. Agent host, Windows

From an administrator PowerShell or `cmd.exe` prompt, in the dr_utils folder. A dry run unless `-Apply` is given. The hook files go to `C:\ProgramData\DataReel\ReportingAgent\uploads.d\`.

```
powershell -ExecutionPolicy Bypass -File hooks\windows\install_hooks.ps1 all
powershell -ExecutionPolicy Bypass -File hooks\windows\install_hooks.ps1 all -Apply
powershell -ExecutionPolicy Bypass -File hooks\windows\install_hooks.ps1 cputest disktest -Apply
```

### 4. Send a result and check it

Linux:

```
cputest --num-threads=8 --submit --now
sudo drra uploads                         # the state of each hook
```

Windows (administrator prompt):

```
cputest --num-threads=8 --submit --now
"%ProgramFiles%\DataReel\ReportingAgent\drra_svc.exe" uploads
```

The result is on the host's DRSM page after the next DRSM run.

## Examples

Linux:

```
cputest --num-threads=$(nproc) --submit --now
disktest --file=/data/testfile.btx --submit
flops --dp --num-threads=$(nproc) --submit
tcptest -n4 -s1G --submit 192.0.2.10
udptest -s100M -r500M --submit 192.0.2.10
mctest -i192.0.2.21 --submit
sar_cpu_stats --submit --now sar_yesterday.txt
dr_report.py -o host_report.html --submit results/

# file and network tools
file_hours /var/spool/feed/latest.dat 2 --submit
file_purge -d30 /var/log/myapp/*.log --submit
filetimes -n /data/incoming/*.csv --submit
internet-test -c 3 -w 10 www.example.com --submit
pingscan -u 192.0.2 --submit
dnsscan -f -p 192.0.2 --submit
portscan -n www.example.com 22,80,443 --submit
netpscan -p 22,80,443 servers.txt --submit
arpscan -v vendors.txt 192.0.2 --submit
getmac printer.example.com --submit

# look first, send later by hand
cputest --drsm=cpu.drsm
cat cpu.drsm
sudo drra submit --hook dru_cputest --now cpu.drsm

# a hook under another ID (its .ini and manifest must exist)
disktest --file=/san/testfile.btx --submit=dru_disktest_san

# a test agent build
cputest --submit --drra=/path/to/lra/drra --drra-config=/path/to/test/drra.ini
```

Windows (administrator prompt):

```
cputest --num-threads=%NUMBER_OF_PROCESSORS% --submit --now
disktest --file=D:\bench\testfile.btx --submit
tcptest -n4 -s1G --submit 192.0.2.10
sar_cpu_stats --submit C:\reports\sar\node1_19.txt
py benchmarks\plot\dr_report.py -o host_report.html --submit results
file_hours D:\feeds\latest.dat 2 --submit
file_purge -d30 C:\logs\myapp\*.log --submit
internet-test -c 3 -w 10 www.example.com --submit
pingscan -u 192.0.2 --submit
portscan -n www.example.com 443,3389 --submit

cputest --drsm=cpu.drsm
type cpu.drsm
"%ProgramFiles%\DataReel\ReportingAgent\drra_svc.exe" submit --hook dru_cputest --now cpu.drsm

cputest --submit --drra=C:\test\agent\drra_svc.exe
```

A weekly scheduled task on Windows:

```
schtasks /Create /TN "dr_utils cputest" /SC WEEKLY /D SUN /ST 02:00 /RU SYSTEM /TR "\"C:\Program Files\dr_utils\cputest.exe\" --num-threads=4 --submit"
```

A connectivity check every 15 minutes on Windows:

```
schtasks /Create /TN "dr_utils internet-test" /SC MINUTE /MO 15 /RU SYSTEM /TR "\"C:\Program Files\dr_utils\internet-test.exe\" -c 3 -w 10 www.example.com --submit"
```

A weekly cron job on Linux (`/etc/cron.d/dru_cputest`):

```
0 2 * * 0 root /usr/local/bin/cputest --num-threads=4 --submit > /dev/null
```

A stale feed check every 30 minutes and a nightly port check (`/etc/cron.d/dru_checks`):

```
*/30 * * * * root /usr/local/bin/file_hours /var/spool/feed/latest.dat 2 --submit > /dev/null
15 1 * * * root /usr/local/bin/portscan -n www.example.com 22,80,443 --submit > /dev/null
```

For a scheduled benchmark, set `uploads_dru_cputest_MAX_AGE_HOURS` on the host in DRSM (see below) so a stopped job shows.

## Daily sar CPU use (Linux)

`hooks/linux/dru_sar_cpu_stats.sh` makes the sar text report of one day with `LC_ALL=C sar -u -f /var/log/sa/saDD` and runs `sar_cpu_stats --submit` on it.

```
dru_sar_cpu_stats.sh [--day=yesterday|today|YYYY-MM-DD] [--sa-dir=/var/log/sa] [--now] [--print] [other sar_cpu_stats options]
```

| Option | Meaning |
|---|---|
| `--day=` | `yesterday` (default), `today` or a date |
| `--sa-dir=` | the sysstat folder (default `/var/log/sa`) |
| `--now` | the agent sends within seconds |
| `--print` | print the DRSM lines instead of submitting them |
| other options | go to `sar_cpu_stats`, for example `--iowait=30` |

`sar_cpu_stats` is found next to the script, then in the PATH, or set `SAR_CPU_STATS=/path/to/sar_cpu_stats`. It needs sysstat.

```
hooks/linux/dru_sar_cpu_stats.sh --print
sudo hooks/linux/dru_sar_cpu_stats.sh --day=2026-11-18 --now --steal=5
sudo hooks/linux/install_hooks.sh --sar-cron              # dry run
sudo hooks/linux/install_hooks.sh --sar-cron --apply
```

`--sar-cron --apply` installs `bin/sar_cpu_stats` and the script in `/usr/local/libexec/dr_utils/`, the `dru_sar_cpu_stats` hook file, and `/etc/cron.d/dru_sar_cpu_stats`, which runs at 00:20 every day:

```
20 0 * * * root /usr/local/libexec/dr_utils/dru_sar_cpu_stats.sh > /dev/null
```

The manifest warns when the data is more than 30 hours old, so a stopped cron job shows.

### Every sar section

`hooks/linux/dru_sar_stats.sh` takes the same options, makes the whole report of the day (`LC_ALL=C sar -A -f /var/log/sa/saDD`) and runs `sar_stats --submit` (hook `dru_sar_stats`). The dashboard shows a row per sar section and keeps values such as `cpu_pct_iowait_max`, `mem_pct_memused_max`, `queue_ldavg_1_max`, `disk_pct_util_max` and `fs_pct_fsused_max` for rules and trends.

```
hooks/linux/dru_sar_stats.sh --print
sudo hooks/linux/install_hooks.sh --sar-all-cron                 # dry run
sudo hooks/linux/install_hooks.sh --sar-all-cron --apply         # /etc/cron.d/dru_sar_stats, 00:25 every day
```

Limits depend on the host, so set them in its DRSM host file, for example:

```
uploads_dru_sar_stats_WARN_IF="cpu_pct_iowait_max > 50 or swpspace_pct_swpused_max > 50 or disk_pct_util_max > 90 or fs_pct_fsused_max > 90"
```

## Alert rules

The manifests have rules only where a limit means the same thing everywhere:

| Hook | Rule |
|---|---|
| `dru_udptest`, `dru_mctest` | warning over 1% loss, error over 10% |
| `dru_sar_cpu_stats` | warning on steal or iowait spikes, error when a file cannot be read |
| `dru_file_hours` | warning when the file is stale (`stale == 1`) |
| `dru_file_purge`, `dru_filetimes` | warning when a file cannot be read, deleted or moved (`errors > 0`) |
| `dru_internet_test` | error when the test failed (`passed == 0`) |
| `dru_dnsscan` | warning on a forward mismatch (`fwd_mismatch > 0`) |
| `dru_netpscan` | warning when a name in the list does not resolve |
| `dru_getmac` | warning when the host has no ARP entry (`found == 0`) |

A tool that cannot run (no ICMP socket, a missing file for `file_hours`) sends an ERROR line with its message. Benchmark speeds, host counts and open ports depend on the machine and network, so set limits per host in its DRSM host file:

```
uploads_dru_cputest_WARN_IF="total < 50"
uploads_dru_disktest_WARN_IF="total > 60"
uploads_dru_sar_cpu_stats_WARN_IF="steal_spikes > 0 or iowait_spikes > 0 or gaps > 2"
uploads_dru_portscan_CRIT_IF="port_443 != open"
uploads_dru_portscan_WARN_IF="port_23 == open"
uploads_dru_pingscan_WARN_IF="up < 12"
uploads_dru_filetimes_WARN_IF="errors > 0 or newest_age_hours > 2"
uploads_dru_internet_test_MAX_AGE_HOURS=2
uploads_dru_cputest_MAX_AGE_HOURS=26
```

Every value in the table above can be in a rule, and the `METRICS` list of each manifest is kept for the trends page as `<hook id>_<value>` (for example `dru_cputest_total`).

## Notes

- The dashboard files a result under the host whose agent sent it. A sar report or results from another host show under the sending host (the sar host name is in the `sar_host` value and the table).
- A tool run with the wrong arguments (its usage message) sends nothing.
- `file_hours` checks one file per hook ID. For several files, give each its own ID (`--submit=dru_feed_a`) with a copy of the manifest folder of that name (`NAME=dru_feed_a`) and of the `.ini` (`id=dru_feed_a`).
- `tcptest`, `udptest` and `mctest` send from the client or receiver side; the servers have no hook output.

## Testing the whole path

`tests/hooks_e2e_test.sh` runs the programs, a real agent, its `hook.php` on a local `php -S` server and the DRSM upload probe, all on one Linux machine:

```
LRA_DIR=/path/to/built/linux_reporting_agent DRSM_DIR=/path/to/drsm tests/hooks_e2e_test.sh
```

It needs dr_utils built (`./build.sh`), the agent built in `LRA_DIR` (`make`), and PHP.
