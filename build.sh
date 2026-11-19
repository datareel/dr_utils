#!/bin/bash
# Build every dr_utils tool on Linux against DataReel 6.4.1.
#
#   DATAREEL_DIR=/path/to/datareel-6.4.1 ./build.sh          build
#   DATAREEL_DIR=/path/to/datareel-6.4.1 ./build.sh clean    remove objects
#
# The DataReel library must be built first:
#   cd $DATAREEL_DIR/build && make && make install
# The programs are copied to bin/ after a build. cputest needs zlib
# (zlib-devel).
set -u
TOP=$(cd "$(dirname "$0")" && pwd)
if [ -z "${DATAREEL_DIR:-}" ]; then
  echo "Set DATAREEL_DIR to a DataReel 6.4.1 tree" >&2
  exit 2
fi
if [ ! -f "$DATAREEL_DIR/lib/libdatareel.a" ]; then
  echo "$DATAREEL_DIR/lib/libdatareel.a not found: build DataReel first (cd \$DATAREEL_DIR/build && make && make install)" >&2
  exit 2
fi
TOOLS="tools/file_hours tools/file_purge tools/filetimes tools/internet-test tools/pingscan tools/dnsscan tools/portscan tools/netpscan tools/arpscan tools/getmac"
TOOLS="$TOOLS benchmarks/cputest benchmarks/disktest benchmarks/memtest benchmarks/tcpserv benchmarks/tcptest benchmarks/udpserv benchmarks/udptest benchmarks/mcserv benchmarks/mctest benchmarks/flops system_stats/sar_cpu_stats"
fail=0
mkdir -p "$TOP/bin"
for dir in $TOOLS; do
  t=$(basename "$dir")
  # One folder at a time, never "cd X; make" (a failed cd re-runs this make)
  if [ "${1:-}" = "clean" ]; then
    (cd "$TOP/$dir" && make -s clean) || fail=1
    rm -f "$TOP/bin/$t"
    continue
  fi
  echo "== $t"
  if (cd "$TOP/$dir" && make); then
    cp -p "$TOP/$dir/$t" "$TOP/bin/$t.new" && mv -f "$TOP/bin/$t.new" "$TOP/bin/$t"
  else
    echo "** $t did not build" >&2
    fail=1
  fi
done
exit $fail
