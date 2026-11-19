# netpscan

Part of dr_utils 1.4. This file is made from [docs/network-tools.md](../../docs/network-tools.md#netpscan) by docs/make_docs.py: edit that page, not this file. All the documentation: [docs/](../../docs/README.md), or the searchable HTML in `docs/html/index.html`.

Ranges (`192.0.2`, `192.0.2.7`, `192.0.2.10-40`), services files and the privileges ping needs are on the [Network tools](../../docs/network-tools.md) page.

Scans a list of addresses for open TCP ports.

```
netpscan [switches] iplist_file [services_file]
```

| Option | Meaning |
|---|---|
| `iplist_file` | one IPv4 address or host name per line; `#` starts a comment |
| `-p ports` | scan these ports: `22,80,443` or `1-1024` |
| `-A` | scan every TCP port named in the services file (the default is the named ports below 1024) |
| `-t ms` | connect and ping timeout (default 500) |
| `-s` | skip addresses that do not answer a ping |
| `services_file` | the services file for the port list and names |

Each open port is printed as `port/tcp name aliases # comment`, and each address ends with `n open TCP port(s)`.

Dashboard: `--drsm=fname`, `--submit[=ID]` (default `dru_netpscan`), `--now`, `--drra=path` send the result to DRSM through the reporting agent, see [Dashboard](../../docs/dashboard-hooks.md).

Exit status: 0 when the scan ran, 2 on an error.

An address list (`servers.txt`):

```
# web and database servers
192.0.2.10
192.0.2.11
db.example.com
```

Linux:

```
netpscan servers.txt
netpscan -p 22,80,443,3389 servers.txt
netpscan -s -A -t 300 servers.txt > ports_$(date +%F).txt
```

Windows:

```
netpscan servers.txt
netpscan -p 22,80,443,3389,5985 servers.txt > ports.txt
netpscan -A servers.txt C:\tools\services.txt
```
