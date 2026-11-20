#!/bin/bash
# dr_utils Linux tests. Build first (./build.sh), then run:
#   tests/run_tests.sh            every test
#   tests/run_tests.sh --no-lan   skip the getmac/arpscan check of a host
#                                 already in this machine's ARP table
#
# Network tests use only 127.0.0.1 and a local HTTP server on a free
# port (python3). Pings need ICMP datagram sockets
# (net.ipv4.ping_group_range) or root.
set -u
TOP=$(cd "$(dirname "$0")/.." && pwd)
BIN=$TOP/bin
WORK=$TOP/tests/work
LAN=1
[ "${1:-}" = "--no-lan" ] && LAN=0

pass=0; fail=0
ok()   { pass=$((pass+1)); echo "ok   $1"; }
bad()  { fail=$((fail+1)); echo "FAIL $1"; [ -n "${2:-}" ] && echo "     $2"; }
# check NAME EXPECTED_RC CMD... : run CMD, compare its exit status
check_rc() {
  local name=$1 want=$2; shift 2
  OUT=$("$@" 2>&1); local rc=$?
  if [ "$rc" = "$want" ]; then ok "$name"; else bad "$name" "exit $rc, wanted $want: $(echo "$OUT" | head -3)"; fi
}
# has NAME PATTERN : the last command's output matches PATTERN
has() {
  if echo "$OUT" | grep -q -E -- "$2"; then ok "$1"; else bad "$1" "no /$2/ in: $(echo "$OUT" | head -5)"; fi
}

for t in file_hours file_purge filetimes internet-test pingscan dnsscan portscan netpscan arpscan getmac \
         cputest disktest memtest tcpserv tcptest udpserv udptest mcserv mctest flops sar_cpu_stats sar_stats; do
  if [ ! -x "$BIN/$t" ]; then echo "$BIN/$t is missing: run ./build.sh first" >&2; exit 2; fi
done

rm -rf "$WORK"; mkdir -p "$WORK/purge" "$WORK/moved"
cd "$WORK" || exit 2

echo "--- file_hours"
touch -d '3 hours ago' old3h.txt
touch -d '40 minutes ago' new40m.txt
check_rc "file_hours 3h file is 2 hours old" 0 "$BIN/file_hours" old3h.txt 2; has "YES" '^YES$'
check_rc "file_hours 3h file is not 5 hours old" 0 "$BIN/file_hours" old3h.txt 5; has "NOT" '^NOT$'
check_rc "file_hours 40m file is 0.5 hours old" 0 "$BIN/file_hours" new40m.txt 0.5; has "YES under one hour" '^YES$'
check_rc "file_hours -s NOT exits 1" 1 "$BIN/file_hours" -s new40m.txt 1
check_rc "file_hours -s YES exits 0" 0 "$BIN/file_hours" -s old3h.txt 1
check_rc "file_hours missing file" 1 "$BIN/file_hours" nofile.txt 1; has "ERR missing" '^ERR - nofile.txt does not exist'
check_rc "file_hours -s missing exits 2" 2 "$BIN/file_hours" -s nofile.txt 1
check_rc "file_hours bad number" 1 "$BIN/file_hours" old3h.txt abc; has "ERR number" 'not a number'
check_rc "file_hours directory" 1 "$BIN/file_hours" purge 1; has "ERR dir" 'is not a file name'
touch -d 'tomorrow' future.txt
check_rc "file_hours future" 1 "$BIN/file_hours" future.txt 1; has "ERR future" 'in the future'

echo "--- filetimes"
check_rc "filetimes one file" 0 "$BIN/filetimes" old3h.txt
has "mod time" 'File modification time:'; has "GMT" 'File modification GMT : .* GMT'; has "age hours" 'File is 3.0[0-9] hour'
check_rc "filetimes -n newest" 0 "$BIN/filetimes" -n old3h.txt new40m.txt; has "newest" '^new40m.txt$'
check_rc "filetimes -o oldest" 0 "$BIN/filetimes" -o old3h.txt new40m.txt; has "oldest" '^old3h.txt$'
check_rc "filetimes missing" 1 "$BIN/filetimes" nofile.txt

