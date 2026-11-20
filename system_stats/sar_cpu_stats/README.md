# sar_cpu_stats

Part of dr_utils 1.5. This file is made from [docs/sar-cpu-stats.md](../../docs/sar-cpu-stats.md) by docs/make_docs.py: edit that page, not this file. All the documentation: [docs/](../../docs/README.md), or the searchable HTML in `docs/html/index.html`.

Reads sar text reports and prints CPU use: averages and peaks, steal and iowait spikes, gaps in the data and impossible samples. With `--csv` it writes every sample for plotting. It is based on work from the 2018 system_stats_analysis_tool `read_cpu_stats`.

```
sar_cpu_stats [args] sar_file [sar_file ...]
```

| Option | Meaning |
|---|---|
| `--csv` | write every sample as CSV instead of the summary |
| `--all-cpus` | with `--csv`: rows for each CPU too, not only `all` |
| `--steal=PCT` | steal spike threshold (default 10) |
| `--iowait=PCT` | iowait spike threshold (default 20) |
| `--list=N` | spikes, gaps and bad samples listed per file (default 20) |
| `--alarm` | exit status 3 when there are spikes, gaps or bad samples |
| `--help`, `--version` | usage, version |
| `--drsm`, `--submit`, `--now`, `--drra`, `--drra-config` | dashboard output, see [Dashboard](../../docs/dashboard-hooks.md) |

Exit status: 0; 1 on an error or no CPU samples; 3 with `--alarm` when something was found.

## Input

sar text reports from sysstat on RHEL 5 to RHEL 10 (and other distributions): one day or many days in a file, 12- or 24-hour times, `%user` or `%usr`, with or without the newer columns (`%steal`, `%irq`, `%soft`, `%guest`, `%gnice`). Make one with:

```
LC_ALL=C sar -u -f /var/log/sa/sa19 > sar_19.txt          # all CPUs, one day
LC_ALL=C sar -P ALL -f /var/log/sa/sa19 > sar_19_cpus.txt # each CPU too
LC_ALL=C sar -A -f /var/log/sa/sa19 > sar_19_all.txt      # a whole report; only the CPU part is read
```

(On Debian and Ubuntu the files are in `/var/log/sysstat/`.) `LC_ALL=C` gives the dates and times the tool reads.

The reports are plain text, so the Windows build reads reports copied from Linux hosts the same way.

## Summary output

For the `all` CPU rows of each file:

- the average and peak of user, nice, system, iowait, steal, irq, soft, guest, gnice, and the lowest idle, with the peak times;
- steal and iowait spikes over the thresholds;
- gaps: a missing run of samples inside a day, or a day whose samples stop early while later days follow (a host or VM that stopped writing);
- bad samples: a percent over 100, or parts that do not add up to about 100. A VM the hypervisor stopped can write a steal of 241%. Bad samples are listed and left out of the averages.

```
Reading sar_steal_spike.txt
  bad sample at 2018-09-24 00:00:00: steal 241.45, iowait 23.50, the parts add up to 438.4%
  gap: samples stop at 2018-09-24 00:10:01
Host: node1.example.com, kernel 2.6.32-696.28.1.el6.x86_64
Days: 2 (2018-09-24 to 2018-09-25), 2 samples of all CPUs, every 10 minutes
           average     peak  peak time
  user        5.96     6.61  2018-09-24 00:10:01
  system      8.46     9.93  2018-09-24 00:10:01
  iowait     17.19    18.80  2018-09-24 00:10:01
  steal       0.00     0.00  -
  idle       67.84    64.00  2018-09-24 00:10:01 (lowest)
Steal >= 10.0%: 0, iowait >= 20.0%: 0, gaps: 1, bad samples: 2
```

(Some rows are left out above.)

## CSV output

Columns: `host,time,cpu,user,nice,system,iowait,steal,irq,soft,guest,gnice,idle,sample`. A column the report does not have is empty. `sample` is `ok` or `bad`.

```
host,time,cpu,user,nice,system,iowait,steal,irq,soft,guest,gnice,idle,sample
node1.example.com,2026-11-19 04:10:24,all,17.02,11.14,4.01,19.40,0.00,,,,,48.44,ok
```

`dr_plot.py` and `dr_report.py` plot this CSV (see [Plots and reports](../../docs/plots-and-reports.md)).

## Examples

Linux:

```
LC_ALL=C sar -u -f /var/log/sa/sa$(date -d yesterday +%d) > sar_yesterday.txt
sar_cpu_stats sar_yesterday.txt
sar_cpu_stats --steal=5 --iowait=30 --list=50 sar_*.txt
sar_cpu_stats --csv sar_yesterday.txt > cpu.csv && dr_plot.py -o cpu.png cpu.csv
sar_cpu_stats --csv --all-cpus sar_19_cpus.txt > cpus.csv && dr_plot.py --cpu 3 -o cpu3.png cpus.csv
```

A cron check that mails only when something is wrong:

```
30 0 * * * root f=/tmp/sar_$$.txt; LC_ALL=C sar -u -f /var/log/sa/sa$(date -d yesterday +\%d) > $f; /usr/local/bin/sar_cpu_stats --alarm $f > $f.out || mail -s "sar CPU alarm" ops@example.com < $f.out; rm -f $f $f.out
```

(To send the result to the dashboard every day instead, use the ready-made job in [Dashboard](../../docs/dashboard-hooks.md#daily-sar-cpu-use-linux).)

Windows, with reports copied from Linux hosts:

```
sar_cpu_stats C:\reports\sar\node1_*.txt
sar_cpu_stats --csv C:\reports\sar\node1_19.txt > node1.csv
py C:\src\dr_utils\benchmarks\plot\dr_plot.py -o node1.png node1.csv
sar_cpu_stats --alarm C:\reports\sar\*.txt
if %ERRORLEVEL%==3 echo spikes, gaps or bad samples found
```

Short example reports are in `tests/data/sar/` (RHEL 5, RHEL 7 with AM/PM times, RHEL 10, a gap, a steal spike).
