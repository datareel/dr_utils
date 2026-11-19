# filetimes

Part of dr_utils 1.4. This file is made from [docs/file-tools.md](../../docs/file-tools.md#filetimes) by docs/make_docs.py: edit that page, not this file. All the documentation: [docs/](../../docs/README.md), or the searchable HTML in `docs/html/index.html`.

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
