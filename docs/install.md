# Building and installing

Most users install a binary package; building from source needs a DataReel 6.4.1 source tree whose library is already built.

## Binary packages

Each release has these downloads, the same layout as the fcrypt packages:

| Package | What |
|---|---|
| `dr_utils_<v>_rhel_7_8_9_10_binaries.tar.gz` | Linux builds for RHEL 7, 8, 9 and 10 (64-bit) in `rhel7/` ... `rhel10/`, with `install_dr_utils.sh` |
| `dr_utils_<v>_windows_binaries.zip` | Windows 64-bit (x64) programs in `bin\`, with `install_dr_utils.ps1` |
| `dr_utils_<v>_windows_32bit_binaries.zip` | Windows 32-bit (x86) programs, the same layout |
| `dr_utils-<v>.tar.gz`, `dr_utils-<v>.zip` | the source code |

Every package has the scripts (`dr_plot.py`, `dr_report.py`, and on Linux the watch scripts), the reporting agent hook files in `hooks/`, this documentation in `docs/` (open `docs/html/index.html`), a `README.txt` and `sha256sum` files. The programs are built without OpenSSL (they use no TLS) and need no other libraries than the C and C++ runtime and zlib on Linux; the Windows programs have the C runtime built in.

Both installers check the sha256 of every file and are a dry run until you add the install switch. Programs already in the target folder are kept as `name.backup-YYYYMMDD-HHMMSS`.

Linux: the installer picks the build for the host from `/etc/redhat-release`:

```
tar xzf dr_utils_1_4_rhel_7_8_9_10_binaries.tar.gz
cd dr_utils_1_4_rhel_7_8_9_10_binaries
./install_dr_utils.sh                              # what it would do
./install_dr_utils.sh --install                    # into ~/bin
sudo ./install_dr_utils.sh --install --system      # into /usr/local/bin
./install_dr_utils.sh --install --prefix=/opt/dr_utils/bin
./install_dr_utils.sh --build=rhel9                # pick a build by hand
```

Windows, from `cmd.exe` in the unzipped folder:

```
powershell -ExecutionPolicy Bypass -File install_dr_utils.ps1
powershell -ExecutionPolicy Bypass -File install_dr_utils.ps1 -Install
powershell -ExecutionPolicy Bypass -File install_dr_utils.ps1 -Install -System
powershell -ExecutionPolicy Bypass -File install_dr_utils.ps1 -Install -Prefix D:\tools\dr_utils -NoPath
```

`-Install` alone installs in `%LOCALAPPDATA%\DataReel\dr_utils` and adds its `bin\` to your own PATH; `-System` (administrator prompt) installs in `%ProgramFiles%\DataReel\dr_utils` and adds it to the system PATH. The installer also copies `getmac.exe` as `dr_getmac.exe`, because Windows' own `getmac.exe` comes first in the PATH. The programs also run straight from the unzipped `bin\` folder.

## Building from source

Use the same build settings (64-bit or 32-bit, OpenSSL or not) for the DataReel library and for dr_utils. dr_utils uses no TLS, so the release builds use DataReel without OpenSSL (`USE_OPEN_SSL=NO` on Linux, the default Schannel build on Windows).

### Linux (RHEL 7, 8, 9, 10 and other distributions with GCC)

Packages: `gcc-c++`, `make`, `zlib-devel` (for cputest) and `openssl-devel` (DataReel builds with OpenSSL by default). On RHEL:

```
sudo dnf install gcc-c++ make zlib-devel openssl-devel
```

Build the library, then dr_utils. Neither make nor DataReel's `linux.env` sets an optimization level, so set `CXXFLAGS` for the library (`build.sh` uses `-O2` when `CXXFLAGS` is not set). On RHEL 7 (GCC 4.8) add `-std=gnu++11`:

```
export CXXFLAGS="-O2"                     # RHEL 7: export CXXFLAGS="-O2 -std=gnu++11"
cd /path/to/datareel-6.4.1/build
make && make install                      # makes lib/libdatareel.a in the DataReel tree

