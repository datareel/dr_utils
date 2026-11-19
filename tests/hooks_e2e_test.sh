#!/bin/bash
# -----------------------------------------------------------
# Shell Script: hooks_e2e_test.sh
# Produced By: DataReel Software Development Team
# File Creation Date: 11/19/2026
# Date Last Modified: 11/19/2026
# -----------------------------------------------------------
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 3 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library. If not, see
# <https://www.gnu.org/licenses/>.
#
# End to end test of the dashboard output: dr_utils programs run with
# --submit hand their results to a real DataReel Reporting Agent for
# Linux (drra submit, a test settings file), the agent sends them to
# its hook.php on a local "php -S" report server with the dr_utils
# manifests, and the DRSM upload probe runs each manifest on the
# stored data, as the dashboard does.
#
# Usage: LRA_DIR=<built agent source tree> DRSM_DIR=<DRSM tree>
#        [KEEP=1] tests/hooks_e2e_test.sh
#
# Needs dr_utils built (./build.sh), the agent built in LRA_DIR
# (make), php, curl and python3 with matplotlib (for dr_report). No
# root; only 127.0.0.1.
#
# Changes:
# 11/19/2026: First version.
# -----------------------------------------------------------
set -u
TOP=$(cd "$(dirname "$0")/.." && pwd)
BIN="${TOP}/bin"
: "${LRA_DIR:?set LRA_DIR to a built agent source tree}"
: "${DRSM_DIR:?set DRSM_DIR to a DRSM tree}"
DRRA="${LRA_DIR}/drra"
[ -x "${DRRA}" ] || { echo "Build the agent first: cd ${LRA_DIR} && make"; exit 2; }
[ -f "${LRA_DIR}/php/hook.php" ] || { echo "No ${LRA_DIR}/php/hook.php"; exit 2; }
[ -f "${DRSM_DIR}/plugins/uploads/upload_probe.sh" ] || { echo "No upload_probe.sh in ${DRSM_DIR}/plugins/uploads"; exit 2; }
[ -x "${BIN}/cputest" ] || { echo "Build dr_utils first: ./build.sh"; exit 2; }

T=$(mktemp -d "${TMPDIR:-/tmp}/dru_e2e_XXXXXX")
PORT=$(( 18000 + RANDOM % 2000 ))
SERVER_PID=""
function cleanup() {
    [ -n "${SERVER_PID}" ] && kill "${SERVER_PID}" 2> /dev/null
    if [ -z "${KEEP:-}" ]; then rm -rf "${T}"; else echo "Kept ${T}"; fi
}
trap cleanup EXIT
trap 'exit 1' INT TERM
failures=0
function pass() { echo "ok   $*"; }
function fail() { echo "FAIL $*"; failures=$((failures + 1)); }
function expect() {
    # $1 == description, $2 == file, $3 == fixed string that must be in it
    if grep -qF -- "${3}" "${2}" 2> /dev/null; then pass "${1}"; else fail "${1}: '${3}' not in ${2}"; fi
}
function reject() {
    if grep -qF -- "${3}" "${2}" 2> /dev/null; then fail "${1}: '${3}' in ${2}"; else pass "${1}"; fi
}
umask 022

