# mcserv

Part of dr_utils 1.4. This file is made from [docs/benchmarks.md](../../docs/benchmarks.md#mcserv-and-mctest) by docs/make_docs.py: edit that page, not this file. All the documentation: [docs/](../../docs/README.md), or the searchable HTML in `docs/html/index.html`.

Sizes take decimal suffixes (`1k` = 1000, `1m`, `1g`). The dashboard options (`--drsm`, `--submit`, `--now`, `--drra`, `--drra-config`) are on the [Dashboard](../../docs/dashboard-hooks.md) page.

Multicast rate, loss and order from one sender (`mcserv`) to one or more receivers (`mctest`). Start the receivers first.

```
mctest [-g group] [-i address] [-n receivers] [-p port] [--timeout=S] [--datfile=fname] [dashboard options] [group]
mcserv [-b size] [-g group] [-i address] [-p port] [-r rate] [-s bytes] [--seconds=N] [--ttl=N] [--no-loop]
```

`mctest` (receiver):

| Switch | Meaning |
|---|---|
| `-g<group>` or `group` | multicast group (default 239.255.10.10) |
| `-i<address>` | join on the interface with this IPv4 address |
| `-n<number>` | receivers in this process, 1 to 100 |
| `-p<port>` | port (default 53898) |
| `--timeout=S` | stop when nothing arrives for S seconds (default 60) |
| `--datfile=fname` | write a plot data file |

`mcserv` (sender):

| Switch | Meaning |
|---|---|
| `-b<size>` | datagram size, 32 to 65507 (default 1500) |
| `-g<group>` | multicast group, 224.0.0.0 to 239.255.255.255 (default 239.255.10.10) |
| `-i<address>` | send from the interface with this IPv4 address |
| `-p<port>` | port (default 53898) |
| `-r<rate>` | send rate in bits a second: `-r10M`, `-r1G` (default `100M`) |
| `-s<bytes>` | bytes to send, `1K` to `100G` (default `150M`) |
| `--seconds=N` | stop after N seconds even if bytes are left |
| `--ttl=N` | multicast TTL, 1 to 32 (default 1: the local network only) |
| `--no-loop` | do not deliver to receivers on this host |

The sender sends numbered datagrams at the rate, then end-of-run datagrams. Each receiver prints the datagrams, loss, out-of-order count and rate. The defaults are cautious: TTL 1 and an administratively scoped group keep the traffic on the local network. Exit status of `mctest`: 0 if every receiver saw the end of a run, 1 otherwise. Allow UDP 53898 on the receivers.

Linux, on two hosts of one network:

```
mctest -i192.0.2.21 --datfile=mc.dat           # receiver on 192.0.2.21
mcserv -i192.0.2.10 -r200M -s500M              # sender on 192.0.2.10
```

On one host (loopback, a quick check):

```
mctest -i127.0.0.1 &
mcserv -i127.0.0.1 -s10M
```

Windows, the same switches (start the receiver in its own prompt):

```
start "mctest" mctest -i192.0.2.21 --datfile=mc.dat
mcserv -i192.0.2.10 -r200M -s500M
```

Example output:

```
Receiver[0]: joined multicast group 239.255.10.10 port 53898
Receiver[0]: run 1435795080: 3333 of 3333 datagrams (0.00% lost, 0 out of order), 4999500 bytes in 0.399 seconds, 12.52 MB/s
All tests complete: 1 of 1 receivers saw the end of a run
```
