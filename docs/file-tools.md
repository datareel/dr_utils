# File tools

Three small tools for scripts and scheduled jobs: is a file stale, purge old files, and show file times. They are based on work from the 2005 file tools and keep their output lines (`YES`, `NOT`, `ERR`, `LST`), so existing scripts keep working.

Ages are measured from the file's modification time. Hours and days can be fractional: `0.5` hours is 30 minutes, `2.5` days is 60 hours.

## file_hours

Says whether a file was last changed `numhours` ago or more.

```
file_hours [-s] filename numhours
```

| Option | Meaning |
|---|---|
| `filename` | the file to check |
| `numhours` | the age in hours: `0.5`, `1`, `2.5`, `36` ... |
| `-s` | exit status 0 for YES, 1 for NOT, 2 for ERR (for `if` tests without reading the output) |

Output, one line:

| Line | Meaning |
|---|---|
| `YES` | the file is `numhours` old or older |
| `NOT` | the file is newer |
| `ERR - <reason>` | the file is missing, is not a file, or has a bad time stamp |

Exit status without `-s`: 0 for YES or NOT, 1 for ERR.

Linux:

```
file_hours /var/spool/feed/latest.dat 2
# alert when the feed file has not changed in 2 hours
if [ "$(file_hours /var/spool/feed/latest.dat 2)" = "YES" ]; then
  echo "feed is stale" | mail -s "feed stale" ops@example.com
fi
# the same with the exit status
if file_hours -s /var/spool/feed/latest.dat 2; then echo "stale"; fi
```

Windows, `cmd.exe`:

```
file_hours C:\spool\feed\latest.dat 0.5
file_hours -s C:\spool\feed\latest.dat 2
if %ERRORLEVEL%==0 echo feed is stale
```

Windows, PowerShell:

```
if ((file_hours C:\spool\feed\latest.dat 2) -eq 'YES') { Write-Warning 'feed is stale' }
file_hours -s C:\spool\feed\latest.dat 2; if ($LASTEXITCODE -eq 2) { 'cannot check the file' }
```

## file_purge

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

## filetimes

Shows a file's access, modification and creation times in local time and GMT, and its age. With several files it can print just the newest or the oldest.

```
filetimes [-n | -o] file [file ...]
```

| Option | Meaning |
|---|---|
| `-n` | print only the name of the newest file (by modification time) |
| `-o` | print only the name of the oldest file |

Without `-n` or `-o` each file gets a block like this (on Linux the third time is the inode change time, `File change time (ctime)`; on Windows it is `File creation time`):

```
Reading time stamps for: a.log
File access time: 11/19/2026  01:11 PM
File access GMT : Thu, 19 Nov 2026 18:11:41 GMT
File modification time: 11/19/2026  01:11 PM
File modification GMT : Thu, 19 Nov 2026 18:11:41 GMT
File change time (ctime): 11/19/2026  01:11 PM
File change GMT  (ctime): Thu, 19 Nov 2026 18:11:41 GMT
File is 0 second(s) old
```

Exit status 1 if any file cannot be read, or on a usage error.

Linux:

```
filetimes /etc/hosts
filetimes -n build/*.tar.gz          # the newest build
latest=$(filetimes -n /data/feed/*.dat) && process_feed "$latest"
filetimes -o /var/log/myapp/*.log    # the oldest log
```

Windows, `cmd.exe`:

```
filetimes C:\Windows\System32\drivers\etc\hosts
filetimes -n C:\builds\*.zip
for /f "delims=" %f in ('filetimes -n C:\data\feed\*.dat') do set LATEST=%f
```

(In a `.cmd` file write `%%f` for `%f`.)

Windows, PowerShell:

```
$latest = filetimes -n C:\data\feed\*.dat
filetimes -o C:\logs\myapp\*.log
```