# Report server with the dr_utils manifests
mkdir -p "${T}/web/upload" "${T}/data" "${T}/plugins/uploads"
cp "${LRA_DIR}"/php/*.php "${T}/web/upload/"
cp -r "${TOP}"/hooks/drsm/uploads/dru_* "${T}/plugins/uploads/"
cp "${DRSM_DIR}/plugins/uploads/upload_probe.sh" "${T}/plugins/uploads/"
cat > "${T}/data/server.ini" <<EOS
[paths]
reports_dir = ${T}/data/reports
data_dir = ${T}/data
[registration]
enrollment_token = test-token
auto_approve_with_token = 1
[hooks]
manifest_dir = ${T}/plugins/uploads
max_bytes = 20000000
EOS
(cd "${T}/web" && LRA_SERVER_INI="${T}/data/server.ini" exec php -S "127.0.0.1:${PORT}" > "${T}/server.log" 2>&1) &
SERVER_PID=$!

# Agent with the dr_utils upload hook files
mkdir -p "${T}/agent/uploads.d"
cp "${TOP}"/hooks/uploads.d/dru_*.ini "${T}/agent/uploads.d/"
chmod 600 "${T}"/agent/uploads.d/*.ini
cat > "${T}/agent/drra.ini" <<EOS
[delivery]
methods=https
[https]
host=127.0.0.1
port=${PORT}
use_https=0
upload_path=/upload/upload.php
register_path=/upload/register.php
enrollment_token=test-token
[agent]
report_dir=${T}/agent/var
data_dir=${T}/agent/var
log_file=${T}/agent/drra.log
[uploads]
dir=${T}/agent/uploads.d
spool_dir=${T}/agent/spool
EOS
A=("${DRRA}" -c "${T}/agent/drra.ini")
HOST=$("${A[@]}" fingerprint 2> /dev/null | sed -n 's/^Host: //p')
for i in $(seq 1 50); do curl -s -o /dev/null "http://127.0.0.1:${PORT}/upload/hook.php" && break; sleep 0.1; done
SUB=(--drra="${DRRA}" --drra-config="${T}/agent/drra.ini")

echo "--- programs submit to the agent"
cd "${T}" || exit 2
"${BIN}/cputest" --num-threads=2 --buf-size=2m --datfile=cpu.dat --submit "${SUB[@]}" > cpu.out 2>&1
expect "cputest submitted" cpu.out "Submitted to the reporting agent, hook dru_cputest"
"${BIN}/disktest" --num-inserts=2k --file="${T}/d.btx" --submit "${SUB[@]}" > disk.out 2>&1
expect "disktest submitted" disk.out "hook dru_disktest"
"${BIN}/memtest" --num-inserts=1k --submit "${SUB[@]}" > mem.out 2>&1
expect "memtest submitted" mem.out "hook dru_memtest"
"${BIN}/flops" --iterations=1m --submit "${SUB[@]}" > flops.out 2>&1
expect "flops submitted" flops.out "hook dru_flops"
UP=$(( 20000 + RANDOM % 2000 ))
"${BIN}/tcpserv" -p${UP} -l127.0.0.1 --count=1 > tcpserv.out 2>&1 &
SRV=$!
sleep 0.5
"${BIN}/tcptest" -p${UP} -s2M --submit "${SUB[@]}" 127.0.0.1 > tcp.out 2>&1
expect "tcptest submitted" tcp.out "hook dru_tcptest"
wait ${SRV}
"${BIN}/udpserv" -p${UP} -l127.0.0.1 --count=1 > udpserv.out 2>&1 &
SRV=$!
sleep 0.5
"${BIN}/udptest" -p${UP} -s1M -r100M --submit "${SUB[@]}" 127.0.0.1 > udp.out 2>&1
expect "udptest submitted" udp.out "hook dru_udptest"
wait ${SRV}
"${BIN}/mctest" -i127.0.0.1 -p${UP} --timeout=10 --submit "${SUB[@]}" > mc.out 2>&1 &
MC=$!
sleep 0.5
"${BIN}/mcserv" -i127.0.0.1 -p${UP} -s1M -r50M > mcserv.out 2>&1
wait ${MC}
expect "mctest submitted" mc.out "hook dru_mctest"
"${BIN}/sar_cpu_stats" --submit "${SUB[@]}" "${TOP}/tests/data/sar/sar_rhel10.txt" > sar.out 2>&1
expect "sar_cpu_stats submitted" sar.out "hook dru_sar_cpu_stats"
python3 "${TOP}/benchmarks/plot/dr_report.py" -o "${T}/report.html" --submit "${SUB[@]}" cpu.dat > report.out 2>&1
expect "dr_report submitted" report.out "hook dru_report"

# The agent takes spooled files two seconds after their last change
sleep 3
"${A[@]}" send-uploads > send.out 2>&1
for id in dru_cputest dru_disktest dru_memtest dru_flops dru_tcptest dru_udptest dru_mctest dru_sar_cpu_stats dru_report; do
    expect "${id} sent" send.out "Upload ${id} sent: STATUS: ok"
done

echo "--- report server parsed the lines"
H="${T}/data/hooks/${HOST}"
expect "cputest INFO kept" "${H}/dru_cputest/latest.lines" "INFO - CPU Speed Benchmark (cputest"
expect "cputest value" "${H}/dru_cputest/latest.lines" "UPLOAD value|threads|2"
expect "cputest table" "${H}/dru_cputest/latest.lines" "UPLOAD columns|Thread|seconds"
expect "disktest phase row" "${H}/dru_disktest/latest.lines" "UPLOAD row|Reinsert|"
expect "udptest loss value" "${H}/dru_udptest/latest.lines" "UPLOAD value|loss_max_pct|"
expect "sar busy value" "${H}/dru_sar_cpu_stats/latest.lines" "UPLOAD value|busy_avg|"
expect "sar iowait spikes" "${H}/dru_sar_cpu_stats/latest.lines" "UPLOAD value|iowait_spikes|4"
expect "report kept as HTML" "${H}/dru_report/latest.data" "<html"
expect "report format" "${H}/dru_report/latest.meta" "FORMAT: html"
reject "no PHP warnings" "${T}/server.log" "PHP Warning"

echo "--- DRSM upload probe with the dr_utils manifests"
function probe() {
    # $1 == hook ID: run its probe.sh with the manifest's settings, as drsm_run.sh does
    local id=$1 dir="${T}/plugins/uploads/$1" envs=() k v
    while IFS='=' read -r k v; do
	[[ "${k}" =~ ^(TITLE|DISPLAY|MAX_AGE_HOURS|WARN_IF|CRIT_IF|METRICS|MAX_LINES)$ ]] || continue
	v="${v#\"}"; v="${v%\"}"
	envs+=("DRSM_${k}=${v}")
    done < "${dir}/plugin.conf"
    (cd "${dir}" && env DRSM_LIB="${DRSM_DIR}/lib" DRSM_TARGET="${HOST}" DRSM_UPLOADS_DIR="${T}/data/hooks" \
	DRSM_UPLOAD_NAME="${HOST}" DRSM_UPLOAD_ID="${id}" "${envs[@]}" bash ./probe.sh) > "${T}/probe_${id}.out" 2>&1
}
for id in dru_cputest dru_disktest dru_udptest dru_mctest dru_sar_cpu_stats dru_report; do probe "${id}"; done
expect "cputest probe meta" "${T}/probe_dru_cputest.out" "UPLOAD meta|${HOST%%.*}|dru_cputest|table|CPU benchmark (cputest)|"
expect "cputest probe metric" "${T}/probe_dru_cputest.out" "METRIC dru_cputest_total="
expect "cputest probe rows" "${T}/probe_dru_cputest.out" "UPLOAD row|Thread 0|"
reject "cputest no warning" "${T}/probe_dru_cputest.out" "WARNING - "
expect "disktest probe metric" "${T}/probe_dru_disktest.out" "METRIC dru_disktest_insert_time="
expect "udptest probe loss metric" "${T}/probe_dru_udptest.out" "METRIC dru_udptest_loss_max_pct="
expect "mctest probe metric" "${T}/probe_dru_mctest.out" "METRIC dru_mctest_total="
expect "sar probe warns on iowait spikes" "${T}/probe_dru_sar_cpu_stats.out" "iowait_spikes > 0 (iowait_spikes is 4)"
expect "sar probe busy metric" "${T}/probe_dru_sar_cpu_stats.out" "METRIC dru_sar_cpu_stats_busy_avg="
expect "report probe html display" "${T}/probe_dru_report.out" "UPLOAD meta|${HOST%%.*}|dru_report|html|Benchmark report|"
reject "report probe no error" "${T}/probe_dru_report.out" "ERROR - "

echo
if [ ${failures} -eq 0 ]; then echo "All tests passed"; else echo "${failures} tests failed"; fi
exit $((failures > 0))
