# Dashboard hooks

The dr_utils benchmarks, `sar_cpu_stats` and `dr_report.py` can send their results to the DataReel System Monitor (DRSM) dashboard through the DataReel Reporting Agent for Linux (1.0-beta2 and later) or Windows (3.3-beta2 and later). They use the agents' **hook uploads**: the program hands a file to the agent (`drra submit` or `drra_svc.exe submit`), the agent sends it, signed, to `hook.php` on the report server, and a DRSM upload plugin (the manifest in this folder) shows it on the host's page, with alert rules and trends.

Nothing in the agents or in DRSM changes: this folder only has the hook files for them.

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
| `sar_cpu_stats` | `dru_sar_cpu_stats` | `busy_avg`, `<field>_avg` and `<field>_peak` (user, system, iowait, steal ...), `idle_min`, `steal_spikes`, `iowait_spikes`, `gaps`, `bad_samples`, `files_failed` | one row per sar file |
| `dr_report.py` | `dru_report` | (the whole HTML report, charts included) | |

Every result also has an INFO line with the main result and settings, and the values `benchmark` (or `program`), `version`, `build` and `collected`. The data format is `drsm-lines` (see "Hook uploads" in DRSM's PLUGINS.md), except `dru_report`, which is `html`.

## Options

Every benchmark and `sar_cpu_stats` take:

| Option | Meaning |
|---|---|
| `--drsm=fname` | Write the DRSM lines to a file (to look at them, or to send later with `drra submit`) |
| `--submit[=ID]` | Hand the results to the agent, upload hook ID (default `dru_<program>`) |
| `--now` | With `--submit`: the agent sends within seconds, not with its next report |
| `--drra=path` | Agent program. Linux: `drra` in the PATH, then `/usr/sbin/drra`. Windows: `%ProgramFiles%\DataReel\ReportingAgent\drra_svc.exe` (an .exe; the program is run directly, not through cmd.exe) |
| `--drra-config=ini` | Linux only: agent settings file (`drra -c ini`), for tests. A Windows test copy of the agent reads the `drra.ini` next to it |

`dr_report.py` takes `--submit [ID]`, `--now`, `--drra` and `--drra-config` the same way (default ID `dru_report`).

The exit status is 1 when the agent refuses the file or cannot be run, so a cron job sees it. `drra submit` needs root or the `drra-upload` group on Linux, and an administrator on Windows. With both `--drsm` and `--submit` the `--drsm` file is the one submitted; with `--submit` alone the lines go through a private temporary folder that is removed afterwards.

```
cputest --num-threads=8 --submit --now
disktest --file=/data/testfile.btx --submit
udptest -s100M -r500M --submit 192.0.2.10
sar_cpu_stats --submit --now sar_today.txt
dr_report.py -o host_report.html --submit results/
```

## Setting it up

1. **Monitor host (DRSM).** Copy the manifests you want to DRSM's upload plugins, for example
   `cp -r hooks/drsm/uploads/dru_* <drsm>/plugins/uploads/`.
   The report server's `hook.php` reads them from `[hooks] manifest_dir` in its `server.ini` (the same folder on a DRSM install), and the host needs `UPLOADS_DIR` and `UPLOADS=auto` (or a list of hook IDs) in its DRSM host file. This is the same setup as every other upload hook; see "Hook uploads" in DRSM's PLUGINS.md. For `dru_report`, the report server's `[hooks] max_bytes` must allow the report size (reports with many charts can be several MB), and the dashboard needs `$UPLOADSdir` for the html display.
2. **Agent host, Linux.** `sudo hooks/linux/install_hooks.sh all` shows what it would do (a dry run); add `--apply` to copy the hook files to `/etc/drra/uploads.d/`. Name some hooks instead of `all` (`cputest disktest`) to install only those.
3. **Agent host, Windows.** From an administrator prompt: `powershell -ExecutionPolicy Bypass -File hooks\windows\install_hooks.ps1 all` (dry run), then again with `-Apply`. The hook files go to `C:\ProgramData\DataReel\ReportingAgent\uploads.d\`.
4. Run a program with `--submit --now`. `drra uploads` (Linux) or `drra_svc.exe uploads` (Windows) shows the hook's state, and the result is on the host's page after the next DRSM run.

## Daily sar CPU use (Linux)

`hooks/linux/dru_sar_cpu_stats.sh` makes the sar text report of one day (yesterday by default) with `LC_ALL=C sar -u -f /var/log/sa/saDD` and runs `sar_cpu_stats --submit` on it. `--print` shows the lines instead of submitting them, `--day=YYYY-MM-DD` picks another day, and other options go to `sar_cpu_stats` (for example `--iowait=30`).

`sudo hooks/linux/install_hooks.sh --sar-cron --apply` installs `bin/sar_cpu_stats` and the script in `/usr/local/libexec/dr_utils/`, the hook file, and `/etc/cron.d/dru_sar_cpu_stats` (00:20 every day). The manifest warns when the data is more than 30 hours old, so a stopped cron job shows.

## Alert rules

The manifests have rules only where a limit means the same thing everywhere: UDP and multicast loss (warning over 1%, error over 10%) and sar steal or iowait spikes. Benchmark speeds depend on the hardware, so set a limit per host in its DRSM host file, for example:

```
uploads_dru_cputest_WARN_IF="total < 50"
uploads_dru_disktest_WARN_IF="total > 60"
uploads_dru_sar_cpu_stats_WARN_IF="steal_spikes > 0 or iowait_spikes > 0 or gaps > 2"
uploads_dru_cputest_MAX_AGE_HOURS=26
```

The benchmarks have no age check by default (they run by hand); set `MAX_AGE_HOURS` on a host that runs one on a schedule. Every value in the table above can be in a rule, and the `METRICS` list of each manifest is kept for the trends page as `<hook id>_<value>` (for example `dru_cputest_total`).

## Notes

- The dashboard files a result under the host whose agent sent it. A sar report or `--datfile` results from another host show under the sending host (the sar host name is in the `sar_host` value and the table).
- `tcptest`, `udptest` and `mctest` send from the client or receiver side; the servers (`tcpserv`, `udpserv`, `mcserv`) have no hook output.
- `tests/hooks_e2e_test.sh` runs the whole path on one Linux machine: the programs, a real agent (`LRA_DIR`, a built agent source tree), its `hook.php` on a local `php -S` server, and the DRSM upload probe (`DRSM_DIR`) on the stored data.

## Files

| Path | What |
|---|---|
| `uploads.d/<id>.ini` | Agent upload hook files (the same on Linux and Windows) |
| `drsm/uploads/<id>/` | DRSM manifests: `plugin.conf` and `probe.sh` (which sources DRSM's `upload_probe.sh`) |
| `linux/install_hooks.sh` | Installs hook files (and the sar cron job) on a Linux agent host, dry run by default |
| `linux/dru_sar_cpu_stats.sh`, `.cron` | Daily sar CPU use to the dashboard |
| `windows/install_hooks.ps1` | Installs hook files on a Windows agent host, dry run by default |
