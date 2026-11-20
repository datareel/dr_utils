dr_utils @VERSION@, binaries for Red Hat Enterprise Linux 7, 8, 9 and 10
(64-bit), built on the DataReel C++ library 6.4.1.

dr_utils is a set of small command line programs: file tools (file_hours,
file_purge, filetimes), network tools (internet-test, pingscan, dnsscan,
portscan, netpscan, arpscan, getmac), system benchmarks (cputest, flops,
memtest, disktest, tcpserv/tcptest, udpserv/udptest, mcserv/mctest) and
sar_cpu_stats, plus the dr_plot.py and dr_report.py chart and report
scripts and the Linux watch scripts.

INSTALL
No installer package is needed: install_dr_utils.sh picks the build for
this host from /etc/redhat-release, checks the sha256 of every file, and
copies the programs and scripts (dry run by default):

  ./install_dr_utils.sh              shows what it would do
  ./install_dr_utils.sh --install    installs in ~/bin
  sudo ./install_dr_utils.sh --install --system    installs in /usr/local/bin
  ./install_dr_utils.sh --help       all options (--prefix=DIR, --build=rhelN)

Programs already there are kept as name.backup-YYYYMMDD-HHMMSS.

BUILDS
  rhel7/    RHEL 7 (and CentOS, Oracle Linux 7)
  rhel8/    RHEL 8 (and Rocky, AlmaLinux 8)
  rhel9/    RHEL 9 (and Rocky, AlmaLinux 9)
  rhel10/   RHEL 10 (and Rocky, AlmaLinux 10)
Each build needs only the C and C++ runtime and zlib, which every RHEL
install has. The programs are built without OpenSSL: they use no TLS, so
the OpenSSL version and the FIPS mode of the host do not matter. Check a
build by hand with: cd rhelN && sha256sum -c sha256sum.txt

OTHER FILES
  scripts/      dr_plot.py, dr_report.py (Python 3 with matplotlib:
                dnf install python3-matplotlib), watch_system.sh,
                watch_mem.sh, watch_mem_usage.sh
  hooks/        the DataReel Reporting Agent hook files, to send results
                to the System Monitor dashboard (--submit). Install them
                with sudo hooks/linux/install_hooks.sh (dry run by default)
  docs/         the documentation: open docs/html/index.html in a browser
                (searchable, works from disk), or read the .md files
  LICENSE       GNU Lesser General Public License version 3

Every program prints its usage with -? or --help (the file and network
tools with no arguments too). Use the network tools and network
benchmarks only on networks and hosts you own or are allowed to test.

Documentation and downloads: https://datareel.com/info_dr_utils.php
