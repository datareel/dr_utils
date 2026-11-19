// Made by docs/make_docs.py
var DRU_INDEX = [
{
"h": "Overview",
"p": "Overview",
"u": "index.html",
"t": ""
},
{
"h": "dr_utils 1.4 documentation",
"p": "Overview",
"u": "index.html#dr_utils-14-documentation",
"t": "dr_utils is a set of command line file tools, network tools, system benchmarks and a sar report reader for Windows and Linux, built on the DataReel C++ library 6.4.1. These pages cover every program: what it does, every option, and command lines for both systems. Page What it covers Building and installing DataReel 6.4.1, build.sh and build.cmd , zlib, putting the programs on the PATH, privileges, the test suites File tools file_hours , file_purge , filetimes Network tools internet-test , pingscan , dnsscan , portscan , netpscan , arpscan , getmac Benchmarks cputest , disktest , memtest , flops , tcpserv / tcptest , udpserv / udptest , mcserv / mctest sar CPU statistics sar_cpu_stats : averages, peaks, steal and iowait spikes, gaps, CSV Plots, reports and watch scripts --datfile files, dr_plot.py , dr_report.py , watch_system.sh , watch_mem.sh , watch_mem_usage.sh Dashboard: reporting agent hooks --drsm , --submit , the DataReel Reporting Agents for Linux and Windows, DRSM setup, the daily sar job Quick reference Every program and option on one page, with exit status"
},
{
"h": "HTML version and folder READMEs",
"p": "Overview",
"u": "index.html#html-version-and-folder-readmes",
"t": "docs/html/ has the same pages as HTML with a side menu and a search box (press / to search). Open docs/html/index.html in a browser; it works straight from disk on Windows and Linux, with no web server and no network, or copy the folder to a web server. A search can be linked as index.html?q=words . Each program folder also has a README.md with that program's part of these pages ( tools/portscan/README.md , benchmarks/cputest/README.md , benchmarks/plot/README.md , system_stats/sar_cpu_stats/README.md ...), so the text docs are next to the source. The markdown pages here are the source. After editing them (or hooks/README.md ), rebuild, and commit the pages, docs/html and the folder READMEs together: python3 docs/make_docs.py # Linux (needs python3-markdown, or pip install markdown) py docs\\make_docs.py # Windows python3 docs/make_docs.py --check # exit 1 if docs/html or a folder README is out of date"
},
{
"h": "Conventions in these pages",
"p": "Overview",
"u": "index.html#conventions-in-these-pages",
"t": "Linux examples run in bash, with the programs on the PATH (or as bin/<program> from the dr_utils folder). Windows examples run in cmd.exe unless they say PowerShell, with the programs on the PATH (or as bin\\<program>.exe ). The programs are the same on both systems, with the same options. Addresses and names in the examples are documentation values: 192.0.2.x and example.com . Put in your own. Use the network tools and network benchmarks only on networks and hosts you own or are allowed to test. The scan tools scan one /24 at most."
},
{
"h": "Common behaviour",
"p": "Overview",
"u": "index.html#common-behaviour",
"t": "Every program prints its version and usage with no arguments (the file and network tools), with -? , or with --help (the benchmarks and sar_cpu_stats ). The usage text is the reference these pages were written from. Sizes and counts in the benchmarks take decimal suffixes: 1k = 1000, 1m = 1000000, 1g = 1000000000 (upper case works too: 150M , 1G ). Wildcards such as *.log work on Windows in cmd.exe and PowerShell, because the programs link setargv.obj and expand them themselves. Exit status: each page lists it. On Linux read $? , in cmd.exe read %ERRORLEVEL% , and in PowerShell read $LASTEXITCODE ."
},
{
"h": "Building and installing",
"p": "Building and installing",
"u": "install.html",
"t": ""
},
{
"h": "Building and installing",
"p": "Building and installing",
"u": "install.html#building-and-installing",
"t": "dr_utils builds against a DataReel 6.4.1 source tree whose library is already built. Use the same build settings (64-bit, OpenSSL or not) for the library and for dr_utils."
},
{
"h": "Linux (RHEL 8, 9, 10 and other distributions with GCC)",
"p": "Building and installing",
"u": "install.html#linux-rhel-8-9-10-and-other-distributions-with-gcc",
"t": "Packages: gcc-c++ , make , zlib-devel (for cputest) and openssl-devel (DataReel builds with OpenSSL by default). On RHEL: sudo dnf install gcc-c++ make zlib-devel openssl-devel Build the library, then dr_utils: cd /path/to/datareel-6.4.1/build make && make install # makes lib/libdatareel.a in the DataReel tree cd /path/to/dr_utils DATAREEL_DIR=/path/to/datareel-6.4.1 ./build.sh ls bin/ # the 21 programs build.sh builds each program folder in turn and copies the program to bin/ . It stops with a message if $DATAREEL_DIR/lib/libdatareel.a is missing. The exit status is 1 if any program did not build. DATAREEL_DIR=/path/to/datareel-6.4.1 ./build.sh clean # remove objects and bin/ programs One program at a time: cd tools/portscan && make DATAREEL_DIR=/path/to/datareel-6.4.1 Without OpenSSL, build both the library and dr_utils with USE_OPEN_SSL=NO : cd /path/to/datareel-6.4.1/build && make USE_OPEN_SSL=NO && make install cd /path/to/dr_utils && DATAREEL_DIR=/path/to/datareel-6.4.1 USE_OPEN_SSL=NO ./build.sh"
},
{
"h": "Installing on Linux",
"p": "Building and installing",
"u": "install.html#installing-on-linux",
"t": "There is no install target. Copy the programs and scripts you want to a folder on the PATH: sudo install -m 0755 bin/* /usr/local/bin/ sudo install -m 0755 benchmarks/plot/dr_plot.py benchmarks/plot/dr_report.py /usr/local/bin/ sudo install -m 0755 benchmarks/scripts/watch_*.sh /usr/local/bin/ dr_plot.py and dr_report.py need Python 3 with matplotlib ( sudo dnf install python3-matplotlib , or pip install matplotlib ). The reporting agent hook files and the daily sar job have their own installer, see Dashboard ."
},
{
"h": "Windows (Visual Studio 2022 or 2026, x64)",
"p": "Building and installing",
"u": "install.html#windows-visual-studio-2022-or-2026-x64",
"t": "Build the DataReel library in %DATAREEL_DIR%\\winslib ( gxcode64.lib , 64-bit release). For cputest, build zlib with the static runtime so it matches DataReel's /MT . In the zlib source folder, from the same prompt: nmake -f win32\\Makefile.msc LOC=-MT zlib.lib From an x64 Native Tools Command Prompt : set DATAREEL_DIR=C:\\src\\datareel-6.4.1 set ZLIB_DIR=C:\\src\\zlib cd C:\\src\\dr_utils build.cmd dir bin build.cmd builds 64-bit release programs ( 64BITCFG=1 FINAL=1 ) and copies each .exe to bin\\ . ZLIB_DIR is a folder with zlib.h and the /MT zlib.lib ; without it cputest does not build and the other programs still do. The exit status is 1 if any program did not build. build.cmd clean & rem remove objects and bin\\ programs cd tools\\portscan && nmake -f msvc.mak & rem one program"
},
{
"h": "Installing on Windows",
"p": "Building and installing",
"u": "install.html#installing-on-windows",
"t": "Copy bin\\*.exe to a folder and add it to the PATH, for example from an administrator prompt: mkdir \"C:\\Program Files\\dr_utils\" copy bin\\*.exe \"C:\\Program Files\\dr_utils\\\" setx /M PATH \"%PATH%;C:\\Program Files\\dr_utils\" setx changes the PATH for new prompts only; open a new prompt afterwards. Windows has its own getmac.exe (in System32 , it lists the local adapters), which comes first on the PATH. Run the dr_utils one by its full path, or copy it under another name: copy bin\\getmac.exe \"C:\\Program Files\\dr_utils\\dr_getmac.exe\" For dr_plot.py and dr_report.py install Python 3 and matplotlib, then run them with the py launcher: py -m pip install matplotlib py C:\\src\\dr_utils\\benchmarks\\plot\\dr_plot.py --help"
},
{
"h": "Privileges",
"p": "Building and installing",
"u": "install.html#privileges",
"t": "What Linux Windows Ping ( pingscan , dnsscan -p , arpscan , and the pings in portscan , netpscan , getmac ) An ICMP datagram socket when net.ipv4.ping_group_range includes the user's group (the RHEL default allows every group); otherwise a raw socket, which needs root or sudo setcap cap_net_raw+ep bin/pingscan IcmpSendEcho , no administrator rights MAC addresses ( arpscan , getmac ) Reads /proc/net/arp , no root SendARP , no administrator rights TCP connects, DNS, file tools, benchmarks No privileges No privileges --submit to the reporting agent root or the drra-upload group administrator Check the Linux ping setting with sysctl net.ipv4.ping_group_range . 0 2147483647 allows every group."
},
{
"h": "Firewall ports for the network benchmarks",
"p": "Building and installing",
"u": "install.html#firewall-ports-for-the-network-benchmarks",
"t": "Tests between two hosts need the benchmark ports open on the server or receiver side: TCP 53897 ( tcpserv ), UDP 53897 ( udpserv ) and UDP 53898 ( mctest ). Linux (firewalld, until the next reload): sudo firewall-cmd --add-port=53897/tcp --add-port=53897/udp --add-port=53898/udp Windows (administrator prompt): netsh advfirewall firewall add rule name=\"dr_utils tcpserv\" dir=in action=allow protocol=TCP localport=53897 netsh advfirewall firewall add rule name=\"dr_utils udpserv\" dir=in action=allow protocol=UDP localport=53897 netsh advfirewall firewall add rule name=\"dr_utils mctest\" dir=in action=allow protocol=UDP localport=53898 Remove them after the test ( firewall-cmd --reload , or netsh advfirewall firewall delete rule name=\"dr_utils tcpserv\" )."
},
{
"h": "Tests",
"p": "Building and installing",
"u": "install.html#tests",
"t": "The tests touch only 127.0.0.1, a local HTTP server they start, and (for getmac and arpscan ) one host already in the machine's ARP table. The benchmark tests use small sizes. tests/run_tests.sh # Linux, after build.sh powershell -ExecutionPolicy Bypass -File tests\\run_tests.ps1 # Windows, after build.cmd powershell -ExecutionPolicy Bypass -File tests\\run_tests.ps1 -NoLan # skip the ARP table check tests/hooks_e2e_test.sh runs the dashboard path end to end on one Linux machine, see Dashboard ."
},
{
"h": "Layout",
"p": "Building and installing",
"u": "install.html#layout",
"t": "Path What tools/<name>/ one folder per file or network tool: the source, makefile (Linux), msvc.mak (Windows), project.mak (the objects for both) benchmarks/<name>/ one folder per benchmark program, laid out the same way benchmarks/plot/ dr_plot.py and dr_report.py benchmarks/scripts/ the Linux watch scripts system_stats/sar_cpu_stats/ the sar report tool hooks/ reporting agent hook files, DRSM manifests and install scripts common/ shared code: ping, connects, DNS, ARP, services files, benchmark sockets, timers, plot files, DRSM lines env/ linux.mak and msvc.mak , the build settings on top of DataReel's tests/ run_tests.sh , run_tests.ps1 , hooks_e2e_test.sh , and short sar reports in tests/data/sar/"
},
{
"h": "File tools",
"p": "File tools",
"u": "file-tools.html",
"t": ""
},
{
"h": "File tools",
"p": "File tools",
"u": "file-tools.html#file-tools",
"t": "Three small tools for scripts and scheduled jobs: is a file stale, purge old files, and show file times. They are based on work from the 2005 file tools and keep their output lines ( YES , NOT , ERR , LST ), so existing scripts keep working. Ages are measured from the file's modification time. Hours and days can be fractional: 0.5 hours is 30 minutes, 2.5 days is 60 hours."
},
{
"h": "file_hours",
"p": "File tools",
"u": "file-tools.html#file_hours",
"t": "Says whether a file was last changed numhours ago or more. file_hours [-s] filename numhours Option Meaning filename the file to check numhours the age in hours: 0.5 , 1 , 2.5 , 36 ... -s exit status 0 for YES, 1 for NOT, 2 for ERR (for if tests without reading the output) Output, one line: Line Meaning YES the file is numhours old or older NOT the file is newer ERR - <reason> the file is missing, is not a file, or has a bad time stamp Dashboard: --drsm=fname , --submit[=ID] (default dru_file_hours ), --now , --drra=path send the result to DRSM through the reporting agent, see Dashboard . Exit status without -s : 0 for YES or NOT, 1 for ERR. Linux: file_hours /var/spool/feed/latest.dat 2 # alert when the feed file has not changed in 2 hours if [ \"$(file_hours /var/spool/feed/latest.dat 2)\" = \"YES\" ]; then echo \"feed is stale\" | mail -s \"feed stale\" ops@example.com fi # the same with the exit status if file_hours -s /var/spool/feed/latest.dat 2; then echo \"stale\"; fi Windows, cmd.exe : file_hours C:\\spool\\feed\\latest.dat 0.5 file_hours -s C:\\spool\\feed\\latest.dat 2 if %ERRORLEVEL%==0 echo feed is stale Windows, PowerShell: if ((file_hours C:\\spool\\feed\\latest.dat 2) -eq 'YES') { Write-Warning 'feed is stale' } file_hours -s C:\\spool\\feed\\latest.dat 2; if ($LASTEXITCODE -eq 2) { 'cannot check the file' }"
},
{
"h": "file_purge",
"p": "File tools",
"u": "file-tools.html#file_purge",
"t": "Deletes or moves files that are a number of hours or days old, or lists their ages. file_purge [switches] file [file ...] Switch Meaning -h<n> delete each file n hours old or older ( -h7 , -h0.5 ) -d<n> delete each file n days old or older ( -d30 , -d2.5 ) -m\"dir\" with -h or -d : move the file to dir instead of deleting it. Across file systems it is a copy and delete. A file of the same name in dir is never overwritten -e delete the file if it exists, with no age check -l list each file's age in hours and days, change nothing -n dry run: print what would be done ( DRY - ... ), change nothing -v verbose output -? the usage text file names can have wildcards ( *.log ), on Windows too. Directories are never purged, even when a wildcard matches them. One of -h , -d , -e or -l is needed. Each file gets one line: Line Meaning YES - name purged (n) hours deleted, or moved ... to dir NOT - name not purged (n) hours too new, or the delete or move failed ( error deleting file ), or the target name already exists in the -m folder ERR - name <reason> missing, not a file, bad time stamp LST - name (n) hours (n) days the -l listing DRY - name would be purged ... the -n dry run Dashboard: --drsm=fname , --submit[=ID] (default dru_file_purge ), --now , --drra=path send the result to DRSM through the reporting agent, see Dashboard . Exit status: 0 when the files were checked (look at the lines for what happened to each), 1 on a usage error (bad switch, no file name, missing -m folder). Run with -n first, every time. Linux: file_purge -l /var/log/myapp/*.log # ages only file_purge -n -d30 /var/log/myapp/*.log # what a 30 day purge would delete file_purge -d30 /var/log/myapp/*.log # delete them file_purge -n -m\"/archive/myapp\" -h12 /var/spool/myapp/*.dat file_purge -m\"/archive/myapp\" -h12 /var/spool/myapp/*.dat file_purge -e /var/run/myapp/stale.lock # delete if it exists A daily cron line ( /etc/cron.d/myapp_purge ): 15 3 * * * root /usr/local/bin/file_purge -d30 /var/log/myapp/*.log > /var/log/myapp_purge.log 2>&1 Windows, cmd.exe : file_purge -l C:\\logs\\myapp\\*.log file_purge -n -d30 C:\\logs\\myapp\\*.log file_purge -d30 C:\\logs\\myapp\\*.log file_purge -n -m\"D:\\archive\\myapp\" -h12 C:\\spool\\myapp\\*.dat file_purge -m\"D:\\archive\\myapp\" -h12 C:\\spool\\myapp\\*.dat Windows, PowerShell (quote the whole -m switch so PowerShell passes it as one argument): file_purge -n '-mD:\\archive\\my app' -h12 C:\\spool\\myapp\\*.dat A daily scheduled task (administrator prompt): schtasks /Create /TN \"myapp log purge\" /SC DAILY /ST 03:15 /RU SYSTEM /TR \"\\\"C:\\Program Files\\dr_utils\\file_purge.exe\\\" -d30 C:\\logs\\myapp\\*.log\""
},
{
"h": "filetimes",
"p": "File tools",
"u": "file-tools.html#filetimes",
"t": "Shows a file's access, modification and creation times in local time and GMT, and its age. With several files it can print just the newest or the oldest. filetimes [-n | -o] file [file ...] Option Meaning -n print only the name of the newest file (by modification time) -o print only the name of the oldest file Without -n or -o each file gets a block like this (on Linux the third time is the inode change time, File change time (ctime) ; on Windows it is File creation time ): Reading time stamps for: a.log File access time: 11/19/2026 01:11 PM File access GMT : Thu, 19 Nov 2026 18:11:41 GMT File modification time: 11/19/2026 01:11 PM File modification GMT : Thu, 19 Nov 2026 18:11:41 GMT File change time (ctime): 11/19/2026 01:11 PM File change GMT (ctime): Thu, 19 Nov 2026 18:11:41 GMT File is 0 second(s) old Dashboard: --drsm=fname , --submit[=ID] (default dru_filetimes ), --now , --drra=path send the result to DRSM through the reporting agent, see Dashboard . Exit status 1 if any file cannot be read, or on a usage error. Linux: filetimes /etc/hosts filetimes -n build/*.tar.gz # the newest build latest=$(filetimes -n /data/feed/*.dat) && process_feed \"$latest\" filetimes -o /var/log/myapp/*.log # the oldest log Windows, cmd.exe : filetimes C:\\Windows\\System32\\drivers\\etc\\hosts filetimes -n C:\\builds\\*.zip for /f \"delims=\" %f in ('filetimes -n C:\\data\\feed\\*.dat') do set LATEST=%f (In a .cmd file write %%f for %f .) Windows, PowerShell: $latest = filetimes -n C:\\data\\feed\\*.dat filetimes -o C:\\logs\\myapp\\*.log"
},
{
"h": "Network tools",
"p": "Network tools",
"u": "network-tools.html",
"t": ""
},
{
"h": "Network tools",
"p": "Network tools",
"u": "network-tools.html#network-tools",
"t": "Seven tools for checking connectivity and finding what is on a network. They are based on work from the 1999-2010 network scan programs and keep their output lines ( Contacting host: , TCP port n open ...), so existing scripts keep working. Use them only on networks and hosts you own or are allowed to scan. The range tools scan one /24 at most. Ranges. pingscan , dnsscan and arpscan take a range: Range Addresses 192.0.2 192.0.2.1 to 192.0.2.254 192.0.2.7 one address 192.0.2.10-40 192.0.2.10 to 192.0.2.40 Services files. portscan and netpscan name open ports from a services file. The default is /etc/services on Linux and %SystemRoot%\\System32\\drivers\\etc\\services on Windows. Name another file as the last argument. Privileges. See Building and installing . On Linux a ping needs net.ipv4.ping_group_range to include the user's group (the RHEL default), or root. On Windows nothing needs administrator rights."
},
{
"h": "internet-test",
"p": "Network tools",
"u": "network-tools.html#internet-test",
"t": "Waits until a web server answers over HTTP: DNS lookup, TCP connect and an HTTP response header are all checked. Any HTTP status passes (a 404 still proves the path works). internet-test [switches] server Switch Meaning server a host name ( www.example.com ), host:port , or a URL ( http://www.example.com:8080/ ) -c n try n times; 0 (the default) tries until it passes -w sec seconds between tries (default 60) -t ms connect timeout in milliseconds (default 5000) -p port HTTP port (default 80) -q print only the final result Output: Internet connectivity test passed (www.example.com, HTTP status 200) , or ... failed after n tries with the reason. Dashboard: --drsm=fname , --submit[=ID] (default dru_internet_test ), --now , --drra=path send the result to DRSM through the reporting agent, see Dashboard . Exit status: 0 passed, 1 every try failed, 2 usage error. Linux: internet-test -c 3 -w 10 www.example.com internet-test -q -c 10 -w 30 www.example.com && /usr/local/bin/start_sync.sh internet-test -p 8080 -t 2000 -c 1 intranet.example.com internet-test -c 1 http://192.0.2.20:8080/ A boot job that waits for the network before starting a sync ( internet-test with -c 0 waits as long as it takes): @reboot root /usr/local/bin/internet-test -q -w 30 www.example.com && /usr/local/bin/start_sync.sh Windows, cmd.exe : internet-test -c 3 -w 10 www.example.com internet-test -q -c 10 -w 30 www.example.com && C:\\jobs\\start_sync.cmd Windows, PowerShell: internet-test -q -c 5 -w 10 www.example.com if ($LASTEXITCODE -ne 0) { Write-Warning 'no Internet' }"
},
{
"h": "pingscan",
"p": "Network tools",
"u": "network-tools.html#pingscan",
"t": "Ping sweep of a range, with the DNS name and reply time of each host. pingscan [-t ms] [-u] range Switch Meaning -t ms ping timeout in milliseconds (default 1000) -u list only the hosts that answer Each address gets a Contacting host: block ( Hostname: , then Host is up, reply in n ms or Host not responding. ). The output ends with n of m hosts answered . One host that fails does not stop the sweep. Dashboard: --drsm=fname , --submit[=ID] (default dru_pingscan ), --now , --drra=path send the result to DRSM through the reporting agent, see Dashboard . Exit status: 0 if any host answered, 1 if none did, 2 on an error. Linux: pingscan 192.0.2 pingscan -u -t 300 192.0.2.10-40 pingscan -u 192.0.2 | grep '^Contacting host:' Windows: pingscan 192.0.2 pingscan -u -t 300 192.0.2.10-40 > lan_up.txt"
},
{
"h": "dnsscan",
"p": "Network tools",
"u": "network-tools.html#dnsscan",
"t": "Lists the reverse DNS names in a range, with an optional forward check and ping. dnsscan [-a] [-f] [-p] [-t ms] range Switch Meaning -a also list addresses with no DNS name ( no_DNS_entry ) -f check that each name resolves back to the address: fwd_ok , or fwd_mismatch with the address it resolves to -p ping each address too: up (ms) or down -t ms ping timeout in milliseconds (default 1000) Output is one tab-separated line per address: address, name, then the -f and -p columns. 192.0.2.10 www.example.com fwd_ok up (0.4 ms) 192.0.2.11 old-host.example.com fwd_mismatch 192.0.2.99 down Dashboard: --drsm=fname , --submit[=ID] (default dru_dnsscan ), --now , --drra=path send the result to DRSM through the reporting agent, see Dashboard . Exit status: 0 if any address has a name, 1 if none, 2 on an error. Linux: dnsscan 192.0.2 dnsscan -f 192.0.2 | grep fwd_mismatch # stale PTR records dnsscan -a -f -p -t 300 192.0.2.1-60 > dns_192.0.2.tsv Windows: dnsscan -f 192.0.2 dnsscan -a -f -p 192.0.2 > dns_192.0.2.tsv PowerShell, as objects: dnsscan -f -p 192.0.2 | ConvertFrom-Csv -Delimiter \"`t\" -Header Address,Name,Forward,Ping"
},
{
"h": "portscan",
"p": "Network tools",
"u": "network-tools.html#portscan",
"t": "Checks one host: its names and addresses, a ping, and a TCP connect to each port, with the services file entry of each open port. portscan [-t ms] [-n] hostname ports [services_file] Option Meaning hostname a host name or address ports a port ( 22 ), a list ( 22,80,443 ) or a range ( 8000-8100 ); up to 1024 ports -t ms connect and ping timeout in milliseconds (default 2000) -n do not ping the host first services_file the services file for the port names (see above) A host that drops ICMP is still scanned. A filtered port waits only for -t , not the system connect timeout. Contacting host: 192.0.2.10 Hostname: www.example.com Host IP Address: 192.0.2.10 TCP port 22 open Name: ssh Comment: The Secure Shell (SSH) Protocol TCP port 443 open Name: https 2 of 3 TCP ports open Dashboard: --drsm=fname , --submit[=ID] (default dru_portscan ), --now , --drra=path send the result to DRSM through the reporting agent, see Dashboard . Exit status: 0 if any port is open, 1 if none is, 2 on an error. Linux: portscan www.example.com 22,80,443 portscan -n -t 500 192.0.2.10 1-1024 portscan 192.0.2.10 8000-8100 ./my_services portscan -n db.example.com 5432 > /dev/null && echo \"database port is open\" Windows: portscan www.example.com 22,80,443,3389 portscan -n -t 500 192.0.2.10 1-1024 portscan -n db.example.com 1433 > nul && echo database port is open"
},
{
"h": "netpscan",
"p": "Network tools",
"u": "network-tools.html#netpscan",
"t": "Scans a list of addresses for open TCP ports. netpscan [switches] iplist_file [services_file] Option Meaning iplist_file one IPv4 address or host name per line; # starts a comment -p ports scan these ports: 22,80,443 or 1-1024 -A scan every TCP port named in the services file (the default is the named ports below 1024) -t ms connect and ping timeout (default 500) -s skip addresses that do not answer a ping services_file the services file for the port list and names Each open port is printed as port/tcp name aliases # comment , and each address ends with n open TCP port(s) . Dashboard: --drsm=fname , --submit[=ID] (default dru_netpscan ), --now , --drra=path send the result to DRSM through the reporting agent, see Dashboard . Exit status: 0 when the scan ran, 2 on an error. An address list ( servers.txt ): # web and database servers 192.0.2.10 192.0.2.11 db.example.com Linux: netpscan servers.txt netpscan -p 22,80,443,3389 servers.txt netpscan -s -A -t 300 servers.txt > ports_$(date +%F).txt Windows: netpscan servers.txt netpscan -p 22,80,443,3389,5985 servers.txt > ports.txt netpscan -A servers.txt C:\\tools\\services.txt"
},
{
"h": "arpscan",
"p": "Network tools",
"u": "network-tools.html#arpscan",
"t": "Lists the MAC address (and vendor) of each host on a directly attached /24 network. It pings each address, then reads the MAC address from the system ARP table ( /proc/net/arp on Linux, SendARP on Windows). arpscan [switches] range Switch Meaning -i dev Linux: only ARP entries on this interface ( -i eth0 ) -v file vendor code file: one XXXXXX Vendor Name line per MAC prefix (6 hex digits, the DataReel FindVendor format) -o file also write the list to a file -t ms ping timeout in milliseconds (default 1000) -a list hosts with no MAC address too ( no_ARP_entry ) Output is one tab-separated line per host: address, DNS name, MAC address and, with -v , the vendor. 192.0.2.1 gw.example.com 00:00:5E:00:53:01 ICANN, IANA Department 192.0.2.12 printer.example.com 00:00:5E:00:53:12 ICANN, IANA Department Only hosts on a directly attached network have a MAC address. Hosts behind a router show the router's or none. An IEEE OUI list converts to a vendor file, for example on Linux: grep '(hex)' oui.txt | sed -E 's/^([0-9A-F]{2})-([0-9A-F]{2})-([0-9A-F]{2}) +\\(hex\\)\\t+/\\1\\2\\3 /' > maccodes.txt Dashboard: --drsm=fname , --submit[=ID] (default dru_arpscan ), --now , --drra=path send the result to DRSM through the reporting agent, see Dashboard . Exit status: 0 if any MAC address was found, 1 if none, 2 on an error. Linux: arpscan 192.0.2 arpscan -i eth0 -v maccodes.txt -o lan.txt 192.0.2 arpscan -a -t 300 192.0.2.1-50 Windows: arpscan 192.0.2 arpscan -v maccodes.txt -o lan.txt 192.0.2"
},
{
"h": "getmac",
"p": "Network tools",
"u": "network-tools.html#getmac",
"t": "Shows the MAC address of one host on a local network. getmac hostname [ethernet_device] Option Meaning hostname a host name or address on a directly attached network ethernet_device Linux: only an ARP entry on this interface It shows the host's names, pings it, and prints MAC Address: XX:XX:XX:XX:XX:XX . Dashboard: --drsm=fname , --submit[=ID] (default dru_getmac ), --now , --drra=path send the result to DRSM through the reporting agent, see Dashboard . Exit status: 0 with the MAC address, 1 if there is no ARP entry (the host did not answer or is not on a local network), 2 on an error. Linux: getmac 192.0.2.12 getmac printer.example.com eth0 mac=$(getmac 192.0.2.12 | sed -n 's/^MAC Address: //p') Windows has its own getmac.exe that lists the local adapters, and it comes first on the PATH. Run this one by its full path (or install it under another name, see Building and installing ): \"C:\\Program Files\\dr_utils\\getmac.exe\" 192.0.2.12 C:\\src\\dr_utils\\bin\\getmac.exe printer.example.com PowerShell: & 'C:\\Program Files\\dr_utils\\getmac.exe' 192.0.2.12 | Select-String '^MAC Address'"
},
{
"h": "Benchmarks",
"p": "Benchmarks",
"u": "benchmarks.html",
"t": ""
},
{
"h": "Benchmarks",
"p": "Benchmarks",
"u": "benchmarks.html#benchmarks",
"t": "System benchmarks for CPU, floating point, memory, disk, TCP, UDP and multicast. They are based on work from the 2002-2010 bmarks programs and their 2016-2019 versions, with the problems of the old versions fixed (wall clock times, measured UDP and multicast loss, no remote server shutdown, a rate-limited multicast sender, a flops loop the compiler cannot remove). All of them: print their usage with -? or --help and their version with --version ( cputest , disktest , memtest , flops ) or in the first output line; take decimal sizes and counts: 1k = 1000, 1m = 1000000, 1g = 1000000000 ( 150M , 1G too); time with the wall clock; take --datfile=fname to write the results as a plot data file for dr_plot.py and dr_report.py (all but the three servers); take --drsm=fname , --submit[=ID] , --now , --drra=path and --drra-config=ini to send the results to the DRSM dashboard through the reporting agent (all but the three servers), see Dashboard . Compare results only between runs with the same settings and the same build (compiler and flags). Run the network benchmarks only between hosts you own or are allowed to test."
},
{
"h": "cputest",
"p": "Benchmarks",
"u": "benchmarks.html#cputest",
"t": "CPU speed: zlib best compression (level 9) and uncompression of random data, checked after each pass. cputest [--num-threads=NUM] [--buf-size=SIZE] [--repeat=NUM] [--datfile=fname] [dashboard options] Option Meaning --num-threads=NUM threads (default 1, at most 1024) --buf-size=SIZE the buffer of random bytes, split between the threads: 24k , 240m , 2.4g (default 100m ) --repeat=NUM passes over the buffer (default 1) --datfile=fname write a plot data file Output: each thread's time, the average, and the total time with the rate in MB/s. Threads: 2, buffer 10M bytes, 1 pass(es) Thread[1]: CPU speed test elapsed time 0.229 seconds Thread[0]: CPU speed test elapsed time 0.244 seconds Average thread time 0.237 seconds Total elapsed time 0.245 seconds, 40.88 MB/s Run it with one thread for single-core speed and one thread per core for the whole CPU. Linux: cputest cputest --num-threads=$(nproc) --buf-size=1g --datfile=cpu_$(hostname -s).dat cputest --num-threads=1 --repeat=5 Windows: cputest cputest --num-threads=%NUMBER_OF_PROCESSORS% --buf-size=1g --datfile=cpu_%COMPUTERNAME%.dat PowerShell: cputest --num-threads=$env:NUMBER_OF_PROCESSORS --buf-size=1g"
},
{
"h": "flops",
"p": "Benchmarks",
"u": "benchmarks.html#flops",
"t": "Floating point operations per second: multiply-add chains over a small array that stays in the CPU cache. flops [--num-threads=NUM] [--dp] [--div] [--iterations=NUM] [--datfile=fname] [dashboard options] Option Meaning --num-threads=NUM threads (default 1, at most 1024); the iterations are split between them --dp double precision (the default is single precision) --div the formula of the old c/flops program, a*c + a/b : a multiply, a divide and an add, counted as 3 operations. Code with divisions runs slower than multiply-add, so this gives the rate such code gets --iterations=NUM loop iterations in all: 1m , 1g , 10g (default 1g ) --datfile=fname write a plot data file Output: each thread's time and the total rate in MFLOPS or GFLOPS. The rate depends on the compiler and its flags (vector units). Linux: flops flops --num-threads=$(nproc) --dp --iterations=10g flops --div --dp --datfile=flops_div.dat Windows: flops --num-threads=%NUMBER_OF_PROCESSORS% --dp --iterations=10g flops --div --datfile=flops_div.dat"
},
{
"h": "memtest",
"p": "Benchmarks",
"u": "benchmarks.html#memtest",
"t": "Memory speed: each thread builds its own binary search tree of \"Item n\" string keys and times the insert, find, delete and insert-again phases. memtest [--num-threads=NUM] [--num-inserts=NUM] [--datfile=fname] [dashboard options] Option Meaning --num-threads=NUM threads (default 1, at most 1024) --num-inserts=NUM keys per thread: 1k , 25k , 100k (default 25k , at most 1m ) --datfile=fname write a plot data file The keys arrive nearly sorted, so the tree is deep and the time grows faster than the key count: compare runs with the same --num-inserts . Threads: 1, 5000 key insertions per thread Thread[0]: Inserted 5000 values in 0.013 seconds Thread[0]: Found 5000 values in 0.011 seconds Thread[0]: Deleted 5000 values in 0.007 seconds Thread[0]: Reinserted 5000 values in 0.012 seconds Average thread times: insert 0.013, find 0.011, delete 0.007, reinsert 0.012 seconds Total elapsed time 0.044 seconds Linux and Windows (the same command lines): memtest memtest --num-threads=4 --num-inserts=50k --datfile=mem.dat"
},
{
"h": "disktest",
"p": "Benchmarks",
"u": "benchmarks.html#disktest",
"t": "Disk speed for database index work: a DataReel B-tree index file (order 128) gets NUM key inserts, finds, deletes (each delete checked) and inserts again, with the time at every 10%. disktest [--num-inserts=NUM] [--file=fname] [--keep] [--datfile=fname] [dashboard options] Option Meaning --num-inserts=NUM keys: 1k , 1m , 1g (default 1m ) --file=fname the B-tree test file (default testfile.btx in the current folder). Put it on the disk you want to test. An existing file is never overwritten --keep keep the test file (by default it is removed at the end) --datfile=fname write the times in the disk benchmark plot format ( [DISKBENCHMARK] , [PLOTINFO] , [HOSTINFO] ) The output has a line at every 10% of each phase, the phase total with keys a second, and the B-tree statistics: Reinserted 10000 keys: 0.072 seconds, 137998 keys a second Root address = 54264 Number of entries = 10000 B-Tree order = 128 B-Tree height = 3 Disk speed test elapsed time 0.271 seconds Linux: disktest --file=/data/testfile.btx disktest --num-inserts=10m --file=/mnt/nfs01/testfile.btx --datfile=disk_nfs01.dat Windows: disktest --file=D:\\bench\\testfile.btx disktest --num-inserts=10m --file=\\\\fileserver.example.com\\share\\testfile.btx --datfile=disk_share.dat"
},
{
"h": "tcpserv and tcptest",
"p": "Benchmarks",
"u": "benchmarks.html#tcpserv-and-tcptest",
"t": "TCP bandwidth, upload or download, with one or more clients. Start tcpserv on one host, then run tcptest on another. tcpserv [-p port] [-l address] [--count=N] [--timeout=S] [--max-bytes=SIZE] tcptest [-b size] [-d] [-n clients] [-p port] [-s bytes] [--datfile=fname] [dashboard options] server tcpserv : Switch Meaning -p<port> port, 1024 to 65535 (default 53897) -l<address> listen on this IPv4 address only (default: all addresses) --count=N exit after N transfers; each client is one transfer (default: run until Ctrl-C) --timeout=S close a client idle for S seconds (default 30) --max-bytes=SIZE the largest transfer accepted (default 100g ) tcptest : Switch Meaning -b<size> write or read size, 64 to 1048576 (default 1500) -d download from the server (default: upload to it) -n<clients> clients, 1 to 1000 (default 1) -p<port> port (default 53897) -s<bytes> bytes per client, 1K to 100G (default 150M ) --datfile=fname write a plot data file The server confirms each byte count. Each client and the total print their rates. Clients cannot stop or restart the server. Exit status of tcptest : 0 if every client finished, 1 otherwise. Allow TCP 53897 in the server's firewall (see install ). Linux, server and client: tcpserv # on 192.0.2.10, Ctrl-C to stop tcpserv -l192.0.2.10 --count=4 # exit after 4 transfers tcptest 192.0.2.10 # one client, 150M upload tcptest -n4 -b65536 -s1G 192.0.2.10 # four clients, 1G each tcptest -d -n4 -b65536 -s1G --datfile=tcp_down.dat 192.0.2.10 Windows, the same switches: tcpserv -p53897 tcptest -n4 -b65536 -s1G --datfile=tcp_up.dat 192.0.2.10 Example output: Connecting 4 clients to 192.0.2.10 port 53897, 1G bytes each, 65536 byte buffers, upload Client[0]: sent 1000000000 bytes in 9.312 seconds, 107.39 MB/s ... All tests complete: 4 of 4 clients, 4000000000 bytes in 36.950 seconds, 108.25 MB/s"
},
{
"h": "udpserv and udptest",
"p": "Benchmarks",
"u": "benchmarks.html#udpserv-and-udptest",
"t": "UDP bandwidth and loss with one or more clients. udpserv [-p port] [-l address] [--count=N] udptest [-b size] [-n clients] [-p port] [-r rate] [-s bytes] [--datfile=fname] [dashboard options] server udpserv : Switch Meaning -p<port> port (default 53897) -l<address> listen on this IPv4 address only --count=N exit after N client runs (default: run until Ctrl-C) udptest : Switch Meaning -b<size> datagram size, 32 to 65507 (default 1500) -n<clients> clients, 1 to 1000 -p<port> port (default 53897) -r<rate> send rate per client in bits a second: -r100M , -r1G (default: as fast as the host can) -s<bytes> bytes per client, 1K to 100G (default 150M ) --datfile=fname write a plot data file The clients send numbered datagrams. At the end the server reports what arrived, and each client prints its send rate, the server's receive rate, the loss and the out-of-order count. Exit status of udptest : 0 if every client got a report, 1 otherwise. Allow UDP 53897 in the server's firewall. Client[0]: sent 3333 datagrams in 0.199 seconds, 25.17 MB/s; server received 3333 (0.00% lost, 0 out of order), 25.18 MB/s All tests complete: 1 of 1 clients reported, 3333 of 3333 datagrams received (0.00% lost), 25.18 MB/s received Without -r the loss shows what the network and the server dropped at full speed; with -r you can find the rate a link carries without loss. Linux: udpserv # on 192.0.2.10 udptest -s100M -r500M 192.0.2.10 udptest -n4 -b8972 -r1G --datfile=udp.dat 192.0.2.10 # jumbo frames (MTU 9000) Windows: udpserv -p53897 udptest -s100M -r500M 192.0.2.10"
},
{
"h": "mcserv and mctest",
"p": "Benchmarks",
"u": "benchmarks.html#mcserv-and-mctest",
"t": "Multicast rate, loss and order from one sender ( mcserv ) to one or more receivers ( mctest ). Start the receivers first. mctest [-g group] [-i address] [-n receivers] [-p port] [--timeout=S] [--datfile=fname] [dashboard options] [group] mcserv [-b size] [-g group] [-i address] [-p port] [-r rate] [-s bytes] [--seconds=N] [--ttl=N] [--no-loop] mctest (receiver): Switch Meaning -g<group> or group multicast group (default 239.255.10.10) -i<address> join on the interface with this IPv4 address -n<number> receivers in this process, 1 to 100 -p<port> port (default 53898) --timeout=S stop when nothing arrives for S seconds (default 60) --datfile=fname write a plot data file mcserv (sender): Switch Meaning -b<size> datagram size, 32 to 65507 (default 1500) -g<group> multicast group, 224.0.0.0 to 239.255.255.255 (default 239.255.10.10) -i<address> send from the interface with this IPv4 address -p<port> port (default 53898) -r<rate> send rate in bits a second: -r10M , -r1G (default 100M ) -s<bytes> bytes to send, 1K to 100G (default 150M ) --seconds=N stop after N seconds even if bytes are left --ttl=N multicast TTL, 1 to 32 (default 1: the local network only) --no-loop do not deliver to receivers on this host The sender sends numbered datagrams at the rate, then end-of-run datagrams. Each receiver prints the datagrams, loss, out-of-order count and rate. The defaults are cautious: TTL 1 and an administratively scoped group keep the traffic on the local network. Exit status of mctest : 0 if every receiver saw the end of a run, 1 otherwise. Allow UDP 53898 on the receivers. Linux, on two hosts of one network: mctest -i192.0.2.21 --datfile=mc.dat # receiver on 192.0.2.21 mcserv -i192.0.2.10 -r200M -s500M # sender on 192.0.2.10 On one host (loopback, a quick check): mctest -i127.0.0.1 & mcserv -i127.0.0.1 -s10M Windows, the same switches (start the receiver in its own prompt): start \"mctest\" mctest -i192.0.2.21 --datfile=mc.dat mcserv -i192.0.2.10 -r200M -s500M Example output: Receiver[0]: joined multicast group 239.255.10.10 port 53898 Receiver[0]: run 1435795080: 3333 of 3333 datagrams (0.00% lost, 0 out of order), 4999500 bytes in 0.399 seconds, 12.52 MB/s All tests complete: 1 of 1 receivers saw the end of a run"
},
{
"h": "A sizing run",
"p": "Benchmarks",
"u": "benchmarks.html#a-sizing-run",
"t": "Linux, one folder of results for a host, then an HTML report (see Plots and reports ): mkdir -p results cputest --num-threads=$(nproc) --datfile=results/cpu.dat flops --num-threads=$(nproc) --dp --datfile=results/flops.dat memtest --num-threads=4 --datfile=results/mem.dat disktest --file=/data/testfile.btx --datfile=results/disk.dat tcptest -n4 -s1G --datfile=results/tcp.dat 192.0.2.10 dr_report.py -o sizing.html --title \"sizing $(hostname -s)\" results Windows: mkdir results cputest --num-threads=%NUMBER_OF_PROCESSORS% --datfile=results\\cpu.dat flops --num-threads=%NUMBER_OF_PROCESSORS% --dp --datfile=results\\flops.dat memtest --num-threads=4 --datfile=results\\mem.dat disktest --file=D:\\bench\\testfile.btx --datfile=results\\disk.dat tcptest -n4 -s1G --datfile=results\\tcp.dat 192.0.2.10 py C:\\src\\dr_utils\\benchmarks\\plot\\dr_report.py -o sizing.html --title \"sizing %COMPUTERNAME%\" results"
},
{
"h": "sar CPU statistics",
"p": "sar CPU statistics",
"u": "sar-cpu-stats.html",
"t": ""
},
{
"h": "sar_cpu_stats",
"p": "sar CPU statistics",
"u": "sar-cpu-stats.html#sar_cpu_stats",
"t": "Reads sar text reports and prints CPU use: averages and peaks, steal and iowait spikes, gaps in the data and impossible samples. With --csv it writes every sample for plotting. It is based on work from the 2018 system_stats_analysis_tool read_cpu_stats . sar_cpu_stats [args] sar_file [sar_file ...] Option Meaning --csv write every sample as CSV instead of the summary --all-cpus with --csv : rows for each CPU too, not only all --steal=PCT steal spike threshold (default 10) --iowait=PCT iowait spike threshold (default 20) --list=N spikes, gaps and bad samples listed per file (default 20) --alarm exit status 3 when there are spikes, gaps or bad samples --help , --version usage, version --drsm , --submit , --now , --drra , --drra-config dashboard output, see Dashboard Exit status: 0; 1 on an error or no CPU samples; 3 with --alarm when something was found."
},
{
"h": "Input",
"p": "sar CPU statistics",
"u": "sar-cpu-stats.html#input",
"t": "sar text reports from sysstat on RHEL 5 to RHEL 10 (and other distributions): one day or many days in a file, 12- or 24-hour times, %user or %usr , with or without the newer columns ( %steal , %irq , %soft , %guest , %gnice ). Make one with: LC_ALL=C sar -u -f /var/log/sa/sa19 > sar_19.txt # all CPUs, one day LC_ALL=C sar -P ALL -f /var/log/sa/sa19 > sar_19_cpus.txt # each CPU too LC_ALL=C sar -A -f /var/log/sa/sa19 > sar_19_all.txt # a whole report; only the CPU part is read (On Debian and Ubuntu the files are in /var/log/sysstat/ .) LC_ALL=C gives the dates and times the tool reads. The reports are plain text, so the Windows build reads reports copied from Linux hosts the same way."
},
{
"h": "Summary output",
"p": "sar CPU statistics",
"u": "sar-cpu-stats.html#summary-output",
"t": "For the all CPU rows of each file: the average and peak of user, nice, system, iowait, steal, irq, soft, guest, gnice, and the lowest idle, with the peak times; steal and iowait spikes over the thresholds; gaps: a missing run of samples inside a day, or a day whose samples stop early while later days follow (a host or VM that stopped writing); bad samples: a percent over 100, or parts that do not add up to about 100. A VM the hypervisor stopped can write a steal of 241%. Bad samples are listed and left out of the averages. Reading sar_steal_spike.txt bad sample at 2018-09-24 00:00:00: steal 241.45, iowait 23.50, the parts add up to 438.4% gap: samples stop at 2018-09-24 00:10:01 Host: node1.example.com, kernel 2.6.32-696.28.1.el6.x86_64 Days: 2 (2018-09-24 to 2018-09-25), 2 samples of all CPUs, every 10 minutes average peak peak time user 5.96 6.61 2018-09-24 00:10:01 system 8.46 9.93 2018-09-24 00:10:01 iowait 17.19 18.80 2018-09-24 00:10:01 steal 0.00 0.00 - idle 67.84 64.00 2018-09-24 00:10:01 (lowest) Steal >= 10.0%: 0, iowait >= 20.0%: 0, gaps: 1, bad samples: 2 (Some rows are left out above.)"
},
{
"h": "CSV output",
"p": "sar CPU statistics",
"u": "sar-cpu-stats.html#csv-output",
"t": "Columns: host,time,cpu,user,nice,system,iowait,steal,irq,soft,guest,gnice,idle,sample . A column the report does not have is empty. sample is ok or bad . host,time,cpu,user,nice,system,iowait,steal,irq,soft,guest,gnice,idle,sample node1.example.com,2026-11-19 04:10:24,all,17.02,11.14,4.01,19.40,0.00,,,,,48.44,ok dr_plot.py and dr_report.py plot this CSV (see Plots and reports )."
},
{
"h": "Examples",
"p": "sar CPU statistics",
"u": "sar-cpu-stats.html#examples",
"t": "Linux: LC_ALL=C sar -u -f /var/log/sa/sa$(date -d yesterday +%d) > sar_yesterday.txt sar_cpu_stats sar_yesterday.txt sar_cpu_stats --steal=5 --iowait=30 --list=50 sar_*.txt sar_cpu_stats --csv sar_yesterday.txt > cpu.csv && dr_plot.py -o cpu.png cpu.csv sar_cpu_stats --csv --all-cpus sar_19_cpus.txt > cpus.csv && dr_plot.py --cpu 3 -o cpu3.png cpus.csv A cron check that mails only when something is wrong: 30 0 * * * root f=/tmp/sar_$$.txt; LC_ALL=C sar -u -f /var/log/sa/sa$(date -d yesterday +\\%d) > $f; /usr/local/bin/sar_cpu_stats --alarm $f > $f.out || mail -s \"sar CPU alarm\" ops@example.com < $f.out; rm -f $f $f.out (To send the result to the dashboard every day instead, use the ready-made job in Dashboard .) Windows, with reports copied from Linux hosts: sar_cpu_stats C:\\reports\\sar\\node1_*.txt sar_cpu_stats --csv C:\\reports\\sar\\node1_19.txt > node1.csv py C:\\src\\dr_utils\\benchmarks\\plot\\dr_plot.py -o node1.png node1.csv sar_cpu_stats --alarm C:\\reports\\sar\\*.txt if %ERRORLEVEL%==3 echo spikes, gaps or bad samples found Short example reports are in tests/data/sar/ (RHEL 5, RHEL 7 with AM/PM times, RHEL 10, a gap, a steal spike)."
},
{
"h": "Plots, reports, watch scripts",
"p": "Plots, reports, watch scripts",
"u": "plots-and-reports.html",
"t": ""
},
{
"h": "Plots, reports and watch scripts",
"p": "Plots, reports, watch scripts",
"u": "plots-and-reports.html#plots-reports-and-watch-scripts",
"t": ""
},
{
"h": "Plot data files (--datfile)",
"p": "Plots, reports, watch scripts",
"u": "plots-and-reports.html#plot-data-files---datfile",
"t": "cputest , disktest , memtest , flops , tcptest , udptest and mctest take --datfile=fname and write their results as a small INI file: the settings, the main result ( total ), one value per thread, client, receiver or phase, and a [HOSTINFO] section (host, date, compiler). For example cputest --num-threads=2 --datfile=cpu.dat : [BENCHMARK] name=cputest version=1.4 threads=2 buf_size=1e+07 repeat=1 total=40.8762 total_unit=MB/s total_label=zlib compress + uncompress rate elapsed=0.244641 items=Thread 0,Thread 1 item_values=0.243874,0.229364 item_unit=seconds [HOSTINFO] host=host1.example.com date=11/19/2026 build=GCC title=CPU Speed Benchmark disktest --datfile writes the older disk benchmark format ( [DISKBENCHMARK] , [PLOTINFO] , [HOSTINFO] ) with the time at every 10% of each phase. The files are the same on Windows and Linux, so results from both plot and report together."
},
{
"h": "dr_plot.py",
"p": "Plots, reports, watch scripts",
"u": "plots-and-reports.html#dr_plotpy",
"t": "Plots --datfile files, sar_cpu_stats --csv output and watch_mem_usage.sh logs. It tells the kind from the contents. Needs Python 3 and matplotlib. dr_plot.py [-h] [-o OUT] [--averages] [--cpu CPU] [--dpi DPI] files [files ...] Option Meaning files one or more input files -o OUT , --out OUT write this PNG file (without -o the plot opens in a window) --averages disktest: milliseconds per key instead of seconds per 10% of the keys --cpu CPU sar CSV: the CPU to plot (default all ; needs --csv --all-cpus input) --dpi DPI PNG resolution Input Plot one benchmark --datfile a bar per thread, client, receiver or phase, the total in the title; UDP and multicast also show the loss per client several files of one benchmark one bar per run (host, date, threads or clients) for the main result: compare machines, builds or thread counts disktest --datfile insert, find, delete and reinsert times for every 10% of the keys; several files compare the phase totals sar_cpu_stats --csv CPU use over time, stacked (user, nice, system, irq + soft, iowait, steal), one panel per host, gaps left empty and bad samples marked in red watch_mem_usage.sh log used memory over time Linux: dr_plot.py cpu_host1.dat # a window dr_plot.py -o cpu.png cpu_host1.dat dr_plot.py -o cpu_cmp.png cpu_host1.dat cpu_host2.dat # compare runs dr_plot.py --averages -o disk_ms.png disk.dat sar_cpu_stats --csv sar_host1.txt > host1.csv && dr_plot.py -o host1_cpu.png host1.csv watch_mem_usage.sh -i 5 > mem.log & memtest --num-threads=8; kill %1; dr_plot.py -o mem.png mem.log Windows ( py launcher, from the dr_utils folder): py benchmarks\\plot\\dr_plot.py -o cpu.png cpu_host1.dat py benchmarks\\plot\\dr_plot.py -o cpu_cmp.png results\\cpu_*.dat py benchmarks\\plot\\dr_plot.py --averages --dpi 150 -o disk.png disk.dat On a Linux host with no display, use -o ."
},
{
"h": "dr_report.py",
"p": "Plots, reports, watch scripts",
"u": "plots-and-reports.html#dr_reportpy",
"t": "Builds one HTML report from a folder (or a list) of result files: --datfile files, sar_cpu_stats --csv files and watch_mem_usage.sh logs. The charts are inside the HTML file, so the one file can be mailed, archived or printed. dr_report.py [-h] -o OUT [--title TITLE] [--csv CSV] [--no-runs] [--submit [ID]] [--now] [--drra DRRA] [--drra-config DRRA_CONFIG] inputs [inputs ...] Option Meaning inputs result files or folders -o OUT , --out OUT the HTML file to write (required) --title TITLE the report title (default \"Benchmark report\") --csv CSV also write the summary table as CSV --no-runs only the comparison charts, not one chart per run --submit [ID] hand the report to the reporting agent, hook ID (default dru_report ), see Dashboard --now with --submit , the agent sends within seconds --drra DRRA agent program (default drra , or %ProgramFiles%\\DataReel\\ReportingAgent\\drra_svc.exe on Windows) --drra-config DRRA_CONFIG agent settings file, drra -c INI (Linux, for tests) The report has: a summary table of every run (benchmark, host, date, compiler, settings, result); a section for each benchmark, with a comparison chart when there are several runs and a chart for each run; for each sar CSV, a table of averages and peaks per host (user, system, iowait, steal, lowest idle), the count of bad samples, and the CPU chart; the memory logs. Files in a folder that are not results are listed as skipped. Linux: mkdir results cputest --num-threads=8 --datfile=results/cpu.dat disktest --file=/data/testfile.btx --datfile=results/disk.dat sar_cpu_stats --csv sar_host1.txt > results/host1_sar.csv dr_report.py -o host1_report.html --title \"host1 sizing\" --csv host1_summary.csv results dr_report.py -o compare.html --no-runs host1_results host2_results Windows: py benchmarks\\plot\\dr_report.py -o report.html --title \"file server sizing\" results py benchmarks\\plot\\dr_report.py -o compare.html --no-runs --csv compare.csv \\\\fileserver.example.com\\bench\\host1 \\\\fileserver.example.com\\bench\\host2"
},
{
"h": "Watch scripts (Linux)",
"p": "Plots, reports, watch scripts",
"u": "plots-and-reports.html#watch-scripts-linux",
"t": "In benchmarks/scripts/ . Run them in a second terminal (or in the background) while a benchmark runs. All three take -i seconds (default 1), -n count (default 0: until Ctrl-C) and STATSONLY . Script What it prints watch_system.sh [-i sec] [-n count] [STATSONLY] the release, kernel, CPU, memory, partitions, mounts and disk statistics from /proc, each between START name and END name lines, then top -b until Ctrl-C, and the statistics again at the end. STATSONLY : the statistics once, no top watch_mem.sh [-i sec] [-n count] [STATSONLY] free and /proc/meminfo every interval, and once more at the end. STATSONLY : /proc/meminfo once watch_mem_usage.sh [-i sec] [-n count] [STATSONLY] one YYYY-MM-DD HH:MM:SS used-KB line per interval, for dr_plot.py and dr_report.py . STATSONLY : one line benchmarks/scripts/watch_system.sh STATSONLY > host1_system.txt benchmarks/scripts/watch_system.sh -i 5 > host1_during_disktest.txt & disktest --file=/data/testfile.btx --datfile=results/disk.dat kill %1 benchmarks/scripts/watch_mem_usage.sh -i 2 > results/mem.log & memtest --num-threads=8 --num-inserts=100k --datfile=results/mem.dat kill %1 dr_report.py -o mem_report.html results On Windows use Performance Monitor or typeperf \"\\Memory\\Committed Bytes\" -si 2 -o mem.csv for the same kind of record (the scripts read /proc, so they are Linux only)."
},
{
"h": "Dashboard hooks",
"p": "Dashboard hooks",
"u": "dashboard-hooks.html",
"t": ""
},
{
"h": "Dashboard: reporting agent hooks",
"p": "Dashboard hooks",
"u": "dashboard-hooks.html#dashboard-reporting-agent-hooks",
"t": "The benchmarks, the file and network tools, sar_cpu_stats and dr_report.py can send their results to the DataReel System Monitor (DRSM) dashboard through the DataReel Reporting Agent for Linux (1.0-beta2 and later) or Windows (3.3-beta2 and later). They use the agents' hook uploads : The program writes its results as DRSM lines (or, for dr_report.py , the HTML report). It hands the file to the agent: drra submit --hook <id> file on Linux, drra_svc.exe submit --hook <id> file on Windows. The agent sends it, signed, to hook.php on the report server, with its next report or, with --now , within seconds. A DRSM upload plugin (a manifest from hooks/drsm/uploads/ ) shows it on the host's page, with alert rules and trends. Nothing in the agents or in DRSM changes; dr_utils only adds the hook files for them. The reference for the files is hooks/README.md ."
},
{
"h": "Options",
"p": "Dashboard hooks",
"u": "dashboard-hooks.html#options",
"t": "Every benchmark except the three servers ( tcpserv , udpserv , mcserv ), every file and network tool, and sar_cpu_stats take: Option Meaning --drsm=fname write the DRSM lines to a file (to look at them, or to send later by hand) --submit[=ID] hand the results to the agent, upload hook ID (default dru_<program> , for example dru_cputest ). An ID is a-z , 0-9 and _ --now with --submit : the agent sends within seconds, not with its next report --drra=path the agent program. Linux: drra in the PATH, then /usr/sbin/drra . Windows: %ProgramFiles%\\DataReel\\ReportingAgent\\drra_svc.exe --drra-config=ini Linux only: the agent settings file ( drra -c ini ), for tests. A Windows test copy of the agent reads the drra.ini next to it dr_report.py takes --submit [ID] (default dru_report ), --now , --drra and --drra-config the same way. With both --drsm and --submit , the --drsm file is the one submitted. With --submit alone the lines go through a private temporary folder that is removed afterwards. The exit status is 1 when the agent refuses the file or cannot be run, so a cron job or scheduled task sees it. drra submit needs root or the drra-upload group on Linux, and an administrator on Windows."
},
{
"h": "What each program sends",
"p": "Dashboard hooks",
"u": "dashboard-hooks.html#what-each-program-sends",
"t": "Program Hook ID Values for rules and trends Table cputest dru_cputest total (MB/s), elapsed , threads , buf_size seconds per thread disktest dru_disktest total (seconds), insert_time , search_time , delete_time , reinsert_time , num_inserts seconds and ms per key per phase memtest dru_memtest total (seconds), threads , num_inserts seconds per phase flops dru_flops total (GFLOPS), elapsed , threads , precision , mode seconds per thread tcptest dru_tcptest total (MB/s), clients , bytes , direction MB/s per client udptest dru_udptest total (MB/s), loss_max_pct , clients , rate_limit MB/s and % lost per client mctest dru_mctest total (MB/s per receiver), loss_max_pct , receivers MB/s and % lost per receiver sar_cpu_stats dru_sar_cpu_stats busy_avg , <field>_avg and <field>_peak , idle_min , steal_spikes , iowait_spikes , gaps , bad_samples , files_failed one row per sar file file_hours dru_file_hours stale (1 when the file is the limit old or older), age_hours , limit_hours , result (YES, NOT, ERR), file the file file_purge dru_file_purge purged , would_purge ( -n ), kept , listed ( -l ), errors , files , mode , dry_run a row per file: result and details filetimes dru_filetimes files , errors , newest_file , newest_age_hours , oldest_file , oldest_age_hours modification time and age per file internet-test dru_internet_test passed (1 or 0), tries , seconds , result , server , port pingscan dru_pingscan up , down , tried , range address, name, status, reply ms ( -u : hosts up only) dnsscan dru_dnsscan named , tried , fwd_ok and fwd_mismatch ( -f ), up ( -p ) address, name, forward check, ping portscan dru_portscan open , ports , ping , ping_ms , host , address , and port_<n> ( open or closed , 64 ports or fewer) open ports (every port when there are 16 or fewer) netpscan dru_netpscan open_ports , scanned , up , skipped ( -s ), not_resolved , ports_per_host a row per address: ping, open count, port list arpscan dru_arpscan found , tried , range address, name, MAC address, vendor getmac dru_getmac found (1 or 0), mac , ping , host , address the host dr_report.py dru_report the whole HTML report, charts included Each result also has an INFO line with the main result and settings, and the values benchmark (or program ), version , build and collected . What cputest --num-threads=2 --drsm=cpu.drsm writes: INFO - CPU Speed Benchmark (cputest 1.4): zlib compress + uncompress rate 40.8762 MB/s, threads=2 buf_size=10000000 repeat=1 DETAIL benchmark=cputest DETAIL version=1.4 DETAIL build=GCC DETAIL threads=2 DETAIL buf_size=10000000 DETAIL repeat=1 DETAIL total=40.8762 DETAIL total_unit=MB/s DETAIL elapsed=0.244641 DETAIL collected=2026-11-19T13:11:41-0500 UPLOAD columns|Thread|seconds UPLOAD row|Thread 0|0.243874 UPLOAD row|Thread 1|0.229364"
},
{
"h": "Setting it up",
"p": "Dashboard hooks",
"u": "dashboard-hooks.html#setting-it-up",
"t": ""
},
{
"h": "1. Monitor host (DRSM)",
"p": "Dashboard hooks",
"u": "dashboard-hooks.html#1-monitor-host-drsm",
"t": "Copy the manifests you want to DRSM's upload plugins: cp -r hooks/drsm/uploads/dru_* /path/to/drsm/plugins/uploads/ The report server's hook.php reads them from [hooks] manifest_dir in its server.ini (the same folder on a DRSM install). Each host needs UPLOADS_DIR and UPLOADS=auto (or a list of hook IDs) in its DRSM host file. This is the same setup as every other upload hook; see \"Hook uploads\" in DRSM's PLUGINS.md. For dru_report , the report server's [hooks] max_bytes must allow the report size (reports with many charts can be several MB), and the dashboard needs $UPLOADSdir for the html display."
},
{
"h": "2. Agent host, Linux",
"p": "Dashboard hooks",
"u": "dashboard-hooks.html#2-agent-host-linux",
"t": "The installer is a dry run unless --apply is given. It copies hooks/uploads.d/<id>.ini to /etc/drra/uploads.d/ . sudo hooks/linux/install_hooks.sh all # what it would do sudo hooks/linux/install_hooks.sh --apply all # install every hook file sudo hooks/linux/install_hooks.sh --apply cputest disktest # only these sudo hooks/linux/install_hooks.sh --help A user who runs the benchmarks without sudo needs the drra-upload group: sudo usermod -aG drra-upload benchuser # then log in again"
},
{
"h": "3. Agent host, Windows",
"p": "Dashboard hooks",
"u": "dashboard-hooks.html#3-agent-host-windows",
"t": "From an administrator PowerShell or cmd.exe prompt, in the dr_utils folder. A dry run unless -Apply is given. The hook files go to C:\\ProgramData\\DataReel\\ReportingAgent\\uploads.d\\ . powershell -ExecutionPolicy Bypass -File hooks\\windows\\install_hooks.ps1 all powershell -ExecutionPolicy Bypass -File hooks\\windows\\install_hooks.ps1 all -Apply powershell -ExecutionPolicy Bypass -File hooks\\windows\\install_hooks.ps1 cputest disktest -Apply"
},
{
"h": "4. Send a result and check it",
"p": "Dashboard hooks",
"u": "dashboard-hooks.html#4-send-a-result-and-check-it",
"t": "Linux: cputest --num-threads=8 --submit --now sudo drra uploads # the state of each hook Windows (administrator prompt): cputest --num-threads=8 --submit --now \"%ProgramFiles%\\DataReel\\ReportingAgent\\drra_svc.exe\" uploads The result is on the host's DRSM page after the next DRSM run."
},
{
"h": "Examples",
"p": "Dashboard hooks",
"u": "dashboard-hooks.html#examples",
"t": "Linux: cputest --num-threads=$(nproc) --submit --now disktest --file=/data/testfile.btx --submit flops --dp --num-threads=$(nproc) --submit tcptest -n4 -s1G --submit 192.0.2.10 udptest -s100M -r500M --submit 192.0.2.10 mctest -i192.0.2.21 --submit sar_cpu_stats --submit --now sar_yesterday.txt dr_report.py -o host_report.html --submit results/ # file and network tools file_hours /var/spool/feed/latest.dat 2 --submit file_purge -d30 /var/log/myapp/*.log --submit filetimes -n /data/incoming/*.csv --submit internet-test -c 3 -w 10 www.example.com --submit pingscan -u 192.0.2 --submit dnsscan -f -p 192.0.2 --submit portscan -n www.example.com 22,80,443 --submit netpscan -p 22,80,443 servers.txt --submit arpscan -v vendors.txt 192.0.2 --submit getmac printer.example.com --submit # look first, send later by hand cputest --drsm=cpu.drsm cat cpu.drsm sudo drra submit --hook dru_cputest --now cpu.drsm # a hook under another ID (its .ini and manifest must exist) disktest --file=/san/testfile.btx --submit=dru_disktest_san # a test agent build cputest --submit --drra=/path/to/lra/drra --drra-config=/path/to/test/drra.ini Windows (administrator prompt): cputest --num-threads=%NUMBER_OF_PROCESSORS% --submit --now disktest --file=D:\\bench\\testfile.btx --submit tcptest -n4 -s1G --submit 192.0.2.10 sar_cpu_stats --submit C:\\reports\\sar\\node1_19.txt py benchmarks\\plot\\dr_report.py -o host_report.html --submit results file_hours D:\\feeds\\latest.dat 2 --submit file_purge -d30 C:\\logs\\myapp\\*.log --submit internet-test -c 3 -w 10 www.example.com --submit pingscan -u 192.0.2 --submit portscan -n www.example.com 443,3389 --submit cputest --drsm=cpu.drsm type cpu.drsm \"%ProgramFiles%\\DataReel\\ReportingAgent\\drra_svc.exe\" submit --hook dru_cputest --now cpu.drsm cputest --submit --drra=C:\\test\\agent\\drra_svc.exe A weekly scheduled task on Windows: schtasks /Create /TN \"dr_utils cputest\" /SC WEEKLY /D SUN /ST 02:00 /RU SYSTEM /TR \"\\\"C:\\Program Files\\dr_utils\\cputest.exe\\\" --num-threads=4 --submit\" A connectivity check every 15 minutes on Windows: schtasks /Create /TN \"dr_utils internet-test\" /SC MINUTE /MO 15 /RU SYSTEM /TR \"\\\"C:\\Program Files\\dr_utils\\internet-test.exe\\\" -c 3 -w 10 www.example.com --submit\" A weekly cron job on Linux ( /etc/cron.d/dru_cputest ): 0 2 * * 0 root /usr/local/bin/cputest --num-threads=4 --submit > /dev/null A stale feed check every 30 minutes and a nightly port check ( /etc/cron.d/dru_checks ): */30 * * * * root /usr/local/bin/file_hours /var/spool/feed/latest.dat 2 --submit > /dev/null 15 1 * * * root /usr/local/bin/portscan -n www.example.com 22,80,443 --submit > /dev/null For a scheduled benchmark, set uploads_dru_cputest_MAX_AGE_HOURS on the host in DRSM (see below) so a stopped job shows."
},
{
"h": "Daily sar CPU use (Linux)",
"p": "Dashboard hooks",
"u": "dashboard-hooks.html#daily-sar-cpu-use-linux",
"t": "hooks/linux/dru_sar_cpu_stats.sh makes the sar text report of one day with LC_ALL=C sar -u -f /var/log/sa/saDD and runs sar_cpu_stats --submit on it. dru_sar_cpu_stats.sh [--day=yesterday|today|YYYY-MM-DD] [--sa-dir=/var/log/sa] [--now] [--print] [other sar_cpu_stats options] Option Meaning --day= yesterday (default), today or a date --sa-dir= the sysstat folder (default /var/log/sa ) --now the agent sends within seconds --print print the DRSM lines instead of submitting them other options go to sar_cpu_stats , for example --iowait=30 sar_cpu_stats is found next to the script, then in the PATH, or set SAR_CPU_STATS=/path/to/sar_cpu_stats . It needs sysstat. hooks/linux/dru_sar_cpu_stats.sh --print sudo hooks/linux/dru_sar_cpu_stats.sh --day=2026-11-18 --now --steal=5 sudo hooks/linux/install_hooks.sh --sar-cron # dry run sudo hooks/linux/install_hooks.sh --sar-cron --apply --sar-cron --apply installs bin/sar_cpu_stats and the script in /usr/local/libexec/dr_utils/ , the dru_sar_cpu_stats hook file, and /etc/cron.d/dru_sar_cpu_stats , which runs at 00:20 every day: 20 0 * * * root /usr/local/libexec/dr_utils/dru_sar_cpu_stats.sh > /dev/null The manifest warns when the data is more than 30 hours old, so a stopped cron job shows."
},
{
"h": "Alert rules",
"p": "Dashboard hooks",
"u": "dashboard-hooks.html#alert-rules",
"t": "The manifests have rules only where a limit means the same thing everywhere: Hook Rule dru_udptest , dru_mctest warning over 1% loss, error over 10% dru_sar_cpu_stats warning on steal or iowait spikes, error when a file cannot be read dru_file_hours warning when the file is stale ( stale == 1 ) dru_file_purge , dru_filetimes warning when a file cannot be read, deleted or moved ( errors > 0 ) dru_internet_test error when the test failed ( passed == 0 ) dru_dnsscan warning on a forward mismatch ( fwd_mismatch > 0 ) dru_netpscan warning when a name in the list does not resolve dru_getmac warning when the host has no ARP entry ( found == 0 ) A tool that cannot run (no ICMP socket, a missing file for file_hours ) sends an ERROR line with its message. Benchmark speeds, host counts and open ports depend on the machine and network, so set limits per host in its DRSM host file: uploads_dru_cputest_WARN_IF=\"total < 50\" uploads_dru_disktest_WARN_IF=\"total > 60\" uploads_dru_sar_cpu_stats_WARN_IF=\"steal_spikes > 0 or iowait_spikes > 0 or gaps > 2\" uploads_dru_portscan_CRIT_IF=\"port_443 != open\" uploads_dru_portscan_WARN_IF=\"port_23 == open\" uploads_dru_pingscan_WARN_IF=\"up < 12\" uploads_dru_filetimes_WARN_IF=\"errors > 0 or newest_age_hours > 2\" uploads_dru_internet_test_MAX_AGE_HOURS=2 uploads_dru_cputest_MAX_AGE_HOURS=26 Every value in the table above can be in a rule, and the METRICS list of each manifest is kept for the trends page as <hook id>_<value> (for example dru_cputest_total )."
},
{
"h": "Notes",
"p": "Dashboard hooks",
"u": "dashboard-hooks.html#notes",
"t": "The dashboard files a result under the host whose agent sent it. A sar report or results from another host show under the sending host (the sar host name is in the sar_host value and the table). A tool run with the wrong arguments (its usage message) sends nothing. file_hours checks one file per hook ID. For several files, give each its own ID ( --submit=dru_feed_a ) with a copy of the manifest folder of that name ( NAME=dru_feed_a ) and of the .ini ( id=dru_feed_a ). tcptest , udptest and mctest send from the client or receiver side; the servers have no hook output."
},
{
"h": "Testing the whole path",
"p": "Dashboard hooks",
"u": "dashboard-hooks.html#testing-the-whole-path",
"t": "tests/hooks_e2e_test.sh runs the programs, a real agent, its hook.php on a local php -S server and the DRSM upload probe, all on one Linux machine: LRA_DIR=/path/to/built/linux_reporting_agent DRSM_DIR=/path/to/drsm tests/hooks_e2e_test.sh It needs dr_utils built ( ./build.sh ), the agent built in LRA_DIR ( make ), and PHP."
},
{
"h": "Hook files reference",
"p": "Hook files reference",
"u": "hooks.html",
"t": ""
},
{
"h": "Dashboard hooks",
"p": "Hook files reference",
"u": "hooks.html#dashboard-hooks",
"t": "The dr_utils benchmarks, sar_cpu_stats and dr_report.py can send their results to the DataReel System Monitor (DRSM) dashboard through the DataReel Reporting Agent for Linux (1.0-beta2 and later) or Windows (3.3-beta2 and later). They use the agents' hook uploads : the program hands a file to the agent ( drra submit or drra_svc.exe submit ), the agent sends it, signed, to hook.php on the report server, and a DRSM upload plugin (the manifest in this folder) shows it on the host's page, with alert rules and trends. Nothing in the agents or in DRSM changes: this folder only has the hook files for them."
},
{
"h": "What each program sends",
"p": "Hook files reference",
"u": "hooks.html#what-each-program-sends",
"t": "Program Hook ID Values for rules and trends Table cputest dru_cputest total (MB/s), elapsed , threads , buf_size seconds per thread disktest dru_disktest total (seconds), insert_time , search_time , delete_time , reinsert_time , num_inserts seconds and ms per key per phase memtest dru_memtest total (seconds), threads , num_inserts seconds per phase flops dru_flops total (GFLOPS), elapsed , threads , precision , mode seconds per thread tcptest dru_tcptest total (MB/s), clients , bytes , direction MB/s per client udptest dru_udptest total (MB/s), loss_max_pct , clients , rate_limit MB/s and % lost per client mctest dru_mctest total (MB/s per receiver), loss_max_pct , receivers MB/s and % lost per receiver sar_cpu_stats dru_sar_cpu_stats busy_avg , <field>_avg and <field>_peak (user, system, iowait, steal ...), idle_min , steal_spikes , iowait_spikes , gaps , bad_samples , files_failed one row per sar file file_hours dru_file_hours stale (1 when the file is the limit old or older), age_hours , limit_hours , result (YES, NOT, ERR), file the file file_purge dru_file_purge purged , would_purge ( -n ), kept , listed ( -l ), errors , files , mode , dry_run a row per file: result and details filetimes dru_filetimes files , errors , newest_file , newest_age_hours , oldest_file , oldest_age_hours modification time and age per file internet-test dru_internet_test passed (1 or 0), tries , seconds , result , server , port pingscan dru_pingscan up , down , tried , range address, name, status, reply ms ( -u : hosts up only) dnsscan dru_dnsscan named , tried , fwd_ok and fwd_mismatch ( -f ), up ( -p ) address, name, forward check, ping portscan dru_portscan open , ports , ping , ping_ms , host , address , and port_<n> ( open or closed , 64 ports or fewer) open ports (every port when there are 16 or fewer) netpscan dru_netpscan open_ports , scanned , up , skipped ( -s ), not_resolved , ports_per_host a row per address: ping, open count, port list arpscan dru_arpscan found , tried , range address, name, MAC address, vendor getmac dru_getmac found (1 or 0), mac , ping , host , address the host dr_report.py dru_report (the whole HTML report, charts included) Every result also has an INFO line with the main result and settings, and the values benchmark (or program ), version , build and collected . The data format is drsm-lines (see \"Hook uploads\" in DRSM's PLUGINS.md), except dru_report , which is html ."
},
{
"h": "Options",
"p": "Hook files reference",
"u": "hooks.html#options",
"t": "Every benchmark (but the three servers), every file and network tool, and sar_cpu_stats take: Option Meaning --drsm=fname Write the DRSM lines to a file (to look at them, or to send later with drra submit ) --submit[=ID] Hand the results to the agent, upload hook ID (default dru_<program> ) --now With --submit : the agent sends within seconds, not with its next report --drra=path Agent program. Linux: drra in the PATH, then /usr/sbin/drra . Windows: %ProgramFiles%\\DataReel\\ReportingAgent\\drra_svc.exe (an .exe; the program is run directly, not through cmd.exe) --drra-config=ini Linux only: agent settings file ( drra -c ini ), for tests. A Windows test copy of the agent reads the drra.ini next to it dr_report.py takes --submit [ID] , --now , --drra and --drra-config the same way (default ID dru_report ). The exit status is 1 when the agent refuses the file or cannot be run, so a cron job sees it. drra submit needs root or the drra-upload group on Linux, and an administrator on Windows. With both --drsm and --submit the --drsm file is the one submitted; with --submit alone the lines go through a private temporary folder that is removed afterwards. cputest --num-threads=8 --submit --now disktest --file=/data/testfile.btx --submit udptest -s100M -r500M --submit 192.0.2.10 sar_cpu_stats --submit --now sar_today.txt dr_report.py -o host_report.html --submit results/ file_hours /var/spool/feed/latest.dat 2 --submit internet-test -c 3 -w 10 www.example.com --submit portscan -n www.example.com 22,80,443 --submit"
},
{
"h": "Setting it up",
"p": "Hook files reference",
"u": "hooks.html#setting-it-up",
"t": "Monitor host (DRSM). Copy the manifests you want to DRSM's upload plugins, for example cp -r hooks/drsm/uploads/dru_* <drsm>/plugins/uploads/ . The report server's hook.php reads them from [hooks] manifest_dir in its server.ini (the same folder on a DRSM install), and the host needs UPLOADS_DIR and UPLOADS=auto (or a list of hook IDs) in its DRSM host file. This is the same setup as every other upload hook; see \"Hook uploads\" in DRSM's PLUGINS.md. For dru_report , the report server's [hooks] max_bytes must allow the report size (reports with many charts can be several MB), and the dashboard needs $UPLOADSdir for the html display. Agent host, Linux. sudo hooks/linux/install_hooks.sh all shows what it would do (a dry run); add --apply to copy the hook files to /etc/drra/uploads.d/ . Name some hooks instead of all ( cputest disktest ) to install only those. Agent host, Windows. From an administrator prompt: powershell -ExecutionPolicy Bypass -File hooks\\windows\\install_hooks.ps1 all (dry run), then again with -Apply . The hook files go to C:\\ProgramData\\DataReel\\ReportingAgent\\uploads.d\\ . Run a program with --submit --now . drra uploads (Linux) or drra_svc.exe uploads (Windows) shows the hook's state, and the result is on the host's page after the next DRSM run."
},
{
"h": "Daily sar CPU use (Linux)",
"p": "Hook files reference",
"u": "hooks.html#daily-sar-cpu-use-linux",
"t": "hooks/linux/dru_sar_cpu_stats.sh makes the sar text report of one day (yesterday by default) with LC_ALL=C sar -u -f /var/log/sa/saDD and runs sar_cpu_stats --submit on it. --print shows the lines instead of submitting them, --day=YYYY-MM-DD picks another day, and other options go to sar_cpu_stats (for example --iowait=30 ). sudo hooks/linux/install_hooks.sh --sar-cron --apply installs bin/sar_cpu_stats and the script in /usr/local/libexec/dr_utils/ , the hook file, and /etc/cron.d/dru_sar_cpu_stats (00:20 every day). The manifest warns when the data is more than 30 hours old, so a stopped cron job shows."
},
{
"h": "Alert rules",
"p": "Hook files reference",
"u": "hooks.html#alert-rules",
"t": "The manifests have rules only where a limit means the same thing everywhere: Hook Rule dru_udptest , dru_mctest warning over 1% loss, error over 10% dru_sar_cpu_stats warning on steal or iowait spikes, error when a file cannot be read dru_file_hours warning when the file is stale ( stale == 1 ) dru_file_purge , dru_filetimes warning when a file cannot be read, deleted or moved ( errors > 0 ) dru_internet_test error when the test failed ( passed == 0 ) dru_dnsscan warning on a forward mismatch ( fwd_mismatch > 0 ) dru_netpscan warning when a name in the list does not resolve dru_getmac warning when the host has no ARP entry ( found == 0 ) A tool that cannot run (no ICMP socket, a missing file for file_hours ) sends an ERROR line with its message. Benchmark speeds, host counts and open ports depend on the machine and network, so set limits per host in its DRSM host file: uploads_dru_cputest_WARN_IF=\"total < 50\" uploads_dru_disktest_WARN_IF=\"total > 60\" uploads_dru_sar_cpu_stats_WARN_IF=\"steal_spikes > 0 or iowait_spikes > 0 or gaps > 2\" uploads_dru_portscan_CRIT_IF=\"port_443 != open\" uploads_dru_portscan_WARN_IF=\"port_23 == open\" uploads_dru_pingscan_WARN_IF=\"up < 12\" uploads_dru_filetimes_WARN_IF=\"errors > 0 or newest_age_hours > 2\" uploads_dru_internet_test_MAX_AGE_HOURS=2 uploads_dru_cputest_MAX_AGE_HOURS=26 The benchmarks have no age check by default (they run by hand); set MAX_AGE_HOURS on a host that runs one on a schedule. Every value in the table above can be in a rule, and the METRICS list of each manifest is kept for the trends page as <hook id>_<value> (for example dru_cputest_total )."
},
{
"h": "Notes",
"p": "Hook files reference",
"u": "hooks.html#notes",
"t": "The dashboard files a result under the host whose agent sent it. A sar report or --datfile results from another host show under the sending host (the sar host name is in the sar_host value and the table). A tool run with the wrong arguments (its usage message) sends nothing. file_hours checks one file per hook ID. For several files, give each its own ID ( --submit=dru_feed_a ) with a copy of the manifest folder of that name ( NAME=dru_feed_a ) and of the .ini ( id=dru_feed_a ). tcptest , udptest and mctest send from the client or receiver side; the servers ( tcpserv , udpserv , mcserv ) have no hook output. tests/hooks_e2e_test.sh runs the whole path on one Linux machine: the programs, a real agent ( LRA_DIR , a built agent source tree), its hook.php on a local php -S server, and the DRSM upload probe ( DRSM_DIR ) on the stored data."
},
{
"h": "Files",
"p": "Hook files reference",
"u": "hooks.html#files",
"t": "Path What uploads.d/<id>.ini Agent upload hook files (the same on Linux and Windows) drsm/uploads/<id>/ DRSM manifests: plugin.conf and probe.sh (which sources DRSM's upload_probe.sh ) linux/install_hooks.sh Installs hook files (and the sar cron job) on a Linux agent host, dry run by default linux/dru_sar_cpu_stats.sh , .cron Daily sar CPU use to the dashboard windows/install_hooks.ps1 Installs hook files on a Windows agent host, dry run by default"
},
{
"h": "Quick reference",
"p": "Quick reference",
"u": "quick-reference.html",
"t": ""
},
{
"h": "Quick reference",
"p": "Quick reference",
"u": "quick-reference.html#quick-reference",
"t": "Every program and option on one page. On Windows the programs are <name>.exe with the same options. Dashboard options ( --drsm , --submit[=ID] , --now , --drra , --drra-config ) are marked [D] , see Dashboard ."
},
{
"h": "File tools (details)",
"p": "Quick reference",
"u": "quick-reference.html#file-tools-details",
"t": "Program and options Exit status file_hours [-s] filename numhours 0 YES or NOT, 1 ERR; with -s : 0 YES, 1 NOT, 2 ERR file_purge [-e | -l | -h<n> | -d<n>] [-m\"dir\"] [-n] [-v] file ... 0 files checked, 1 usage error filetimes [-n | -o] file ... 0, 1 if a file cannot be read"
},
{
"h": "Network tools (details)",
"p": "Quick reference",
"u": "quick-reference.html#network-tools-details",
"t": "Program and options Exit status internet-test [-c n] [-w sec] [-t ms] [-p port] [-q] server (defaults: -c 0 until it passes, -w 60 , -t 5000 , -p 80 ) 0 passed, 1 failed, 2 error pingscan [-t ms] [-u] range ( -t 1000 ) 0 any answered, 1 none, 2 error dnsscan [-a] [-f] [-p] [-t ms] range 0 any name, 1 none, 2 error portscan [-t ms] [-n] hostname ports [services_file] ( -t 2000 , up to 1024 ports) 0 any open, 1 none, 2 error netpscan [-p ports] [-A] [-t ms] [-s] iplist_file [services_file] ( -t 500 ) 0 ran, 2 error arpscan [-i dev] [-v vendor_file] [-o out_file] [-t ms] [-a] range 0 any MAC, 1 none, 2 error getmac hostname [ethernet_device] 0 MAC found, 1 no ARP entry, 2 error Ranges: 192.0.2 (.1 to .254), 192.0.2.7 , 192.0.2.10-40 ."
},
{
"h": "Benchmarks (details)",
"p": "Quick reference",
"u": "quick-reference.html#benchmarks-details",
"t": "Program and options Defaults cputest [--num-threads=N] [--buf-size=SIZE] [--repeat=N] [--datfile=f] [D] 1 thread, 100m , 1 pass flops [--num-threads=N] [--dp] [--div] [--iterations=N] [--datfile=f] [D] 1 thread, single precision, 1g memtest [--num-threads=N] [--num-inserts=N] [--datfile=f] [D] 1 thread, 25k keys (max 1m ) disktest [--num-inserts=N] [--file=f] [--keep] [--datfile=f] [D] 1m keys, testfile.btx tcpserv [-p port] [-l address] [--count=N] [--timeout=S] [--max-bytes=SIZE] port 53897, all addresses, until Ctrl-C, 30 s, 100g tcptest [-b size] [-d] [-n clients] [-p port] [-s bytes] [--datfile=f] [D] server 1500, upload, 1 client, 53897, 150M udpserv [-p port] [-l address] [--count=N] port 53897 udptest [-b size] [-n clients] [-p port] [-r rate] [-s bytes] [--datfile=f] [D] server 1500, 1 client, 53897, no rate limit, 150M mcserv [-b size] [-g group] [-i address] [-p port] [-r rate] [-s bytes] [--seconds=N] [--ttl=N] [--no-loop] 1500, 239.255.10.10, 53898, 100M bits/s, 150M , TTL 1 mctest [-g group] [-i address] [-n receivers] [-p port] [--timeout=S] [--datfile=f] [D] [group] 239.255.10.10, 53898, 60 s Exit status: tcptest 0 if every client finished; udptest 0 if every client got a report; mctest 0 if every receiver saw the end of a run; 1 otherwise. Sizes: 1k = 1000, 1m = 10^6, 1g = 10^9. Rates ( -r ) are bits a second."
},
{
"h": "sar reports (details)",
"p": "Quick reference",
"u": "quick-reference.html#sar-reports-details",
"t": "Program and options Exit status sar_cpu_stats [--csv [--all-cpus]] [--steal=PCT] [--iowait=PCT] [--list=N] [--alarm] [D] sar_file ... 0; 1 error or no samples; 3 with --alarm and findings"
},
{
"h": "Scripts (plots and reports, dashboard)",
"p": "Quick reference",
"u": "quick-reference.html#scripts-plots-and-reports-dashboard",
"t": "Script Options benchmarks/plot/dr_plot.py [-o OUT] [--averages] [--cpu CPU] [--dpi DPI] files ... benchmarks/plot/dr_report.py -o OUT [--title T] [--csv CSV] [--no-runs] [--submit [ID]] [--now] [--drra P] [--drra-config INI] inputs ... benchmarks/scripts/watch_system.sh [-i sec] [-n count] [STATSONLY] (Linux) benchmarks/scripts/watch_mem.sh [-i sec] [-n count] [STATSONLY] (Linux) benchmarks/scripts/watch_mem_usage.sh [-i sec] [-n count] [STATSONLY] (Linux) hooks/linux/install_hooks.sh [--sar-cron] [--apply] all | <id> ... (dry run without --apply ) hooks/linux/dru_sar_cpu_stats.sh [--day=yesterday|today|YYYY-MM-DD] [--sa-dir=DIR] [--now] [--print] [sar_cpu_stats options] hooks/windows/install_hooks.ps1 all | <id> ... [-Apply] (dry run without -Apply ) build.sh / build.cmd [clean] , with DATAREEL_DIR set (and ZLIB_DIR on Windows) tests/run_tests.sh / tests\\run_tests.ps1 run_tests.ps1 [-NoLan]"
},
{
"h": "Ports",
"p": "Quick reference",
"u": "quick-reference.html#ports",
"t": "Port Used by TCP 53897 tcpserv / tcptest UDP 53897 udpserv / udptest UDP 53898, group 239.255.10.10 mcserv / mctest"
}
];