echo "--- file_purge"
P=purge
touch -d '10 days ago' $P/a.log; touch -d '2 hours ago' $P/b.log; touch $P/c.log; mkdir -p $P/sub.log
check_rc "file_purge -l lists" 0 "$BIN/file_purge" -l $P/*.log; has "LST" '^LST - purge/a.log \(240.0[0-9]\) hours \(10.00\) days'
check_rc "file_purge -n dry run" 0 "$BIN/file_purge" -n -h1 $P/*.log; has "DRY" '^DRY - purge/b.log would be purged'
if [ -f $P/a.log ] && [ -f $P/b.log ]; then ok "file_purge -n kept the files"; else bad "file_purge -n deleted files"; fi
check_rc "file_purge -h1.5" 0 "$BIN/file_purge" -h1.5 $P/*.log
has "b purged" '^YES - purge/b.log purged'; has "c kept" '^NOT - purge/c.log not purged'
if [ ! -e $P/a.log ] && [ ! -e $P/b.log ] && [ -f $P/c.log ] && [ -d $P/sub.log ]; then ok "file_purge -h deleted only old files"; else bad "file_purge -h files" "$(ls $P)"; fi
touch -d '20 hours ago' $P/d.log
check_rc "file_purge -d0.5 (20 h old)" 0 "$BIN/file_purge" -d0.5 $P/d.log; has "d purged" '^YES - purge/d.log purged \(0.8[0-9]\) days'
touch -d '3 days ago' $P/e.log
check_rc "file_purge -m move" 0 "$BIN/file_purge" -m"$WORK/moved" -d2 $P/e.log; has "moved" '^YES - purge/e.log moved'
if [ -f moved/e.log ] && [ ! -e $P/e.log ]; then ok "file_purge -m moved the file"; else bad "file_purge -m file"; fi
touch -d '3 days ago' $P/e.log
check_rc "file_purge -m target exists" 0 "$BIN/file_purge" -m"$WORK/moved" -d2 $P/e.log; has "not overwritten" 'not moved, .* exists'
check_rc "file_purge -e" 0 "$BIN/file_purge" -e $P/c.log; has "existing" '^YES - purge/c.log existing file purged'
check_rc "file_purge -h and -d" 1 "$BIN/file_purge" -h1 -d1 old3h.txt
check_rc "file_purge no operation" 1 "$BIN/file_purge" old3h.txt
check_rc "file_purge bad -h" 1 "$BIN/file_purge" -hx old3h.txt
check_rc "file_purge missing move dir" 1 "$BIN/file_purge" -m"$WORK/nodir" -h1 old3h.txt
check_rc "file_purge no files" 1 "$BIN/file_purge" -h1
if [ -f old3h.txt ]; then ok "file_purge error cases changed nothing"; else bad "old3h.txt deleted"; fi

echo "--- range and argument errors"
check_rc "pingscan bad range" 2 "$BIN/pingscan" 300.1.1
check_rc "pingscan bad last byte range" 2 "$BIN/pingscan" 10.0.0.9-3
check_rc "pingscan /16 refused" 2 "$BIN/pingscan" 10.0
check_rc "dnsscan no range" 2 "$BIN/dnsscan"
check_rc "portscan bad ports" 2 "$BIN/portscan" 127.0.0.1 0
check_rc "portscan too many ports" 2 "$BIN/portscan" 127.0.0.1 1-2000
check_rc "arpscan bad range" 2 "$BIN/arpscan" a.b.c
check_rc "netpscan missing list" 2 "$BIN/netpscan" "$WORK/none.txt"

echo "--- ping"
check_rc "pingscan 127.0.0.1" 0 "$BIN/pingscan" -t 500 127.0.0.1
has "reply" 'Host is up, reply in'; has "summary" '1 of 1 hosts answered'
check_rc "pingscan -u 127.0.0.1-3" 0 "$BIN/pingscan" -u -t 300 127.0.0.1-3; has "three up" '3 of 3 hosts answered'
check_rc "dnsscan 127.0.0.1 -f -p" 0 "$BIN/dnsscan" -f -p -t 300 127.0.0.1
has "dnsscan name" '^127.0.0.1	localhost'; has "dnsscan up" 'up \('

echo "--- TCP (local HTTP server)"
if ! command -v python3 >/dev/null; then
  bad "python3 is needed for the TCP tests"
else
  PORT=$(python3 -c 'import socket; s=socket.socket(); s.bind(("127.0.0.1",0)); print(s.getsockname()[1]); s.close()')
  CLOSED=$(python3 -c 'import socket; s=socket.socket(); s.bind(("127.0.0.1",0)); print(s.getsockname()[1]); s.close()')
  mkdir -p www && echo hello > www/index.html
  # (cd www): Python 3.6 (RHEL 7 and 8) has no --directory
  (cd www && exec python3 -m http.server "$PORT" --bind 127.0.0.1) > http.log 2>&1 &
  HTTP_PID=$!
  for i in $(seq 1 50); do
    python3 -c "import socket; socket.create_connection(('127.0.0.1',$PORT),1)" 2>/dev/null && break
    sleep 0.1
  done
  printf 'drtest\t%s/tcp\tdrtestalias\t# dr_utils test port\nclosed\t%s/tcp\n' "$PORT" "$CLOSED" > services.txt
  check_rc "portscan open port" 0 "$BIN/portscan" -t 1000 127.0.0.1 "$PORT" services.txt
  has "open" "TCP port $PORT open"; has "service name" 'Name: +drtest'; has "service comment" 'Comment: +dr_utils test port'
  check_rc "portscan closed port" 1 "$BIN/portscan" -t 1000 127.0.0.1 "$CLOSED" services.txt; has "closed" "TCP port $CLOSED closed"
  check_rc "portscan list" 0 "$BIN/portscan" -n 127.0.0.1 "$CLOSED,$PORT" services.txt; has "1 of 2" '1 of 2 TCP ports open'
  printf '# test list\n127.0.0.1   # loopback\n\n' > iplist.txt
  check_rc "netpscan services file" 0 "$BIN/netpscan" -A iplist.txt services.txt
  has "netpscan open" "^$PORT/tcp drtest drtestalias # dr_utils test port"; has "netpscan count" '^1 open TCP port'
  check_rc "netpscan -p" 0 "$BIN/netpscan" -p "$CLOSED,$PORT" iplist.txt services.txt; has "netpscan -p open" "TCP port $PORT open"
  check_rc "internet-test pass" 0 "$BIN/internet-test" -c 1 "127.0.0.1:$PORT"; has "passed" 'test passed .*HTTP status 200'
  check_rc "internet-test URL" 0 "$BIN/internet-test" -c 1 -p "$PORT" "http://127.0.0.1/index.html"
  check_rc "internet-test fail" 1 "$BIN/internet-test" -c 2 -w 0 -t 500 "127.0.0.1:$CLOSED"; has "failed after 2" 'failed after 2 tries'
  check_rc "internet-test bad name" 1 "$BIN/internet-test" -c 1 no-such-host.invalid; has "DNS step" 'DNS:'
  echo "--- TCP tools: dashboard lines (--drsm)"
  check_rc "portscan --drsm" 0 "$BIN/portscan" -n 127.0.0.1 "$CLOSED,$PORT" services.txt --drsm="$WORK/ps.lines"
  OUT=$(cat "$WORK/ps.lines"); has "portscan summary" "^INFO - portscan 127.0.0.1 \(127.0.0.1\): 1 of 2 TCP ports open, ping not run\$"
  has "portscan port values" "^DETAIL port_$PORT=open\$"; has "portscan closed value" "^DETAIL port_$CLOSED=closed\$"
  has "portscan open row" "^UPLOAD row\|$PORT\|open\|drtest\$"
  check_rc "netpscan --drsm" 0 "$BIN/netpscan" -p "$CLOSED,$PORT" iplist.txt services.txt --drsm="$WORK/np.lines"
  OUT=$(cat "$WORK/np.lines"); has "netpscan values" '^DETAIL open_ports=1$'; has "netpscan row" "^UPLOAD row\|127.0.0.1\|127.0.0.1\|.*\|1\|$PORT\$"
  check_rc "internet-test --drsm pass" 0 "$BIN/internet-test" -c 1 "127.0.0.1:$PORT" --drsm="$WORK/it.lines"
  OUT=$(cat "$WORK/it.lines"); has "internet-test passed value" '^DETAIL passed=1$'; has "internet-test result" '^DETAIL result=HTTP status 200$'
  check_rc "internet-test --drsm fail" 1 "$BIN/internet-test" -c 1 -t 500 "127.0.0.1:$CLOSED" --drsm="$WORK/it2.lines"
  OUT=$(cat "$WORK/it2.lines"); has "internet-test failed value" '^DETAIL passed=0$'
  if grep -q '^ERROR' "$WORK/it2.lines"; then bad "a failed test is a value, not an ERROR line"; else ok "a failed test is a value, not an ERROR line"; fi
  kill "$HTTP_PID" 2>/dev/null; wait "$HTTP_PID" 2>/dev/null
fi

echo "--- MAC addresses"
check_rc "getmac loopback has no ARP entry" 1 "$BIN/getmac" 127.0.0.1
check_rc "arpscan loopback none found" 1 "$BIN/arpscan" -t 200 127.0.0.1
if [ "$LAN" = 1 ]; then
  # A host already in this machine's ARP table (reachable, Flags 0x2)
  read -r A_IP A_MAC A_DEV < <(awk 'NR>1 && $3=="0x2" && $4!="00:00:00:00:00:00" {print $1, toupper($4), $6; exit}' /proc/net/arp)
  if [ -n "${A_IP:-}" ]; then
    check_rc "getmac $A_IP" 0 "$BIN/getmac" "$A_IP"; has "getmac MAC" "MAC Address: $A_MAC"
    check_rc "getmac $A_IP $A_DEV" 0 "$BIN/getmac" "$A_IP" "$A_DEV"
    check_rc "getmac wrong interface" 1 "$BIN/getmac" "$A_IP" no-such-dev0
    check_rc "arpscan $A_IP -o" 0 "$BIN/arpscan" -o arp.txt "$A_IP"; has "arpscan MAC" "^$A_IP	.*	$A_MAC"
    if grep -q "$A_MAC" arp.txt; then ok "arpscan -o file"; else bad "arpscan -o file"; fi
    printf '%s Test Vendor\n' "$(echo "$A_MAC" | cut -c1-8 | tr -d :)" > vendors.txt
    check_rc "arpscan -v vendor" 0 "$BIN/arpscan" -v vendors.txt "$A_IP"
    has "arpscan vendor name" "	$A_MAC	Test Vendor$"
  else
    echo "skip getmac/arpscan LAN check: no complete entry in /proc/net/arp"
  fi
fi

echo "--- benchmarks (small sizes, loopback only)"
check_rc "cputest 2 threads" 0 "$BIN/cputest" --num-threads=2 --buf-size=4m
has "cputest threads" 'Thread\[1\]: CPU speed test elapsed time'; has "cputest total" '^Total elapsed time .* (KB|MB|GB)/s'
check_rc "cputest bad size" 1 "$BIN/cputest" --buf-size=abc
check_rc "cputest unknown switch" 1 "$BIN/cputest" --bogus
check_rc "disktest 20k" 0 "$BIN/disktest" --num-inserts=20k --file="$WORK/disk.btx" --datfile="$WORK/disk.dat"
has "disktest phases" 'Reinserted 20000 keys:'; has "disktest entries" 'Number of entries = 20000'
if [ ! -e "$WORK/disk.btx" ]; then ok "disktest removed its file"; else bad "disktest left its file"; fi
if grep -q '^write_time=.*,.*' "$WORK/disk.dat" && grep -q '^xticks_str=2K,4K' "$WORK/disk.dat" && grep -q '^\[HOSTINFO\]' "$WORK/disk.dat"; then ok "disktest plot data"; else bad "disktest plot data" "$(head -8 "$WORK/disk.dat")"; fi
touch "$WORK/exists.btx"
check_rc "disktest refuses an existing file" 1 "$BIN/disktest" --num-inserts=1k --file="$WORK/exists.btx"
check_rc "memtest 2 threads" 0 "$BIN/memtest" --num-threads=2 --num-inserts=5k
has "memtest phases" 'Thread\[1\]: Reinserted 5000 values'; has "memtest average" '^Average thread times'
if command -v python3 >/dev/null; then
  TP=$(python3 -c 'import socket; s=socket.socket(); s.bind(("127.0.0.1",0)); print(s.getsockname()[1]); s.close()')
  "$BIN/tcpserv" -p$TP -l127.0.0.1 --count=3 > tcpserv.log 2>&1 &
  SP=$!
  for i in $(seq 1 50); do python3 -c "import socket; socket.create_connection(('127.0.0.1',$TP),1)" 2>/dev/null && break; sleep 0.1; done
  # The probe connection above is one bad request; --count counts it
  check_rc "tcptest upload 2 clients" 0 "$BIN/tcptest" -p$TP -n2 -b65536 -s20M --datfile="$WORK/tcp.dat" 127.0.0.1
  has "tcptest total" 'All tests complete: 2 of 2 clients'
  wait $SP; SRC=$?
  if [ $SRC = 0 ] && grep -q 'upload 20000000 bytes' tcpserv.log; then ok "tcpserv logged the uploads"; else bad "tcpserv" "$(cat tcpserv.log)"; fi
  "$BIN/tcpserv" -p$TP -l127.0.0.1 --count=1 > tcpserv2.log 2>&1 &
  SP=$!; sleep 0.5
  check_rc "tcptest download" 0 "$BIN/tcptest" -d -p$TP -b65536 -s10M 127.0.0.1; has "tcptest received" 'received 10000000 bytes'
  wait $SP
  check_rc "tcptest no server" 1 "$BIN/tcptest" -p$TP -s1M 127.0.0.1
  check_rc "tcptest bad -b" 1 "$BIN/tcptest" -b10 127.0.0.1
  UP=$(python3 -c 'import socket; s=socket.socket(socket.AF_INET,socket.SOCK_DGRAM); s.bind(("127.0.0.1",0)); print(s.getsockname()[1]); s.close()')
  "$BIN/udpserv" -p$UP -l127.0.0.1 --count=2 > udpserv.log 2>&1 &
  SP=$!; sleep 0.5
  check_rc "udptest 2 clients at 200M" 0 "$BIN/udptest" -p$UP -n2 -s5M -r200M --datfile="$WORK/udp.dat" 127.0.0.1
  has "udptest report" 'server received [0-9]+ \([0-9.]+% lost'; has "udptest total" 'All tests complete: 2 of 2 clients reported'
  wait $SP
  if grep -q 'run .*datagrams' udpserv.log; then ok "udpserv logged the runs"; else bad "udpserv" "$(cat udpserv.log)"; fi
  check_rc "udptest no server" 1 "$BIN/udptest" -p$UP -s100k 127.0.0.1; has "no report" 'no report from the server'
  "$BIN/mctest" -n2 -i127.0.0.1 -p$UP --timeout=10 --datfile="$WORK/mc.dat" > mctest.log 2>&1 &
  MP=$!; sleep 0.5
  check_rc "mcserv on loopback" 0 "$BIN/mcserv" -i127.0.0.1 -p$UP -s2M -r100M; has "mcserv sent" '^Sent 1333 datagrams'
  wait $MP; MRC=$?
  if [ $MRC = 0 ] && grep -q 'Receiver\[1\]: run .*1333 datagrams' mctest.log; then ok "mctest received the run"; else bad "mctest" "$(cat mctest.log)"; fi
  check_rc "mcserv refuses a unicast group" 1 "$BIN/mcserv" -g192.0.2.1
  check_rc "mcserv ttl limit" 1 "$BIN/mcserv" --ttl=64
else
  bad "python3 is needed for the network benchmark tests"
fi

echo "--- flops, plot and watch scripts"
check_rc "flops 2 threads" 0 "$BIN/flops" --num-threads=2 --iterations=100m; has "flops rate" 'Total elapsed time .* (MFLOPS|GFLOPS)'
check_rc "flops --dp" 0 "$BIN/flops" --dp --iterations=50m; has "flops dp" 'Thread\[0\]: Double precision'
check_rc "flops --div (c/flops formula)" 0 "$BIN/flops" --div --num-threads=2 --iterations=100m; has "flops div" 'a\*b\+a/b loops'; has "flops div 3" '3 FLOPs a loop'
check_rc "flops --div --dp" 0 "$BIN/flops" --div --dp --iterations=50m
check_rc "flops bad iterations" 1 "$BIN/flops" --iterations=10
if python3 -c 'import matplotlib' 2>/dev/null; then
  PL="$TOP/benchmarks/plot/dr_plot.py"
  check_rc "dr_plot disk" 0 python3 "$PL" -o "$WORK/disk.png" "$WORK/disk.dat"
  check_rc "dr_plot disk --averages" 0 python3 "$PL" --averages -o "$WORK/disk_avg.png" "$WORK/disk.dat"
  cp "$WORK/disk.dat" "$WORK/disk2.dat"
  check_rc "dr_plot disk compare" 0 python3 "$PL" -o "$WORK/disk_cmp.png" "$WORK/disk.dat" "$WORK/disk2.dat"
  "$BIN/cputest" --num-threads=2 --buf-size=2m --datfile="$WORK/cpu2.dat" >/dev/null
  "$BIN/cputest" --num-threads=1 --buf-size=2m --datfile="$WORK/cpu1.dat" >/dev/null
  "$BIN/memtest" --num-inserts=2k --datfile="$WORK/mem.dat" >/dev/null
  "$BIN/flops" --iterations=10m --datfile="$WORK/flops.dat" >/dev/null
  for f in cpu2 mem flops tcp udp mc; do
    if grep -q '^\[BENCHMARK\]' "$WORK/$f.dat" 2>/dev/null && grep -q '^item_values=' "$WORK/$f.dat"; then ok "$f --datfile"; else bad "$f --datfile" "$(head -5 "$WORK/$f.dat" 2>&1)"; fi
    check_rc "dr_plot $f" 0 python3 "$PL" -o "$WORK/$f.png" "$WORK/$f.dat"
  done
  check_rc "dr_plot cputest compare" 0 python3 "$PL" -o "$WORK/cpu_cmp.png" "$WORK/cpu1.dat" "$WORK/cpu2.dat"
  "$BIN/sar_cpu_stats" --csv --all-cpus "$TOP/tests/data/sar/sar_rhel7_ampm.txt" "$TOP/tests/data/sar/sar_steal_spike.txt" > "$WORK/sar.csv"
  check_rc "dr_plot sar csv" 0 python3 "$PL" -o "$WORK/sar.png" "$WORK/sar.csv"
  check_rc "dr_plot sar one cpu" 0 python3 "$PL" --cpu 3 -o "$WORK/sar3.png" "$WORK/sar.csv"
  check_rc "dr_plot sar missing cpu" 1 python3 "$PL" --cpu 99 -o "$WORK/sar99.png" "$WORK/sar.csv"
  "$TOP/benchmarks/scripts/watch_mem_usage.sh" -n 2 -i 1 > "$WORK/mem.log"
  check_rc "dr_plot memory log" 0 python3 "$PL" -o "$WORK/memlog.png" "$WORK/mem.log"
  check_rc "dr_plot mixed benchmarks refused" 1 python3 "$PL" -o "$WORK/x.png" "$WORK/cpu1.dat" "$WORK/mem.dat"
  check_rc "dr_plot mixed kinds refused" 1 python3 "$PL" -o "$WORK/x.png" "$WORK/cpu1.dat" "$WORK/sar.csv"
  check_rc "dr_plot unknown file" 1 python3 "$PL" -o "$WORK/x.png" "$TOP/README.md"
  n=0; for f in disk disk_avg disk_cmp cpu2 mem flops tcp udp mc cpu_cmp sar sar3 memlog; do file "$WORK/$f.png" | grep -q 'PNG image' && n=$((n+1)); done
  if [ $n = 13 ]; then ok "13 PNG files"; else bad "PNG files" "$n of 13"; fi
  RP="$TOP/benchmarks/plot/dr_report.py"
  mkdir -p "$WORK/report_in"
  cp "$WORK"/cpu1.dat "$WORK"/cpu2.dat "$WORK"/mem.dat "$WORK"/flops.dat "$WORK"/tcp.dat "$WORK"/udp.dat "$WORK"/mc.dat "$WORK"/disk.dat "$WORK"/sar.csv "$WORK"/mem.log "$WORK/report_in/"
  echo "not a result" > "$WORK/report_in/notes.csv"
  check_rc "dr_report folder" 0 python3 "$RP" -o "$WORK/report.html" --title "dr_utils test report" --csv "$WORK/summary.csv" "$WORK/report_in"
  has "report counts" '(10 result files, 1 skipped)'
  R=$(cat "$WORK/report.html")
  for want in '<title>dr_utils test report</title>' 'id="summary"' 'id="b-cputest"' 'id="b-disktest"' 'id="b-udptest"' 'id="sar"' 'id="mem"' 'Comparison of 2 runs' 'bad samples' 'Skipped files'; do
    if echo "$R" | grep -q -- "$want"; then ok "report has $want"; else bad "report has $want"; fi
  done
  imgs=$(grep -o 'data:image/png;base64' "$WORK/report.html" | wc -l)
  if [ "$imgs" = 11 ]; then ok "report has $imgs charts"; else bad "report charts" "$imgs"; fi
  if [ "$(wc -l < "$WORK/summary.csv")" = 9 ] && head -1 "$WORK/summary.csv" | grep -q '^benchmark,host,date,build,settings,result,file$'; then ok "report summary CSV"; else bad "report summary CSV" "$(cat "$WORK/summary.csv")"; fi
  check_rc "dr_report --no-runs" 0 python3 "$RP" -o "$WORK/report2.html" --no-runs "$WORK/report_in"
  check_rc "dr_report missing file" 1 python3 "$RP" -o "$WORK/report3.html" "$WORK/none.dat"
  check_rc "dr_report nothing to report" 1 python3 "$RP" -o "$WORK/report4.html" "$TOP/tests/data/sar"
  "$BIN/sar_stats" --csv "$TOP/tests/data/sar/sar_A_rhel10.txt" > "$WORK/sarall.csv"
  check_rc "dr_plot every sar section (PDF)" 0 python3 "$PL" -o "$WORK/sarall.pdf" "$WORK/sarall.csv"
  has "pdf pages" 'sarall.pdf \([0-9]+ pages\)'
  if file "$WORK/sarall.pdf" | grep -q 'PDF document'; then ok "PDF file"; else bad "PDF file"; fi
  rm -f "$WORK"/sarall_*.png
  check_rc "dr_plot sar sections as PNG" 0 python3 "$PL" --section=cpu,mem --items 2 -o "$WORK/sarall.png" "$WORK/sarall.csv"
  n=$(ls "$WORK"/sarall_cpu_all.png "$WORK"/sarall_mem.png 2>/dev/null | wc -l)
  if [ "$n" = 2 ]; then ok "sar PNG per chart"; else bad "sar PNG per chart" "$(ls "$WORK" | grep sarall)"; fi
  check_rc "dr_plot sar unknown section" 1 python3 "$PL" --section=nosuch -o "$WORK/x.pdf" "$WORK/sarall.csv"
  mkdir -p "$WORK/report_sar" && cp "$WORK/sarall.csv" "$WORK/report_sar/"
  check_rc "dr_report every sar section" 0 python3 "$RP" -o "$WORK/report5.html" "$WORK/report_sar"
  R=$(cat "$WORK/report5.html")
  for want in 'sar report: node1.example.com' 'CPU utilization \(sar -u, -P ALL\)' 'File systems \(sar -F\)' 'Averages per item, busiest first' 'Network interfaces \(sar -n DEV\)'; do
    if echo "$R" | grep -q -E -- "$want"; then ok "sar report has $want"; else bad "sar report has $want"; fi
  done
else
  bad "python3 matplotlib is needed for the plot tests (python3-matplotlib)"
fi
check_rc "watch_mem_usage.sh -n 2" 0 "$TOP/benchmarks/scripts/watch_mem_usage.sh" -n 2 -i 1; has "mem lines" '^[0-9-]+ [0-9:]+ [0-9]+$'
check_rc "watch_mem.sh -n 1" 0 "$TOP/benchmarks/scripts/watch_mem.sh" -n 1; has "meminfo" 'MemTotal:'
check_rc "watch_system.sh STATSONLY" 0 "$TOP/benchmarks/scripts/watch_system.sh" STATSONLY; has "sections" 'END DISKSTATS'

echo "--- sar_cpu_stats"
SAR="$TOP/tests/data/sar"
check_rc "sar RHEL 5" 0 "$BIN/sar_cpu_stats" "$SAR/sar_rhel5_partial.txt"; has "rhel5 samples" '52 samples of all CPUs, every 10 minutes'; has "rhel5 user" 'user +0\.'
if echo "$OUT" | grep -q 'gnice'; then bad "rhel5 has no gnice column"; else ok "rhel5 missing columns not shown"; fi
check_rc "sar RHEL 7 AM/PM" 0 "$BIN/sar_cpu_stats" "$SAR/sar_rhel7_ampm.txt"; has "rhel7 samples" '42 samples'
check_rc "sar RHEL 10" 0 "$BIN/sar_cpu_stats" "$SAR/sar_rhel10.txt"; has "rhel10 interval" 'every 1 seconds'; has "rhel10 iowait spikes" 'iowait >= 20.0%: 4'
check_rc "sar gap" 0 "$BIN/sar_cpu_stats" "$SAR/sar_gap.txt"; has "gap listed" 'gap: no samples for 70 minutes before 2018-09-30 01:40:01'
check_rc "sar steal spike" 0 "$BIN/sar_cpu_stats" "$SAR/sar_steal_spike.txt"
has "241 flagged" 'bad sample at 2018-09-24 00:00:00: steal 241.45'; has "day stops" 'gap: samples stop at 2018-09-24 00:10:01'; has "counts" 'gaps: 1, bad samples: 2'
check_rc "sar --alarm" 3 "$BIN/sar_cpu_stats" --alarm "$SAR/sar_steal_spike.txt"
check_rc "sar --alarm clean" 0 "$BIN/sar_cpu_stats" --alarm "$SAR/sar_rhel7_ampm.txt"
check_rc "sar --steal=0.01 --iowait=5" 0 "$BIN/sar_cpu_stats" --iowait=5 "$SAR/sar_rhel7_ampm.txt"
check_rc "sar csv" 0 "$BIN/sar_cpu_stats" --csv --all-cpus "$SAR/sar_rhel7_ampm.txt"
has "csv header" '^host,time,cpu,user,nice,system,iowait,steal,irq,soft,guest,gnice,idle,sample$'
has "csv row" '^rhel7-2.example.com,2018-09-30 00:10:01,all,0.33,0.00,0.24,0.25,0.00,0.00,0.00,0.00,0.00,99.19,ok$'
has "csv per cpu" ',2018-09-30 00:10:01,5,'
check_rc "sar csv rhel5 empty columns" 0 "$BIN/sar_cpu_stats" --csv "$SAR/sar_rhel5_partial.txt"; has "empty cols" ',0.00,,,,,99.28,ok$'
check_rc "sar two files" 0 "$BIN/sar_cpu_stats" "$SAR/sar_rhel5_partial.txt" "$SAR/sar_gap.txt"
check_rc "sar no CPU data" 1 "$BIN/sar_cpu_stats" "$SAR/README.md"
check_rc "sar missing file" 1 "$BIN/sar_cpu_stats" "$SAR/none.txt"

echo "--- sar_stats (every sar section)"
check_rc "sar_stats RHEL 10 list" 0 "$BIN/sar_stats" --list "$SAR/sar_A_rhel10.txt"
has "rhel10 35 sections" 'samples every 2 seconds, 35 sections'
for id in cpu pcsw swap paging io mem swpspace huge ktables queue disk net_dev net_edev nfs nfsd sock ip eip icmp eicmp tcp etcp udp sock6 ip6 eip6 icmp6 eicmp6 udp6 softnet cpufreq fan temp usb fs; do
  if echo "$OUT" | grep -q -E "^  $id "; then ok "rhel10 section $id"; else bad "rhel10 section $id"; fi
done
has "rhel10 disks" '^  disk +-d +13 +14 +Block devices'
has "rhel10 file system" '\[FILESYSTEM\] MBfsfree'
check_rc "sar_stats RHEL 10 summary" 0 "$BIN/sar_stats" "$SAR/sar_A_rhel10.txt"
has "cpu all" '^== cpu: CPU utilization \(sar -u, -P ALL\), 9 CPU, 14 samples'
has "cpu idle" '^  %idle +70\.22 +58\.07 +80\.83  2026-11-20 22:45:55'
has "per disk" '^  sda +132\.32 +199\.86 +7836\.27'
has "disk highest" '^  wkB/s +40374\.50  sda at 2026-11-20 22:45:45'
has "file system" '^== fs: File systems \(sar -F\), 1 FILESYSTEM'
check_rc "sar_stats RHEL 5 (separate proc/s, cswch/s, INTR, i000/s)" 0 "$BIN/sar_stats" --list "$SAR/sar_A_rhel5.txt"
has "rhel5 days" 'Days: 2 \(2018-10-01 to 2018-10-02\), samples every 10 minutes, 17 sections'
has "rhel5 pcsw one sample per time" '^  pcsw +-w +1 +22 '
has "rhel5 per-CPU interrupts" '^  intr_cpu +-I ALL -P ALL +2 +22 '
check_rc "sar_stats RHEL 7" 0 "$BIN/sar_stats" --list "$SAR/sar_A_rhel7.txt"
has "rhel7 sections" '18 sections'; has "rhel7 huge pages" '^  huge +-H +1 +11 '
check_rc "sar_stats AM/PM times" 0 "$BIN/sar_stats" "$SAR/sar_rhel7_ampm.txt"
has "ampm 24-hour" '^  %usr +0\.27 +0\.23 +0\.67  2018-09-30 00:30:01'
check_rc "sar_stats --section" 0 "$BIN/sar_stats" --section=queue,mem "$SAR/sar_A_rhel5.txt"
has "only queue" '^== queue: Run queue'; if echo "$OUT" | grep -q '^== cpu'; then bad "--section left cpu out"; else ok "--section left cpu out"; fi
has "constant column" '^  plist-sz +138\.00 +138\.00 +138\.00  -$'
check_rc "sar_stats --item" 0 "$BIN/sar_stats" --section=cpu --item=all "$SAR/sar_A_rhel7.txt"; has "only all" '1 CPU, 11 samples'
check_rc "sar_stats --csv" 0 "$BIN/sar_stats" --csv --section=mem "$SAR/sar_A_rhel10.txt"
has "csv header" '^host,time,section,item,metric,value$'; has "csv row" '^node1.example.com,2026-11-20 22:45:31,mem,,kbmemfree,6577840$'
printf 'Linux 6.0 (node9.example.com) \t11/20/26 \t_x86_64_\t(2 CPU)\n\n00:00:01     newm/s   other%%\n00:10:01       1.50      20.00\n00:20:01       2.50      40.00\n' > "$WORK/sar_new.txt"
check_rc "sar_stats unknown section" 0 "$BIN/sar_stats" "$WORK/sar_new.txt"; has "kept by its columns" '^== other_newm_s: newm/s other%'
has "unknown values" '^  newm/s +2\.00 +1\.50 +2\.50  2026-11-20 00:20:01'
check_rc "sar_stats no data" 1 "$BIN/sar_stats" "$SAR/README.md"
check_rc "sar_stats missing file" 1 "$BIN/sar_stats" "$SAR/none.txt"
check_rc "sar_stats bad switch" 1 "$BIN/sar_stats" --bogus "$SAR/sar_A_rhel5.txt"

echo "--- dashboard hooks (--drsm, --submit with a stand-in agent)"
# A stand-in for drra: records its arguments and a copy of the file
FAKE="$WORK/fake_drra"
cat > "$FAKE" <<'EOS'
#!/bin/bash
echo "$*" > "$(dirname "$0")/drra.args"
for last; do :; done
cp "$last" "$(dirname "$0")/drra.payload"
[ -n "${FAKE_DRRA_FAIL:-}" ] && exit 4
exit 0
EOS
chmod 755 "$FAKE"
check_rc "cputest --drsm" 0 "$BIN/cputest" --num-threads=2 --buf-size=2m --drsm="$WORK/cpu.lines"
L=$(cat "$WORK/cpu.lines")
OUT=$L; has "cputest INFO line" '^INFO - CPU Speed Benchmark \(cputest [0-9.]+\): zlib compress \+ uncompress rate [0-9.]+ MB/s, threads=2 buf_size=2000000 repeat=1$'
has "cputest plain decimal setting" '^DETAIL buf_size=2000000$'
has "cputest total value" '^DETAIL total=[0-9]+(\.[0-9]+)?$'
has "cputest table header" '^UPLOAD columns\|Thread\|seconds$'
has "cputest table rows" '^UPLOAD row\|Thread 1\|[0-9.]+$'
if grep -q 'e+' "$WORK/cpu.lines"; then bad "no exponents in DRSM lines" "$(grep 'e+' "$WORK/cpu.lines")"; else ok "no exponents in DRSM lines"; fi
check_rc "cputest --submit --now" 0 "$BIN/cputest" --buf-size=1m --submit --now --drra="$FAKE"
has "submit message" 'Submitted to the reporting agent, hook dru_cputest \(send now\)'
OUT=$(cat "$WORK/drra.args"); has "agent arguments" '^submit --hook dru_cputest --now /.*/dru_cputest\.txt$'
OUT=$(cat "$WORK/drra.payload"); has "payload is DRSM lines" '^DETAIL benchmark=cputest$'
TMPF=$(sed 's/.* //' "$WORK/drra.args")
if [ ! -e "$TMPF" ] && [ ! -e "$(dirname "$TMPF")" ]; then ok "temporary file removed"; else bad "temporary file removed" "$TMPF"; fi
check_rc "submit=ID and --drra-config" 0 "$BIN/flops" --iterations=1m --submit=my_flops --drra-config=/x/drra.ini --drra="$FAKE"
OUT=$(cat "$WORK/drra.args"); has "-c and hook ID" '^-c /x/drra.ini submit --hook my_flops /'
check_rc "--drsm and --submit send the --drsm file" 0 "$BIN/memtest" --num-inserts=1k --drsm="$WORK/mem.lines" --submit --drra="$FAKE"
OUT=$(cat "$WORK/drra.args"); has "submits the --drsm file" "submit --hook dru_memtest $WORK/mem.lines\$"
OUT=$(cat "$WORK/mem.lines"); has "memtest phase table" '^UPLOAD row\|Reinsert\|'
check_rc "agent failure is an error" 1 env FAKE_DRRA_FAIL=1 "$BIN/cputest" --buf-size=1m --submit --drra="$FAKE"
has "agent failure message" 'agent submit failed for hook dru_cputest \(exit 4\)'
check_rc "missing agent is an error" 1 "$BIN/cputest" --buf-size=1m --submit --drra="$WORK/no_such_drra"
check_rc "--now without --submit" 1 "$BIN/cputest" --now; has "now needs submit" 'now needs --submit'
check_rc "bad hook ID" 1 "$BIN/disktest" --submit=Bad-ID; has "bad ID message" 'hook ID is a-z'
check_rc "disktest --drsm" 0 "$BIN/disktest" --num-inserts=2k --file="$WORK/disk2.btx" --drsm="$WORK/disk.lines"
OUT=$(cat "$WORK/disk.lines"); has "disktest phase times" '^DETAIL search_time=[0-9.]+$'; has "disktest ms per key" '^UPLOAD columns\|Phase\|seconds\|ms per key$'
check_rc "sar_cpu_stats --drsm" 0 "$BIN/sar_cpu_stats" --drsm="$WORK/sar.lines" "$SAR/sar_steal_spike.txt" "$SAR/sar_gap.txt"
OUT=$(cat "$WORK/sar.lines")
has "sar summary" '^INFO - sar CPU use \(sar_cpu_stats [0-9.]+\): 2 file\(s\), 7 samples'
has "sar values" '^DETAIL gaps=2$'; has "sar bad samples" '^DETAIL bad_samples=2$'; has "sar busy" '^DETAIL busy_avg=9.82$'
has "sar events" '^INFO - sar_gap.txt: gap: no samples for 70 minutes'
has "sar table row" '^UPLOAD row\|sar_gap.txt\|rhel7-2.example.com\|1\|5\|'
check_rc "sar_cpu_stats --drsm missing file" 1 "$BIN/sar_cpu_stats" --drsm="$WORK/sar2.lines" "$SAR/none.txt"
OUT=$(cat "$WORK/sar2.lines"); has "sar missing file ERROR" '^ERROR - Cannot open SAR file'
if command -v python3 > /dev/null && python3 -c 'import matplotlib' 2> /dev/null; then
  check_rc "dr_report --submit" 0 python3 "$TOP/benchmarks/plot/dr_report.py" -o "$WORK/report5.html" --submit --now --drra="$FAKE" "$WORK/cpu1.dat"
  OUT=$(cat "$WORK/drra.args"); has "report submit arguments" "^submit --hook dru_report --now $WORK/report5.html\$"
  check_rc "dr_report --now alone" 2 python3 "$TOP/benchmarks/plot/dr_report.py" -o "$WORK/report6.html" --now "$WORK/cpu1.dat"
