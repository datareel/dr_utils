# getmac

Part of dr_utils 1.5. This file is made from [docs/network-tools.md](../../docs/network-tools.md#getmac) by docs/make_docs.py: edit that page, not this file. All the documentation: [docs/](../../docs/README.md), or the searchable HTML in `docs/html/index.html`.

Ranges (`192.0.2`, `192.0.2.7`, `192.0.2.10-40`), services files and the privileges ping needs are on the [Network tools](../../docs/network-tools.md) page.

Shows the MAC address of one host on a local network.

```
getmac hostname [ethernet_device]
```

| Option | Meaning |
|---|---|
| `hostname` | a host name or address on a directly attached network |
| `ethernet_device` | Linux: only an ARP entry on this interface |

It shows the host's names, pings it, and prints `MAC Address: XX:XX:XX:XX:XX:XX`.

Dashboard: `--drsm=fname`, `--submit[=ID]` (default `dru_getmac`), `--now`, `--drra=path` send the result to DRSM through the reporting agent, see [Dashboard](../../docs/dashboard-hooks.md).

Exit status: 0 with the MAC address, 1 if there is no ARP entry (the host did not answer or is not on a local network), 2 on an error.

Linux:

```
getmac 192.0.2.12
getmac printer.example.com eth0
mac=$(getmac 192.0.2.12 | sed -n 's/^MAC Address: //p')
```

Windows has its own `getmac.exe` that lists the local adapters, and it comes first on the PATH. Run this one by its full path (or install it under another name, see [Building and installing](../../docs/install.md#binary-packages)):

```
"C:\Program Files\dr_utils\getmac.exe" 192.0.2.12
C:\src\dr_utils\bin\getmac.exe printer.example.com
```

PowerShell:

```
& 'C:\Program Files\dr_utils\getmac.exe' 192.0.2.12 | Select-String '^MAC Address'
```
