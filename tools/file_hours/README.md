# file_hours

Part of dr_utils 1.4. This file is made from [docs/file-tools.md](../../docs/file-tools.md#file_hours) by docs/make_docs.py: edit that page, not this file. All the documentation: [docs/](../../docs/README.md), or the searchable HTML in `docs/html/index.html`.

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
