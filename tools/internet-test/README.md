# internet-test

Part of dr_utils 1.4. This file is made from [docs/network-tools.md](../../docs/network-tools.md#internet-test) by docs/make_docs.py: edit that page, not this file. All the documentation: [docs/](../../docs/README.md), or the searchable HTML in `docs/html/index.html`.

Ranges (`192.0.2`, `192.0.2.7`, `192.0.2.10-40`), services files and the privileges ping needs are on the [Network tools](../../docs/network-tools.md) page.

Waits until a web server answers over HTTP: DNS lookup, TCP connect and an HTTP response header are all checked. Any HTTP status passes (a 404 still proves the path works).

```
internet-test [switches] server
```

| Switch | Meaning |
|---|---|
| `server` | a host name (`www.example.com`), `host:port`, or a URL (`http://www.example.com:8080/`) |
| `-c n` | try `n` times; `0` (the default) tries until it passes |
| `-w sec` | seconds between tries (default 60) |
| `-t ms` | connect timeout in milliseconds (default 5000) |
| `-p port` | HTTP port (default 80) |
| `-q` | print only the final result |

Output: `Internet connectivity test passed (www.example.com, HTTP status 200)`, or `... failed after n tries` with the reason.

Exit status: 0 passed, 1 every try failed, 2 usage error.

Linux:

```
internet-test -c 3 -w 10 www.example.com
internet-test -q -c 10 -w 30 www.example.com && /usr/local/bin/start_sync.sh
internet-test -p 8080 -t 2000 -c 1 intranet.example.com
internet-test -c 1 http://192.0.2.20:8080/
```

A boot job that waits for the network before starting a sync (`internet-test` with `-c 0` waits as long as it takes):

```
@reboot  root  /usr/local/bin/internet-test -q -w 30 www.example.com && /usr/local/bin/start_sync.sh
```

Windows, `cmd.exe`:

```
internet-test -c 3 -w 10 www.example.com
internet-test -q -c 10 -w 30 www.example.com && C:\jobs\start_sync.cmd
```

Windows, PowerShell:

```
internet-test -q -c 5 -w 10 www.example.com
if ($LASTEXITCODE -ne 0) { Write-Warning 'no Internet' }
```