fi
echo "--- dashboard hooks: file and network tools"
touch -d '3 hours ago' "$WORK/h3.txt"; touch "$WORK/h0.txt"
check_rc "file_hours --drsm YES" 0 "$BIN/file_hours" "$WORK/h3.txt" 2 --drsm="$WORK/fh.lines"
OUT=$(cat "$WORK/fh.lines"); has "file_hours stale" '^DETAIL stale=1$'; has "file_hours age" '^DETAIL age_hours=3.0[0-9]$'
check_rc "file_hours --submit NOT" 0 "$BIN/file_hours" "$WORK/h0.txt" 2 --submit --drra="$FAKE"
OUT=$(cat "$WORK/drra.args"); has "file_hours hook ID" '^submit --hook dru_file_hours /'
OUT=$(cat "$WORK/drra.payload"); has "file_hours not stale" '^DETAIL stale=0$'
check_rc "file_hours missing --drsm" 1 "$BIN/file_hours" "$WORK/none.txt" 2 --drsm="$WORK/fh2.lines"
OUT=$(cat "$WORK/fh2.lines"); has "file_hours ERROR line" '^ERROR - file_hours: ERR - .*none.txt does not exist$'
check_rc "file_hours usage error sends nothing" 1 "$BIN/file_hours" "$WORK/h0.txt" --drsm="$WORK/fh3.lines"
if [ -e "$WORK/fh3.lines" ]; then bad "no lines on a usage error"; else ok "no lines on a usage error"; fi
mkdir -p "$WORK/hp"; touch -d '3 days ago' "$WORK/hp/a.log"; touch "$WORK/hp/b.log"
check_rc "file_purge -n --drsm" 0 "$BIN/file_purge" -n -d2 "$WORK/hp/a.log" "$WORK/hp/b.log" --drsm="$WORK/fp.lines"
OUT=$(cat "$WORK/fp.lines"); has "file_purge counts" '^DETAIL would_purge=1$'; has "file_purge kept" '^DETAIL kept=1$'
has "file_purge rows" '^UPLOAD row\|dry run\|.*a.log would be purged'
if [ -f "$WORK/hp/a.log" ]; then ok "file_purge -n kept the file"; else bad "file_purge -n kept the file"; fi
check_rc "filetimes -n --drsm" 1 "$BIN/filetimes" -n "$WORK/h3.txt" "$WORK/h0.txt" "$WORK/none.txt" --drsm="$WORK/ft.lines"
OUT=$(cat "$WORK/ft.lines"); has "filetimes newest" '^DETAIL newest_file=.*h0.txt$'; has "filetimes errors" '^DETAIL errors=1$'
has "filetimes row" '^UPLOAD row\|.*h3.txt\|[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9:]{8}\|3.0[0-9]\|ok$'
check_rc "pingscan --drsm" 0 "$BIN/pingscan" -t 300 127.0.0.1 --drsm="$WORK/pg.lines"
OUT=$(cat "$WORK/pg.lines"); has "pingscan up" '^DETAIL up=1$'; has "pingscan row" '^UPLOAD row\|127.0.0.1\|.*\|up\|[0-9.]+$'
check_rc "dnsscan --drsm" 0 "$BIN/dnsscan" -f -p 127.0.0.1 --drsm="$WORK/dn.lines"
OUT=$(cat "$WORK/dn.lines"); has "dnsscan forward" '^DETAIL fwd_ok=1$'; has "dnsscan row" '^UPLOAD row\|127.0.0.1\|localhost\|fwd_ok\|up \('
check_rc "getmac --drsm no entry" 1 "$BIN/getmac" 127.0.0.1 --drsm="$WORK/gm.lines"
OUT=$(cat "$WORK/gm.lines"); has "getmac found 0" '^DETAIL found=0$'
check_rc "arpscan --drsm none" 1 "$BIN/arpscan" -t 200 127.0.0.1 --drsm="$WORK/ar.lines"
OUT=$(cat "$WORK/ar.lines"); has "arpscan found 0" '^DETAIL found=0$'
check_rc "internet-test --submit=ID" 1 "$BIN/internet-test" -c 1 -t 300 127.0.0.1:1 --submit=web_check --now --drra="$FAKE"
OUT=$(cat "$WORK/drra.args"); has "internet-test hook ID" '^submit --hook web_check --now /'
check_rc "pingscan agent failure" 2 env FAKE_DRRA_FAIL=1 "$BIN/pingscan" -t 300 127.0.0.1 --submit --drra="$FAKE"
has "pingscan agent failure message" 'agent submit failed for hook dru_pingscan'
check_rc "tool --now without --submit" 2 "$BIN/portscan" --now 127.0.0.1 22
for f in "$TOP"/hooks/uploads.d/*.ini; do
  id=$(basename "$f" .ini)
  if grep -q "^id=$id\$" "$f" && [ -f "$TOP/hooks/drsm/uploads/$id/plugin.conf" ] && \
     grep -q "^NAME=$id\$" "$TOP/hooks/drsm/uploads/$id/plugin.conf" && [ -x "$TOP/hooks/drsm/uploads/$id/probe.sh" ]; then
    ok "hook files $id"
  else
    bad "hook files $id" "ini id, manifest NAME and probe.sh must match"
  fi
done
check_rc "install_hooks.sh dry run" 0 "$TOP/hooks/linux/install_hooks.sh" all; has "dry run" '^Dry run'; has "would install" 'would run: install .*dru_sar_cpu_stats.ini /etc/drra/uploads.d/'
check_rc "install_hooks.sh --sar-all-cron dry run" 0 "$TOP/hooks/linux/install_hooks.sh" --sar-all-cron
has "sar_stats job" 'would run: install -D .*/sar_stats /usr/local/libexec/dr_utils/sar_stats'; has "sar_stats cron" 'dru_sar_stats.cron /etc/cron.d/dru_sar_stats'
check_rc "sar_stats --drsm" 0 "$BIN/sar_stats" --drsm="$WORK/sarall.lines" "$SAR/sar_A_rhel10.txt"
OUT=$(cat "$WORK/sarall.lines")
has "sar_stats summary" '^INFO - sar report \(sar_stats [0-9.]+\): 1 file\(s\), 35 sections, host node1.example.com'
has "sar_stats cpu value" '^DETAIL cpu_pct_idle_avg=70\.22$'; has "sar_stats disk max" '^DETAIL disk_pct_util_max='
has "sar_stats fs value" '^DETAIL fs_pct_fsused_avg='; has "sar_stats table" '^UPLOAD row\|mem\|-r\|Memory\|1\|14\|'

echo
echo "$pass passed, $fail failed"
[ "$fail" = 0 ]
