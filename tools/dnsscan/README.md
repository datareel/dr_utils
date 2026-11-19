# dnsscan

Part of dr_utils 1.4. This file is made from [docs/network-tools.md](../../docs/network-tools.md#dnsscan) by docs/make_docs.py: edit that page, not this file. All the documentation: [docs/](../../docs/README.md), or the searchable HTML in `docs/html/index.html`.

Ranges (`192.0.2`, `192.0.2.7`, `192.0.2.10-40`), services files and the privileges ping needs are on the [Network tools](../../docs/network-tools.md) page.

Lists the reverse DNS names in a range, with an optional forward check and ping.

```
dnsscan [-a] [-f] [-p] [-t ms] range
```

| Switch | Meaning |
|---|---|
| `-a` | also list addresses with no DNS name (`no_DNS_entry`) |
| `-f` | check that each name resolves back to the address: `fwd_ok`, or `fwd_mismatch` with the address it resolves to |
| `-p` | ping each address too: `up (ms)` or `down` |
| `-t ms` | ping timeout in milliseconds (default 1000) |

Output is one tab-separated line per address: address, name, then the `-f` and `-p` columns.

```
192.0.2.10	www.example.com	fwd_ok	up (0.4 ms)
192.0.2.11	old-host.example.com	fwd_mismatch 192.0.2.99	down
```

Dashboard: `--drsm=fname`, `--submit[=ID]` (default `dru_dnsscan`), `--now`, `--drra=path` send the result to DRSM through the reporting agent, see [Dashboard](../../docs/dashboard-hooks.md).

Exit status: 0 if any address has a name, 1 if none, 2 on an error.

Linux:

```
dnsscan 192.0.2
dnsscan -f 192.0.2 | grep fwd_mismatch          # stale PTR records
dnsscan -a -f -p -t 300 192.0.2.1-60 > dns_192.0.2.tsv
```

Windows:

```
dnsscan -f 192.0.2
dnsscan -a -f -p 192.0.2 > dns_192.0.2.tsv
```

PowerShell, as objects:

```
dnsscan -f -p 192.0.2 | ConvertFrom-Csv -Delimiter "`t" -Header Address,Name,Forward,Ping
```
