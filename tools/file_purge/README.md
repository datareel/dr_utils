# file_purge

Part of dr_utils 1.5. This file is made from [docs/file-tools.md](../../docs/file-tools.md#file_purge) by docs/make_docs.py: edit that page, not this file. All the documentation: [docs/](../../docs/README.md), or the searchable HTML in `docs/html/index.html`.

Deletes or moves files that are a number of hours or days old, or lists their ages.

```
file_purge [switches] file [file ...]
```

| Switch | Meaning |
|---|---|
| `-h<n>` | delete each file `n` hours old or older (`-h7`, `-h0.5`) |
| `-d<n>` | delete each file `n` days old or older (`-d30`, `-d2.5`) |
| `-m"dir"` | with `-h` or `-d`: move the file to `dir` instead of deleting it. Across file systems it is a copy and delete. A file of the same name in `dir` is never overwritten |
| `-e` | delete the file if it exists, with no age check |
| `-l` | list each file's age in hours and days, change nothing |
| `-n` | dry run: print what would be done (`DRY - ...`), change nothing |
| `-v` | verbose output |
| `-?` | the usage text |

`file` names can have wildcards (`*.log`), on Windows too. Directories are never purged, even when a wildcard matches them. One of `-h`, `-d`, `-e` or `-l` is needed.

Each file gets one line:

| Line | Meaning |
|---|---|
| `YES - name purged (n) hours` | deleted, or `moved ... to dir` |
| `NOT - name not purged (n) hours` | too new, or the delete or move failed (`error deleting file`), or the target name already exists in the `-m` folder |
| `ERR - name <reason>` | missing, not a file, bad time stamp |
| `LST - name (n) hours (n) days` | the `-l` listing |
| `DRY - name would be purged ...` | the `-n` dry run |

Dashboard: `--drsm=fname`, `--submit[=ID]` (default `dru_file_purge`), `--now`, `--drra=path` send the result to DRSM through the reporting agent, see [Dashboard](../../docs/dashboard-hooks.md).

Exit status: 0 when the files were checked (look at the lines for what happened to each), 1 on a usage error (bad switch, no file name, missing `-m` folder).

Run with `-n` first, every time.

Linux:

```
file_purge -l /var/log/myapp/*.log            # ages only
file_purge -n -d30 /var/log/myapp/*.log       # what a 30 day purge would delete
file_purge -d30 /var/log/myapp/*.log          # delete them
file_purge -n -m"/archive/myapp" -h12 /var/spool/myapp/*.dat
file_purge -m"/archive/myapp" -h12 /var/spool/myapp/*.dat
file_purge -e /var/run/myapp/stale.lock       # delete if it exists
```

A daily cron line (`/etc/cron.d/myapp_purge`):

```
15 3 * * * root /usr/local/bin/file_purge -d30 /var/log/myapp/*.log > /var/log/myapp_purge.log 2>&1
```

Windows, `cmd.exe`:

```
file_purge -l C:\logs\myapp\*.log
file_purge -n -d30 C:\logs\myapp\*.log
file_purge -d30 C:\logs\myapp\*.log
file_purge -n -m"D:\archive\myapp" -h12 C:\spool\myapp\*.dat
file_purge -m"D:\archive\myapp" -h12 C:\spool\myapp\*.dat
```

Windows, PowerShell (quote the whole `-m` switch so PowerShell passes it as one argument):

```
file_purge -n '-mD:\archive\my app' -h12 C:\spool\myapp\*.dat
```

A daily scheduled task (administrator prompt):

```
schtasks /Create /TN "myapp log purge" /SC DAILY /ST 03:15 /RU SYSTEM /TR "\"C:\Program Files\dr_utils\file_purge.exe\" -d30 C:\logs\myapp\*.log"
```
