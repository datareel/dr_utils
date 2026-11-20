#!/bin/bash
# -----------------------------------------------------------
# Shell Script: install_dr_utils.sh
# Produced By: DataReel Software Development Team
# File Creation Date: 11/20/2026
# Date Last Modified: 11/20/2026
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
# Installs the dr_utils programs from the Linux binary package
# (dr_utils_<version>_rhel_7_8_9_10_binaries.tar.gz), choosing the
# build for this host from /etc/redhat-release:
#   RHEL 7 rhel7/   RHEL 8 rhel8/   RHEL 9 rhel9/   RHEL 10 rhel10/
# (Rocky, AlmaLinux and other RHEL rebuilds the same way.) The scripts
# in scripts/ (dr_plot.py, dr_report.py, watch_*.sh) are installed too.
#
# Dry run by default: it shows what it would copy. Run with --install to
# copy. Nothing is removed; programs already in the target folder are
# kept as name.backup-YYYYMMDD-HHMMSS.
#
#   ./install_dr_utils.sh [--install] [--system | --prefix=DIR] [--build=DIR]
#     --system      install in /usr/local/bin (run as root)
#     --prefix=DIR  install in DIR (default: ~/bin)
#     --build=DIR   use this build folder of the package (rhel7, rhel8,
#                   rhel9, rhel10) instead of the one chosen for the host
#
# The reporting agent hook files (results to the DRSM dashboard) have
# their own installer: hooks/linux/install_hooks.sh, see docs.
#
# Changes:
# 11/20/2026: First version, the same steps as install_fcrypt.sh.
# -----------------------------------------------------------
set -u
PKG=$(cd "$(dirname "$0")" && pwd)
VERSION=$(cat "${PKG}/VERSION" 2>/dev/null || echo unknown)
GO=0
PREFIX=${HOME}/bin
BUILD=""
for a in "$@"; do
    case "$a" in
	--install) GO=1 ;;
	--system) PREFIX=/usr/local/bin ;;
	--prefix=*) PREFIX=${a#--prefix=} ;;
	--build=*) BUILD=${a#--build=} ;;
	-h|--help) sed -n 22,40p "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
	*) echo "ERROR: unknown option $a, see $0 --help"; exit 1 ;;
    esac
done

if [ -z "${BUILD}" ]; then
    if [ ! -f /etc/redhat-release ]; then
	echo "ERROR: /etc/redhat-release not found: this package is for RHEL 7, 8, 9 and 10"
	echo "       (on another distribution try --build=rhel10, or build from the source code)"
	exit 1
    fi
    REL=$(sed -n 's/.*release \([0-9]*\).*/\1/p' /etc/redhat-release | head -1)
    case "${REL}" in
	7|8|9|10) BUILD=rhel${REL} ;;
	*) echo "ERROR: $(cat /etc/redhat-release) is not supported, the package is for RHEL 7, 8, 9 and 10"; exit 1 ;;
    esac
fi
SRC=${PKG}/${BUILD}
if [ ! -f "${SRC}/sha256sum.txt" ]; then
    echo "ERROR: ${SRC} not found in the package"
    exit 1
fi
PROGS=$(awk '{print $2}' "${SRC}/sha256sum.txt" | sed 's/^\*//')
SCRIPTS=$(cd "${PKG}/scripts" 2>/dev/null && ls)

[ -f /etc/redhat-release ] && echo "Host: $(cat /etc/redhat-release)"
echo "dr_utils ${VERSION}, build: ${BUILD}"

(cd "${SRC}" && sha256sum -c --quiet sha256sum.txt) || { echo "ERROR: the sha256 check of ${SRC} failed, nothing was installed"; exit 1; }
echo "sha256 check of ${SRC}: OK"
if [ -f "${PKG}/scripts.sha256sum.txt" ]; then
    (cd "${PKG}/scripts" && sha256sum -c --quiet ../scripts.sha256sum.txt) || { echo "ERROR: the sha256 check of scripts/ failed, nothing was installed"; exit 1; }
fi

# The programs need only the C and C++ runtime and zlib: check that one runs
if ! "${SRC}/cputest" --version 2>&1 | grep -q "cputest version ${VERSION}"; then
    echo "ERROR: ${SRC}/cputest does not run on this host, nothing was installed"
    "${SRC}/cputest" --version 2>&1 | tail -3
    exit 1
fi

stamp=$(date +%Y%m%d-%H%M%S)
for p in ${PROGS}; do
    [ -e "${PREFIX}/${p}" ] && echo "$([ ${GO} -eq 1 ] || echo 'DRY RUN: ')keep ${PREFIX}/${p} as ${PREFIX}/${p}.backup-${stamp}"
    echo "$([ ${GO} -eq 1 ] || echo 'DRY RUN: ')install ${SRC}/${p} -> ${PREFIX}/${p}"
done
for p in ${SCRIPTS}; do
    [ -e "${PREFIX}/${p}" ] && echo "$([ ${GO} -eq 1 ] || echo 'DRY RUN: ')keep ${PREFIX}/${p} as ${PREFIX}/${p}.backup-${stamp}"
    echo "$([ ${GO} -eq 1 ] || echo 'DRY RUN: ')install ${PKG}/scripts/${p} -> ${PREFIX}/${p}"
done
if [ ${GO} -eq 0 ]; then
    echo "Dry run only, nothing was changed. Run with --install to install."
    exit 0
fi

mkdir -p "${PREFIX}" || exit 1
inst() { # source name
    if [ -e "${PREFIX}/$2" ]; then
	mv "${PREFIX}/$2" "${PREFIX}/$2.backup-${stamp}" || exit 1
    fi
    # Copy to a new name, then rename, so a running copy is never overwritten
    cp -p "$1" "${PREFIX}/.$2.new" && chmod 755 "${PREFIX}/.$2.new" && mv "${PREFIX}/.$2.new" "${PREFIX}/$2" || exit 1
}
for p in ${PROGS}; do inst "${SRC}/${p}" "${p}"; done
for p in ${SCRIPTS}; do inst "${PKG}/scripts/${p}" "${p}"; done
echo "Installed in ${PREFIX}: $(echo ${PROGS} ${SCRIPTS} | wc -w) programs and scripts"
"${PREFIX}/cputest" --version 2>&1 | head -1
case ":${PATH}:" in
    *":${PREFIX}:"*) ;;
    *) echo "Note: ${PREFIX} is not in your PATH" ;;
esac
command -v python3 > /dev/null && python3 -c 'import matplotlib' 2>/dev/null || \
    echo "Note: dr_plot.py and dr_report.py need Python 3 with matplotlib (dnf install python3-matplotlib)"
exit 0
