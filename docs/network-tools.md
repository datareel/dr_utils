# Network tools

Seven tools for checking connectivity and finding what is on a network. They are based on work from the 1999-2010 network scan programs and keep their output lines (`Contacting host:`, `TCP port n open` ...), so existing scripts keep working.

Use them only on networks and hosts you own or are allowed to scan. The range tools scan one /24 at most.

**Ranges.** `pingscan`, `dnsscan` and `arpscan` take a range:

| Range | Addresses |
|---|---|
| `192.0.2` | 192.0.2.1 to 192.0.2.254 |
| `192.0.2.7` | one address |
| `192.0.2.10-40` | 192.0.2.10 to 192.0.2.40 |

**Services files.** `portscan` and `netpscan` name open ports from a services file. The default is `/etc/services` on Linux and `%SystemRoot%\System32\drivers\etc\services` on Windows. Name another file as the last argument.

**Privileges.** See [Building and installing](install.md#privileges). On Linux a ping needs `net.ipv4.ping_group_range` to include the user's group (the RHEL default), or root. On Windows nothing needs administrator rights.

## internet-test

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

Dashboard: `--drsm=fname`, `--submit[=ID]` (default `dru_internet_test`), `--now`, `--drra=path` send the result to DRSM through the reporting agent, see [Dashboard](dashboard-hooks.md).

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

## pingscan

Ping sweep of a range, with the DNS name and reply time of each host.

```
pingscan [-t ms] [-u] range
```

| Switch | Meaning |
|---|---|
| `-t ms` | ping timeout in milliseconds (default 1000) |
| `-u` | list only the hosts that answer |

Each address gets a `Contacting host:` block (`Hostname:`, then `Host is up, reply in n ms` or `Host not responding.`). The output ends with `n of m hosts answered`. One host that fails does not stop the sweep.

Dashboard: `--drsm=fname`, `--submit[=ID]` (default `dru_pingscan`), `--now`, `--drra=path` send the result to DRSM through the reporting agent, see [Dashboard](dashboard-hooks.md).

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

## dnsscan

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

Dashboard: `--drsm=fname`, `--submit[=ID]` (default `dru_dnsscan`), `--now`, `--drra=path` send the result to DRSM through the reporting agent, see [Dashboard](dashboard-hooks.md).

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

## portscan

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

Dashboard: `--drsm=fname`, `--submit[=ID]` (default `dru_portscan`), `--now`, `--drra=path` send the result to DRSM through the reporting agent, see [Dashboard](dashboard-hooks.md).

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

## netpscan

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

Dashboard: `--drsm=fname`, `--submit[=ID]` (default `dru_netpscan`), `--now`, `--drra=path` send the result to DRSM through the reporting agent, see [Dashboard](dashboard-hooks.md).

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

## arpscan

Lists the MAC address (and vendor) of each host on a directly attached /24 network. It pings each address, then reads the MAC address from the system ARP table (`/proc/net/arp` on Linux, `SendARP` on Windows).

```
arpscan [switches] range
```

| Switch | Meaning |
|---|---|
| `-i dev` | Linux: only ARP entries on this interface (`-i eth0`) |
| `-v file` | vendor code file: one `XXXXXX Vendor Name` line per MAC prefix (6 hex digits, the DataReel `FindVendor` format) |
| `-o file` | also write the list to a file |
| `-t ms` | ping timeout in milliseconds (default 1000) |
| `-a` | list hosts with no MAC address too (`no_ARP_entry`) |

Output is one tab-separated line per host: address, DNS name, MAC address and, with `-v`, the vendor.

```
192.0.2.1	gw.example.com	00:00:5E:00:53:01	ICANN, IANA Department
192.0.2.12	printer.example.com	00:00:5E:00:53:12	ICANN, IANA Department
```

Only hosts on a directly attached network have a MAC address. Hosts behind a router show the router's or none.

An IEEE OUI list converts to a vendor file, for example on Linux:

```
grep '(hex)' oui.txt | sed -E 's/^([0-9A-F]{2})-([0-9A-F]{2})-([0-9A-F]{2}) +\(hex\)\t+/\1\2\3 /' > maccodes.txt
```

Dashboard: `--drsm=fname`, `--submit[=ID]` (default `dru_arpscan`), `--now`, `--drra=path` send the result to DRSM through the reporting agent, see [Dashboard](dashboard-hooks.md).

Exit status: 0 if any MAC address was found, 1 if none, 2 on an error.

Linux:

```
arpscan 192.0.2
arpscan -i eth0 -v maccodes.txt -o lan.txt 192.0.2
arpscan -a -t 300 192.0.2.1-50
```

Windows:

```
arpscan 192.0.2
arpscan -v maccodes.txt -o lan.txt 192.0.2
```

## getmac

Shows the MAC address of one host on a local network.

```
getmac hostname [ethernet_device]
```

| Option | Meaning |
|---|---|
| `hostname` | a host name or address on a directly attached network |
| `ethernet_device` | Linux: only an ARP entry on this interface |

It shows the host's names, pings it, and prints `MAC Address: XX:XX:XX:XX:XX:XX`.

Dashboard: `--drsm=fname`, `--submit[=ID]` (default `dru_getmac`), `--now`, `--drra=path` send the result to DRSM through the reporting agent, see [Dashboard](dashboard-hooks.md).

Exit status: 0 with the MAC address, 1 if there is no ARP entry (the host did not answer or is not on a local network), 2 on an error.

Linux:

```
getmac 192.0.2.12
getmac printer.example.com eth0
mac=$(getmac 192.0.2.12 | sed -n 's/^MAC Address: //p')
```

Windows has its own `getmac.exe` that lists the local adapters, and it comes first on the PATH. Run this one by its full path (or install it under another name, see [Building and installing](install.md#binary-packages)):

```
"C:\Program Files\dr_utils\getmac.exe" 192.0.2.12
C:\src\dr_utils\bin\getmac.exe printer.example.com
```

PowerShell:

```
& 'C:\Program Files\dr_utils\getmac.exe' 192.0.2.12 | Select-String '^MAC Address'
```
