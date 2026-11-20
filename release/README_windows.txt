dr_utils @VERSION@, Windows @ARCH@ binaries, built on the DataReel C++
library 6.4.1 with Visual Studio 2026.

dr_utils is a set of small command line programs: file tools (file_hours,
file_purge, filetimes), network tools (internet-test, pingscan, dnsscan,
portscan, netpscan, arpscan, getmac), system benchmarks (cputest, flops,
memtest, disktest, tcpserv/tcptest, udpserv/udptest, mcserv/mctest) and
sar_cpu_stats, plus the dr_plot.py and dr_report.py chart and report
scripts.

PACKAGES
  dr_utils_<version>_windows_binaries.zip         64-bit (x64), the
                                                  preferred package for
                                                  Windows 10 and 11
  dr_utils_<version>_windows_32bit_binaries.zip   32-bit (x86), for
                                                  32-bit Windows
The programs have the C runtime built in: no DLLs and no Visual C++
redistributable are needed. They use no OpenSSL.

INSTALL
install_dr_utils.ps1 checks the sha256 of every file and copies the
programs to bin\ and the scripts to scripts\ under the install folder,
then adds bin\ to the PATH. It is a dry run by default. From cmd.exe in
this folder:

  powershell -ExecutionPolicy Bypass -File install_dr_utils.ps1
      shows what it would do
  powershell -ExecutionPolicy Bypass -File install_dr_utils.ps1 -Install
      installs in %LOCALAPPDATA%\DataReel\dr_utils, your own PATH
  powershell -ExecutionPolicy Bypass -File install_dr_utils.ps1 -Install -System
      installs in %ProgramFiles%\DataReel\dr_utils, the system PATH
      (administrator prompt)

Programs already there are kept as name.backup-YYYYMMDD-HHMMSS. Open a
new prompt after the install to get the new PATH.

Windows has its own getmac.exe (it lists the local adapters), and it
comes first in the PATH: the installer also copies the dr_utils getmac
as dr_getmac.exe.

You can also run the programs straight from bin\ with no install.

OTHER FILES
  scripts\      dr_plot.py and dr_report.py (Python 3 with matplotlib:
                py -m pip install matplotlib)
  hooks\        the DataReel Reporting Agent hook files, to send results
                to the System Monitor dashboard (--submit). Install them
                with hooks\windows\install_hooks.ps1 (dry run by default)
  docs\         the documentation: open docs\html\index.html in a browser
                (searchable, works from disk), or read the .md files
  LICENSE       GNU Lesser General Public License version 3

Every program prints its usage with -? or --help (the file and network
tools with no arguments too). Use the network tools and network
benchmarks only on networks and hosts you own or are allowed to test.

Documentation and downloads: https://datareel.com/info_dr_utils.php
