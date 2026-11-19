#!/bin/bash
# -----------------------------------------------------------
# Shell Script: dru_sar_cpu_stats.sh
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
# Send one day of sar CPU use to the dashboard: makes the sar text
# report of the day (yesterday by default) and runs
# sar_cpu_stats --submit, which hands it to the DataReel Reporting
# Agent (hook dru_sar_cpu_stats). Run it from cron once a day, see
# dru_sar_cpu_stats.cron.
#
#   dru_sar_cpu_stats.sh [--day=yesterday|today|YYYY-MM-DD]
#                        [--sa-dir=/var/log/sa] [--now] [--print]
#                        [other sar_cpu_stats options]
#
#   --print   print the DRSM lines instead of submitting them
#
# sar_cpu_stats is found next to this script, then in the PATH, or
# set SAR_CPU_STATS=/path/to/sar_cpu_stats. Needs sysstat (sar).
# drra submit needs root or the drra-upload group.
#
# Changes:
# 11/19/2026: First version.
# -----------------------------------------------------------
set -u
DAY=yesterday
SA_DIR=/var/log/sa
PRINT=0
ARGS=()
for a in "$@"; do
    case "${a}" in
	--day=*) DAY="${a#--day=}" ;;
	--sa-dir=*) SA_DIR="${a#--sa-dir=}" ;;
	--print) PRINT=1 ;;
	-h|--help) sed -n 22,35p "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
	*) ARGS+=("${a}") ;;
    esac
done

HERE=$(cd "$(dirname "$0")" && pwd)
PROG="${SAR_CPU_STATS:-}"
if [ -z "${PROG}" ]; then
    if [ -x "${HERE}/sar_cpu_stats" ]; then PROG="${HERE}/sar_cpu_stats"
    else PROG=$(command -v sar_cpu_stats || true); fi
fi
if [ -z "${PROG}" ] || [ ! -x "${PROG}" ]; then
    echo "ERROR - sar_cpu_stats not found: put it in the PATH or set SAR_CPU_STATS" >&2
    exit 1
fi
command -v sar > /dev/null || { echo "ERROR - sar not found (install sysstat)" >&2; exit 1; }

if ! D=$(date -d "${DAY}" +%Y%m%d 2> /dev/null); then
    echo "ERROR - bad --day ${DAY}" >&2
    exit 2
fi
# sysstat names the daily files saDD, or saYYYYMMDD when set to
SA_FILE=""
for f in "${SA_DIR}/sa${D}" "${SA_DIR}/sa${D:6:2}"; do
    if [ -f "${f}" ]; then SA_FILE="${f}"; break; fi
done
if [ -z "${SA_FILE}" ]; then
    echo "ERROR - no sar data file for ${D} in ${SA_DIR}" >&2
    exit 1
fi

WORK=$(mktemp -d "${TMPDIR:-/tmp}/dru_sar_XXXXXX") || exit 1
trap 'rm -rf "${WORK}"' EXIT
TXT="${WORK}/sar_$(hostname -s)_${D}.txt"
# sar -u reads only the CPU lines; LC_ALL=C for the 24-hour times
if ! LC_ALL=C sar -u -f "${SA_FILE}" > "${TXT}" 2> "${WORK}/err"; then
    echo "ERROR - sar -u -f ${SA_FILE} failed: $(head -1 "${WORK}/err")" >&2
    exit 1
fi
if [ "${PRINT}" == "1" ]; then
    "${PROG}" --drsm="${WORK}/lines" "${ARGS[@]}" "${TXT}" > /dev/null
    rc=$?
    cat "${WORK}/lines" 2> /dev/null
    exit ${rc}
fi
"${PROG}" --submit "${ARGS[@]}" "${TXT}"
