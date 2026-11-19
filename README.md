# dr_utils 1.0

Small command line file and network tools for Windows and Linux, built on the [DataReel](https://datareel.com) C++ library, version 6.4.1. They are open source (GNU LGPL 3) versions of tools Douglas Gaer wrote from 1999 to 2010. DataReel 7.4 does not already have them.

| Tool | What it does |
|---|---|
| `file_hours` | Says YES if a file was last changed N hours ago or more, else NOT: a stale file check for scripts |
| `file_purge` | Deletes or moves files N hours or days old, with a dry run and a listing mode |
| `filetimes` | Shows a file's access, change and creation times in local time and GMT, and its age; finds the newest or oldest of several files |
| `internet-test` | Waits until a web server answers over HTTP (DNS, connect and HTTP checked) |
| `pingscan` | Ping sweep of a /24 network or a range, with DNS names and reply times |
| `dnsscan` | Lists the DNS names in a /24 range, with forward/reverse checks and an optional ping |
| `portscan` | Checks one host: names, ping, TCP connect to a port, a list or a range, and service names |
| `netpscan` | Scans a list of addresses for open TCP ports (services file ports, or a list) |
| `arpscan` | Lists the MAC address (and vendor) of each host on a local /24 network |
| `getmac` | Shows the MAC address of one host on a local network |

Use the network tools only on networks and hosts you own or are allowed to test. They scan one /24 at most.

## License

GNU Lesser General Public License version 3 or later, the same as DataReel 6.4.2 and later. See [LICENSE](LICENSE). Each file says which original program it is based on.

## Building

You need a DataReel 6.4.1 source tree with its library built. Use the same build settings for the library and for dr_utils.

### Linux (RHEL 8, 9, 10 and other distributions with GCC)

```
cd /path/to/datareel-6.4.1/build && make && make install   # lib/libdatareel.a
cd /path/to/dr_utils
DATAREEL_DIR=/path/to/datareel-6.4.1 ./build.sh            # programs in bin/
tests/run_tests.sh
```

DataReel builds with OpenSSL by default (openssl-devel). Use `USE_OPEN_SSL=NO` for both the library and dr_utils on a system without it. One tool: `cd tools/portscan && make DATAREEL_DIR=...`.

### Windows (Visual Studio 2022 or 2026, x64)

Build the DataReel library in `%DATAREEL_DIR%\winslib` (gxcode64.lib, 64-bit release). Then, from an x64 Native Tools command prompt:

```
set DATAREEL_DIR=C:\path\to\datareel-6.4.1
cd \path\to\dr_utils
build.cmd
powershell -ExecutionPolicy Bypass -File tests\run_tests.ps1
```

`build.cmd` builds 64-bit release programs (`64BITCFG=1 FINAL=1`) into `bin\`. One tool: `cd tools\portscan` and `nmake -f msvc.mak`. The programs link `setargv.obj`, so wildcards such as `*.log` work in cmd.exe.

## Privileges

- **Ping** (pingscan, dnsscan -p, arpscan, and the informational pings of portscan, netpscan and getmac): on Linux an ICMP datagram socket is used when `net.ipv4.ping_group_range` includes the user's group (the RHEL default allows every group), else a raw socket, which needs root or `setcap cap_net_raw+ep`. On Windows `IcmpSendEcho` needs no administrator rights.
- **MAC addresses**: Linux reads `/proc/net/arp` and Windows uses `SendARP`. Neither needs root. Only hosts on a directly attached network have a MAC address.
- **TCP connects and DNS** need no privileges.

## Tool reference

Every tool prints its usage with no arguments or `-?`.

### file_hours

```
file_hours [-s] filename numhours
```

Prints `YES` if the file mod time is `numhours` old or older, `NOT` if it is newer, and `ERR - <reason>` if the file cannot be checked. `numhours` can be fractional (0.5 is 30 minutes). The exit status is 0 for YES or NOT and 1 for ERR. With `-s` it is 0 for YES, 1 for NOT and 2 for ERR.

```
if [ "$(file_hours /var/spool/feed/latest.dat 2)" = "YES" ]; then echo "feed is stale"; fi
```

### file_purge

```
file_purge [-e | -l | -h<n> | -d<n>] [-m"dir"] [-n] [-v] file [file ...]
```

- `-h<n>` or `-d<n>`: delete each file that is n hours or days old or older.
- `-m"dir"`: move the file instead (a copy and delete across file systems). An existing file in `dir` is never overwritten.
- `-e`: delete the file if it exists, with no time check.
- `-l`: list each file's age and change nothing.
- `-n`: dry run. Print what would be done (`DRY - ...`) and change nothing.

Directories are never purged. Each file gets one line: `YES`, `NOT`, `ERR`, `LST` or `DRY`. Run with `-n` first.

```
file_purge -n -d30 /var/log/myapp/*.log     # what would go
file_purge -d30 /var/log/myapp/*.log
file_purge -m"D:\archive" -h12 C:\spool\*.dat
```

### filetimes

```
filetimes [-n | -o] file [file ...]
```

Prints each file's access, modification and creation times (on Linux the inode change time) in local time and GMT, and how old it is. `-n` or `-o` prints only the name of the newest or oldest file.

### internet-test

```
internet-test [-c n] [-w sec] [-t ms] [-p port] [-q] server
```

Resolves `server`, connects to its HTTP port and reads a response header. Any HTTP status passes. By default it tries every 60 seconds until it passes (`-c 0`). Use `-c` for a fixed number of tries. `server` can be a host name, `host:port` or a URL. Exit status 0 passed, 1 failed, 2 usage error.

```
internet-test -c 10 -w 30 www.example.com && start_my_job
```

### pingscan

```
pingscan [-t ms] [-u] range
```

`range` is `a.b.c` (hosts .1 to .254), `a.b.c.d` or `a.b.c.x-y`. Each address is pinged and its DNS name looked up. `-u` lists only the hosts that answer. The output ends with "n of m hosts answered".

### dnsscan

```
dnsscan [-a] [-f] [-p] [-t ms] range
```

Prints one tab-separated line per named address: address, reverse DNS name, then the `-f` forward check (`fwd_ok`, or `fwd_mismatch` with what the name resolves to), then the `-p` ping result (`up (ms)` or `down`). `-a` also lists addresses without a name.

### portscan

```
portscan [-t ms] [-n] hostname ports [services_file]
```

`ports` is a port, a list or a range (`22`, `22,80,443`, `8000-8100`, up to 1024 ports). The tool shows the host's names and addresses, pings it (`-n` skips the ping), connects to each port with a timeout (default 2000 ms) and shows the services file entry of each open port. A host that drops ICMP is still scanned. Exit status 0 if any port is open, 1 if none, 2 on an error.

### netpscan

```
netpscan [-p ports] [-A] [-t ms] [-s] iplist_file [services_file]
```

`iplist_file` has one address or host name per line, with `#` comments. By default every address is scanned on the TCP ports below 1024 that are named in the services file. `-A` scans every TCP entry in the services file, and `-p` scans a port list instead. `-s` skips addresses that do not answer a ping. Each open port is printed as `port/tcp name aliases # comment`.

### arpscan

```
arpscan [-i dev] [-v vendor_file] [-o out_file] [-t ms] [-a] range
```

Pings each address, then reads its MAC address from the system ARP table. One tab-separated line per host: address, DNS name, MAC address and, with `-v`, the vendor. The vendor file has one `XXXXXX Vendor Name` line per MAC prefix (6 hex digits, the DataReel `FindVendor` format). An IEEE OUI list converts to it. `-i` (Linux) uses only entries on one interface. `-o` also writes the list to a file.

### getmac

```
getmac hostname [ethernet_device]
```

Shows the host's names, pings it and prints `MAC Address: XX:XX:XX:XX:XX:XX`. The exit status is 1 if there is no ARP entry. Windows has its own `getmac.exe`, which lists the local adapters. Run this one by its full path, or rename it, if both are on the PATH.

## Layout

| Path | What |
|---|---|
| `tools/<name>/` | one folder per tool: source, `makefile` (Linux), `msvc.mak` (Windows), `project.mak` (objects for both) |
| `common/` | `drnet.h/.cpp` (ping, TCP connect with a timeout, DNS, ARP, ranges, services files) and `drutil.h` (file ages) |
| `env/` | `linux.mak` and `msvc.mak`, the build settings on top of DataReel's |
| `tests/` | `run_tests.sh` (Linux) and `run_tests.ps1` (Windows) |

## History

| Tool | Based on |
|---|---|
| file_hours, file_purge, filetimes | Douglas Gaer's 2005 file tools |
| internet-test | the 2005 Internet connectivity test |
| pingscan, dnsscan, portscan, netpscan, arpscan, getmac | the 1999-2010 network scan programs |

The 1.0 versions keep the original switches and output lines, and fix these problems in the old versions:

- Ages under one hour or one day counted as zero, so `-h0.5` never matched.
- A filtered port waited for the system connect timeout, about two minutes.
- One failed ping stopped a sweep.
- The Windows ARP lookup ran `arp -a` into a file in the current folder.
- Windows needed administrator rights for ICMP.
