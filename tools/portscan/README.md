# portscan

Part of dr_utils 1.4. This file is made from [docs/network-tools.md](../../docs/network-tools.md#portscan) by docs/make_docs.py: edit that page, not this file. All the documentation: [docs/](../../docs/README.md), or the searchable HTML in `docs/html/index.html`.

Ranges (`192.0.2`, `192.0.2.7`, `192.0.2.10-40`), services files and the privileges ping needs are on the [Network tools](../../docs/network-tools.md) page.

Checks one host: its names and addresses, a ping, and a TCP connect to each port, with the services file entry of each open port.

```
portscan [-t ms] [-n] hostname ports [services_file]
```

| Option | Meaning |
|---|---|
| `hostname` | a host name or address |
| `ports` | a port (`22`), a list (`22,80,443`) or a range (`8000-8100`); up to 1024 ports |
| `-t ms` | connect and ping timeout in milliseconds (default 2000) |
| `-n` | do not ping the host first |
| `services_file` | the services file for the port names (see above) |

A host that drops ICMP is still scanned. A filtered port waits only for `-t`, not the system connect timeout.

```
Contacting host: 192.0.2.10
Hostname: www.example.com
Host IP Address: 192.0.2.10
TCP port 22 open
  Name:     ssh
  Comment:  The Secure Shell (SSH) Protocol
TCP port 443 open
  Name:     https
2 of 3 TCP ports open
```

Dashboard: `--drsm=fname`, `--submit[=ID]` (default `dru_portscan`), `--now`, `--drra=path` send the result to DRSM through the reporting agent, see [Dashboard](../../docs/dashboard-hooks.md).

Exit status: 0 if any port is open, 1 if none is, 2 on an error.

Linux:

```
portscan www.example.com 22,80,443
portscan -n -t 500 192.0.2.10 1-1024
portscan 192.0.2.10 8000-8100 ./my_services
portscan -n db.example.com 5432 > /dev/null && echo "database port is open"
```

Windows:

```
portscan www.example.com 22,80,443,3389
portscan -n -t 500 192.0.2.10 1-1024
portscan -n db.example.com 1433 > nul && echo database port is open
```
