# arpscan

Part of dr_utils 1.5. This file is made from [docs/network-tools.md](../../docs/network-tools.md#arpscan) by docs/make_docs.py: edit that page, not this file. All the documentation: [docs/](../../docs/README.md), or the searchable HTML in `docs/html/index.html`.

Ranges (`192.0.2`, `192.0.2.7`, `192.0.2.10-40`), services files and the privileges ping needs are on the [Network tools](../../docs/network-tools.md) page.

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

Dashboard: `--drsm=fname`, `--submit[=ID]` (default `dru_arpscan`), `--now`, `--drra=path` send the result to DRSM through the reporting agent, see [Dashboard](../../docs/dashboard-hooks.md).

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
