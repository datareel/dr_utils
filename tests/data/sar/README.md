# sar test reports

Parts of sar text reports used by the sar_cpu_stats and sar_stats tests. The host names were changed to example.com names before the reports were kept.

| File | What it shows |
|---|---|
| `sar_rhel5_partial.txt` | RHEL 5 sysstat: `%user` and `%system` names, no `%irq`/`%soft`/`%guest`, and the `proc/s` section before the CPU section |
| `sar_rhel7_ampm.txt` | RHEL 7 sysstat with 12-hour AM/PM times and `sar -P ALL` rows for 6 CPUs |
| `sar_rhel10.txt` | RHEL 10 sysstat (`sar -P ALL 1 3` and `sar -u 1 2`, 1-second samples) |
| `sar_gap.txt` | the RHEL 7 report with 70 minutes of samples removed |
| `sar_steal_spike.txt` | a VM the hypervisor stopped: samples stop after 00:10, then rows of zeros and a row with steal 241.45% and soft 133.21% |
| `sar_A_rhel5.txt` | a full RHEL 5 report (sar -A), the first two hours of two days: `proc/s` and `cswch/s` as two sections, `INTR`, per-CPU `i000/s` interrupts, `TTY` |
| `sar_A_rhel7.txt` | a full RHEL 7 report, the first two hours of one day: huge pages, `DEV` block devices, network, NFS, sockets |
| `sar_A_rhel10.txt` | a full RHEL 10 report (sysstat 12.7, `sadc -S XALL`, 2-second samples): 35 sections, among them IPv6, softnet, CPU frequency, fans, temperatures, USB and file systems. Device and USB names were made generic. |
