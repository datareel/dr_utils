# udptest

Part of dr_utils 1.4. This file is made from [docs/benchmarks.md](../../docs/benchmarks.md#udpserv-and-udptest) by docs/make_docs.py: edit that page, not this file. All the documentation: [docs/](../../docs/README.md), or the searchable HTML in `docs/html/index.html`.

Sizes take decimal suffixes (`1k` = 1000, `1m`, `1g`). The dashboard options (`--drsm`, `--submit`, `--now`, `--drra`, `--drra-config`) are on the [Dashboard](../../docs/dashboard-hooks.md) page.

UDP bandwidth and loss with one or more clients.

```
udpserv [-p port] [-l address] [--count=N]
udptest [-b size] [-n clients] [-p port] [-r rate] [-s bytes] [--datfile=fname] [dashboard options] server
```

`udpserv`:

| Switch | Meaning |
|---|---|
| `-p<port>` | port (default 53897) |
| `-l<address>` | listen on this IPv4 address only |
| `--count=N` | exit after N client runs (default: run until Ctrl-C) |

`udptest`:

| Switch | Meaning |
|---|---|
| `-b<size>` | datagram size, 32 to 65507 (default 1500) |
| `-n<clients>` | clients, 1 to 1000 |
| `-p<port>` | port (default 53897) |
| `-r<rate>` | send rate per client in bits a second: `-r100M`, `-r1G` (default: as fast as the host can) |
| `-s<bytes>` | bytes per client, `1K` to `100G` (default `150M`) |
| `--datfile=fname` | write a plot data file |

The clients send numbered datagrams. At the end the server reports what arrived, and each client prints its send rate, the server's receive rate, the loss and the out-of-order count. Exit status of `udptest`: 0 if every client got a report, 1 otherwise. Allow UDP 53897 in the server's firewall.

```
Client[0]: sent 3333 datagrams in 0.199 seconds, 25.17 MB/s; server received 3333 (0.00% lost, 0 out of order), 25.18 MB/s
All tests complete: 1 of 1 clients reported, 3333 of 3333 datagrams received (0.00% lost), 25.18 MB/s received
```

Without `-r` the loss shows what the network and the server dropped at full speed; with `-r` you can find the rate a link carries without loss.

Linux:

```
udpserv                                         # on 192.0.2.10
udptest -s100M -r500M 192.0.2.10
udptest -n4 -b8972 -r1G --datfile=udp.dat 192.0.2.10   # jumbo frames (MTU 9000)
```

Windows:

```
udpserv -p53897
udptest -s100M -r500M 192.0.2.10
```
