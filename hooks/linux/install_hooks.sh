#!/bin/bash
# -----------------------------------------------------------
# Shell Script: install_hooks.sh
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
# Installs dr_utils upload hooks on a host with the DataReel Reporting
# Agent for Linux: hooks/uploads.d/<id>.ini to /etc/drra/uploads.d/.
# With --sar-cron it also installs bin/sar_cpu_stats and
# dru_sar_cpu_stats.sh in /usr/local/libexec/dr_utils/ and the daily
# schedule in /etc/cron.d/dru_sar_cpu_stats (CPU use), and with
# --sar-all-cron sar_stats, dru_sar_stats.sh and /etc/cron.d/dru_sar_stats
# (every sar section). A dry run unless --apply
# is given. The DRSM manifests (hooks/drsm/uploads/<id>) go on the
# monitor host by hand, see hooks/README.md.
#
# Usage: sudo hooks/linux/install_hooks.sh [--sar-cron] [--sar-all-cron] [--apply] all | <id> ...
#   <id> is a hook ID such as dru_cputest, or a program name (cputest)
#
# Changes:
# 11/19/2026: First version.
# 11/20/2026: --sar-all-cron (sar_stats, every sar section).
# -----------------------------------------------------------
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
TOP=$(cd "${HERE}/../.." && pwd)
INI_DIR="${TOP}/hooks/uploads.d"
UPLOADS_D=/etc/drra/uploads.d
LIBEXEC=/usr/local/libexec/dr_utils
APPLY=0
SAR=0
SARALL=0
IDS=()
for a in "$@"; do
    case "${a}" in
	--apply) APPLY=1 ;;
	--sar-cron) SAR=1 ;;
	--sar-all-cron) SARALL=1 ;;
	-h|--help) sed -n 22,33p "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
	-*) echo "Unknown option ${a}" >&2; exit 2 ;;
	all) for f in "${INI_DIR}"/dru_*.ini; do IDS+=("$(basename "${f}" .ini)"); done ;;
	*) id="${a}"; [[ "${id}" == dru_* ]] || id="dru_${id}"; IDS+=("${id}") ;;
    esac
done
if [ "${#IDS[@]}" == "0" ] && [ "${SAR}" == "0" ] && [ "${SARALL}" == "0" ]; then
    echo "Usage: $0 [--sar-cron] [--sar-all-cron] [--apply] all | <id> ...   (hooks: $(cd "${INI_DIR}" && ls dru_*.ini | sed 's/\.ini$//' | tr '\n' ' '))" >&2
    exit 2
fi
[ "${SAR}" == "1" ] && IDS+=(dru_sar_cpu_stats)
[ "${SARALL}" == "1" ] && IDS+=(dru_sar_stats)

function run()
{
    if [ "${APPLY}" == "1" ]; then
	echo "+ $*"
	"$@" || { echo "ERROR - failed: $*" >&2; exit 1; }
    else
	echo "would run: $*"
    fi
}

if [ "${APPLY}" == "1" ] && [ "$(id -u)" != "0" ]; then
    echo "Run --apply as root" >&2
    exit 1
fi
[ "${APPLY}" == "1" ] || echo "Dry run, nothing is changed (add --apply)"
if [ "$(id -u)" == "0" ] && [ ! -d "${UPLOADS_D}" ]; then
    echo "WARNING - ${UPLOADS_D} is missing: install the agent (1.0-beta2 or later) first"
fi

declare -A seen
for id in "${IDS[@]}"; do
    [ -n "${seen[${id}]:-}" ] && continue
    seen[${id}]=1
    if ! [[ "${id}" =~ ^[a-z0-9_]{1,32}$ ]] || [ ! -f "${INI_DIR}/${id}.ini" ]; then
	echo "ERROR - no upload hook ${id} in ${INI_DIR}" >&2
	exit 2
    fi
    if [ -f "${UPLOADS_D}/${id}.ini" ]; then
	echo "${UPLOADS_D}/${id}.ini is there already, left as it is"
    else
	run install -o root -g root -m 0600 "${INI_DIR}/${id}.ini" "${UPLOADS_D}/${id}.ini"
    fi
done

# The daily sar job: program, script and cron file
sar_job() { # program
    local p=$1 sh=dru_$1
    # bin/ of a source build, else the installed program (install_dr_utils.sh)
    # (sudo's PATH on RHEL has no /usr/local/bin, so look there too)
    local prog="${TOP}/bin/${p}"
    [ -x "${prog}" ] || prog=$(command -v "${p}" 2>/dev/null || true)
    [ -n "${prog}" ] || { [ -x "/usr/local/bin/${p}" ] && prog="/usr/local/bin/${p}"; }
    if [ -z "${prog}" ] || [ ! -x "${prog}" ]; then
	echo "ERROR - ${p} not found: run ./build.sh, or install the binary package with install_dr_utils.sh" >&2
	exit 1
    fi
    command -v sar > /dev/null || echo "WARNING - sar is not installed (sysstat)"
    run install -D -o root -g root -m 0755 "${prog}" "${LIBEXEC}/${p}"
    run install -D -o root -g root -m 0755 "${HERE}/${sh}.sh" "${LIBEXEC}/${sh}.sh"
    run install -o root -g root -m 0644 "${HERE}/${sh}.cron" "/etc/cron.d/${sh}"
}
[ "${SAR}" == "1" ] && sar_job sar_cpu_stats
[ "${SARALL}" == "1" ] && sar_job sar_stats
echo "DRSM manifests for the monitor host: ${TOP}/hooks/drsm/uploads/<id>"
