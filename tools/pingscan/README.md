# pingscan

Part of dr_utils 1.4. This file is made from [docs/network-tools.md](../../docs/network-tools.md#pingscan) by docs/make_docs.py: edit that page, not this file. All the documentation: [docs/](../../docs/README.md), or the searchable HTML in `docs/html/index.html`.

Ranges (`192.0.2`, `192.0.2.7`, `192.0.2.10-40`), services files and the privileges ping needs are on the [Network tools](../../docs/network-tools.md) page.

Ping sweep of a range, with the DNS name and reply time of each host.

```
pingscan [-t ms] [-u] range
```

| Switch | Meaning |
|---|---|
| `-t ms` | ping timeout in milliseconds (default 1000) |
| `-u` | list only the hosts that answer |

Each address gets a `Contacting host:` block (`Hostname:`, then `Host is up, reply in n ms` or `Host not responding.`). The output ends with `n of m hosts answered`. One host that fails does not stop the sweep.

Dashboard: `--drsm=fname`, `--submit[=ID]` (default `dru_pingscan`), `--now`, `--drra=path` send the result to DRSM through the reporting agent, see [Dashboard](../../docs/dashboard-hooks.md).

Exit status: 0 if any host answered, 1 if none did, 2 on an error.

Linux:

```
pingscan 192.0.2
pingscan -u -t 300 192.0.2.10-40
pingscan -u 192.0.2 | grep '^Contacting host:'
```

Windows:

```
pingscan 192.0.2
pingscan -u -t 300 192.0.2.10-40 > lan_up.txt
```
