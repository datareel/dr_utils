# tcpserv

Part of dr_utils 1.5. This file is made from [docs/benchmarks.md](../../docs/benchmarks.md#tcpserv-and-tcptest) by docs/make_docs.py: edit that page, not this file. All the documentation: [docs/](../../docs/README.md), or the searchable HTML in `docs/html/index.html`.

Sizes take decimal suffixes (`1k` = 1000, `1m`, `1g`). The dashboard options (`--drsm`, `--submit`, `--now`, `--drra`, `--drra-config`) are on the [Dashboard](../../docs/dashboard-hooks.md) page.

TCP bandwidth, upload or download, with one or more clients. Start `tcpserv` on one host, then run `tcptest` on another.

```
tcpserv [-p port] [-l address] [--count=N] [--timeout=S] [--max-bytes=SIZE]
tcptest [-b size] [-d] [-n clients] [-p port] [-s bytes] [--datfile=fname] [dashboard options] server
```

`tcpserv`:

| Switch | Meaning |
|---|---|
| `-p<port>` | port, 1024 to 65535 (default 53897) |
| `-l<address>` | listen on this IPv4 address only (default: all addresses) |
| `--count=N` | exit after N transfers; each client is one transfer (default: run until Ctrl-C) |
| `--timeout=S` | close a client idle for S seconds (default 30) |
| `--max-bytes=SIZE` | the largest transfer accepted (default `100g`) |

`tcptest`:

| Switch | Meaning |
|---|---|
| `-b<size>` | write or read size, 64 to 1048576 (default 1500) |
| `-d` | download from the server (default: upload to it) |
| `-n<clients>` | clients, 1 to 1000 (default 1) |
| `-p<port>` | port (default 53897) |
| `-s<bytes>` | bytes per client, `1K` to `100G` (default `150M`) |
| `--datfile=fname` | write a plot data file |

The server confirms each byte count. Each client and the total print their rates. Clients cannot stop or restart the server. Exit status of `tcptest`: 0 if every client finished, 1 otherwise. Allow TCP 53897 in the server's firewall (see [install](../../docs/install.md#firewall-ports-for-the-network-benchmarks)).

Linux, server and client:

```
tcpserv                                   # on 192.0.2.10, Ctrl-C to stop
tcpserv -l192.0.2.10 --count=4            # exit after 4 transfers

tcptest 192.0.2.10                        # one client, 150M upload
tcptest -n4 -b65536 -s1G 192.0.2.10       # four clients, 1G each
tcptest -d -n4 -b65536 -s1G --datfile=tcp_down.dat 192.0.2.10
```

Windows, the same switches:

```
tcpserv -p53897
tcptest -n4 -b65536 -s1G --datfile=tcp_up.dat 192.0.2.10
```

Example output:

```
Connecting 4 clients to 192.0.2.10 port 53897, 1G bytes each, 65536 byte buffers, upload
Client[0]: sent 1000000000 bytes in 9.312 seconds, 107.39 MB/s
...
All tests complete: 4 of 4 clients, 4000000000 bytes in 36.950 seconds, 108.25 MB/s
```
