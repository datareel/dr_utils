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

for t in file_hours file_purge filetimes internet-test pingscan dnsscan portscan netpscan arpscan getmac; do
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
  python3 -m http.server "$PORT" --bind 127.0.0.1 --directory www > http.log 2>&1 &
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

echo
echo "$pass passed, $fail failed"
[ "$fail" = 0 ]