cd /path/to/dr_utils
DATAREEL_DIR=/path/to/datareel-6.4.1 ./build.sh
ls bin/                                   # the 21 programs
```

`build.sh` builds each program folder in turn and copies the program to `bin/`. It stops with a message if `$DATAREEL_DIR/lib/libdatareel.a` is missing. The exit status is 1 if any program did not build.

```
DATAREEL_DIR=/path/to/datareel-6.4.1 ./build.sh clean     # remove objects and bin/ programs
```

One program at a time:

```
cd tools/portscan && make DATAREEL_DIR=/path/to/datareel-6.4.1
```

Without OpenSSL, build both the library and dr_utils with `USE_OPEN_SSL=NO`:

```
cd /path/to/datareel-6.4.1/build && make USE_OPEN_SSL=NO && make install
cd /path/to/dr_utils && DATAREEL_DIR=/path/to/datareel-6.4.1 USE_OPEN_SSL=NO ./build.sh
```

#### Installing a source build on Linux

There is no install target. Copy the programs and scripts you want to a folder on the PATH:

```
sudo install -m 0755 bin/* /usr/local/bin/
sudo install -m 0755 benchmarks/plot/dr_plot.py benchmarks/plot/dr_report.py /usr/local/bin/
sudo install -m 0755 benchmarks/scripts/watch_*.sh /usr/local/bin/
```

`dr_plot.py` and `dr_report.py` need Python 3 with matplotlib (`sudo dnf install python3-matplotlib`, or `pip install matplotlib`).

The reporting agent hook files and the daily sar job have their own installer, see [Dashboard](dashboard-hooks.md).

### Windows (Visual Studio 2022 or 2026, x64 or x86)

1. Build the DataReel library in `%DATAREEL_DIR%\winslib` (`gxcode64.lib`, 64-bit release).
2. For cputest, build zlib with the static runtime so it matches DataReel's `/MT`. In the zlib source folder, from the same prompt:

   ```
   nmake -f win32\Makefile.msc LOC=-MT zlib.lib
   ```

3. From an **x64 Native Tools Command Prompt**:

   ```
   set DATAREEL_DIR=C:\src\datareel-6.4.1
   set ZLIB_DIR=C:\src\zlib
   cd C:\src\dr_utils
   build.cmd
   dir bin
   ```

`build.cmd` builds 64-bit release programs (`64BITCFG=1 FINAL=1`) and copies each `.exe` to `bin\`. `ZLIB_DIR` is a folder with `zlib.h` and the `/MT` `zlib.lib`; without it cputest does not build and the other programs still do. The exit status is 1 if any program did not build.

For 32-bit programs, build the DataReel library as 32-bit (`gxcode32.lib`) and a 32-bit `/MT` zlib, then from an **x86 Native Tools Command Prompt**:

```
set DATAREEL_DIR=C:\src\datareel-6.4.1-x86
set ZLIB_DIR=C:\src\zlib-x86
set 64BITCFG=0
build.cmd
```

```
build.cmd clean                           & rem remove objects and bin\ programs
cd tools\portscan && nmake -f msvc.mak    & rem one program
```

#### Installing a source build on Windows

Copy `bin\*.exe` to a folder and add it to the PATH, for example from an administrator prompt:

```
mkdir "C:\Program Files\dr_utils"
copy bin\*.exe "C:\Program Files\dr_utils\"
setx /M PATH "%PATH%;C:\Program Files\dr_utils"
```

`setx` changes the PATH for new prompts only; open a new prompt afterwards.

Windows has its own `getmac.exe` (in `System32`, it lists the local adapters), which comes first on the PATH. Run the dr_utils one by its full path, or copy it under another name:

```
copy bin\getmac.exe "C:\Program Files\dr_utils\dr_getmac.exe"
```

For `dr_plot.py` and `dr_report.py` install Python 3 and matplotlib, then run them with the `py` launcher:

```
py -m pip install matplotlib
py C:\src\dr_utils\benchmarks\plot\dr_plot.py --help
```

## Privileges

| What | Linux | Windows |
|---|---|---|
| Ping (`pingscan`, `dnsscan -p`, `arpscan`, and the pings in `portscan`, `netpscan`, `getmac`) | An ICMP datagram socket when `net.ipv4.ping_group_range` includes the user's group (the RHEL default allows every group); otherwise a raw socket, which needs root or `sudo setcap cap_net_raw+ep bin/pingscan` | `IcmpSendEcho`, no administrator rights |
| MAC addresses (`arpscan`, `getmac`) | Reads `/proc/net/arp`, no root | `SendARP`, no administrator rights |
| TCP connects, DNS, file tools, benchmarks | No privileges | No privileges |
| `--submit` to the reporting agent | root or the `drra-upload` group | administrator |

Check the Linux ping setting with `sysctl net.ipv4.ping_group_range`. `0 2147483647` allows every group.

## Firewall ports for the network benchmarks

Tests between two hosts need the benchmark ports open on the server or receiver side: TCP 53897 (`tcpserv`), UDP 53897 (`udpserv`) and UDP 53898 (`mctest`).

Linux (firewalld, until the next reload):

```
sudo firewall-cmd --add-port=53897/tcp --add-port=53897/udp --add-port=53898/udp
```

Windows (administrator prompt):

```
netsh advfirewall firewall add rule name="dr_utils tcpserv" dir=in action=allow protocol=TCP localport=53897
netsh advfirewall firewall add rule name="dr_utils udpserv" dir=in action=allow protocol=UDP localport=53897
netsh advfirewall firewall add rule name="dr_utils mctest"  dir=in action=allow protocol=UDP localport=53898
```

Remove them after the test (`firewall-cmd --reload`, or `netsh advfirewall firewall delete rule name="dr_utils tcpserv"`).

## Tests

The tests touch only 127.0.0.1, a local HTTP server they start, and (for `getmac` and `arpscan`) one host already in the machine's ARP table. The benchmark tests use small sizes.

```
tests/run_tests.sh                                              # Linux, after build.sh
powershell -ExecutionPolicy Bypass -File tests\run_tests.ps1   # Windows, after build.cmd
powershell -ExecutionPolicy Bypass -File tests\run_tests.ps1 -NoLan   # skip the ARP table check
```

`tests/hooks_e2e_test.sh` runs the dashboard path end to end on one Linux machine, see [Dashboard](dashboard-hooks.md#testing-the-whole-path).

## Layout

| Path | What |
|---|---|
| `tools/<name>/` | one folder per file or network tool: the source, `makefile` (Linux), `msvc.mak` (Windows), `project.mak` (the objects for both) |
| `benchmarks/<name>/` | one folder per benchmark program, laid out the same way |
| `benchmarks/plot/` | `dr_plot.py` and `dr_report.py` |
| `benchmarks/scripts/` | the Linux watch scripts |
| `system_stats/sar_cpu_stats/` | the sar report tool |
| `hooks/` | reporting agent hook files, DRSM manifests and install scripts |
| `common/` | shared code: ping, connects, DNS, ARP, services files, benchmark sockets, timers, plot files, DRSM lines |
| `release/` | the installers of the binary packages, their READMEs and `package.sh` |
| `env/` | `linux.mak` and `msvc.mak`, the build settings on top of DataReel's |
| `tests/` | `run_tests.sh`, `run_tests.ps1`, `hooks_e2e_test.sh`, and short sar reports in `tests/data/sar/` |
