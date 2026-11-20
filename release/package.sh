#!/bin/bash
# -----------------------------------------------------------
# Shell Script: package.sh
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
# Makes the dr_utils release packages, the same layout as the fcrypt
# packages:
#   dr_utils-<v>.tar.gz, dr_utils-<v>.zip               source code
#   dr_utils_<v_v>_rhel_7_8_9_10_binaries.tar.gz        rhel7/ ... rhel10/
#                                                       + install_dr_utils.sh
#   dr_utils_<v_v>_windows_binaries.zip                 64-bit
#   dr_utils_<v_v>_windows_32bit_binaries.zip           32-bit
# each with a .sha256sum file, plus sha256sum_dr_utils_<v>.txt.
#
#   release/package.sh COMMIT BUILDDIR OUTDIR
#     COMMIT    the release commit or tag (the source and the docs)
#     BUILDDIR  the builds: rhel7/ rhel8/ rhel9/ rhel10/ (programs) and
#               win64/ win32/ (.exe files)
#     OUTDIR    where the packages go (made fresh, must not exist)
#
# Every package gets docs/ (the HTML documentation and the markdown),
# hooks/ (reporting agent hook files), the scripts and the LICENSE.
#
# Changes:
# 11/20/2026: First version.
# -----------------------------------------------------------
set -eu
[ $# -eq 3 ] || { sed -n 30,35p "$0"; exit 2; }
COMMIT=$1
B=$(cd "$2" && pwd)
OUT=$3
REPO=$(cd "$(dirname "$0")/.." && pwd)
[ -e "${OUT}" ] && { echo "ERROR: ${OUT} exists"; exit 1; }
V=$(git -C "${REPO}" show "${COMMIT}:version.txt" | tr -d '[:space:]')
VU=${V//./_}
PROGS="file_hours file_purge filetimes internet-test pingscan dnsscan portscan netpscan arpscan getmac
cputest disktest memtest tcpserv tcptest udpserv udptest mcserv mctest flops sar_cpu_stats"
SCRIPTS_PY="benchmarks/plot/dr_plot.py benchmarks/plot/dr_report.py"
SCRIPTS_SH="benchmarks/scripts/watch_system.sh benchmarks/scripts/watch_mem.sh benchmarks/scripts/watch_mem_usage.sh"
mkdir -p "${OUT}"
OUT=$(cd "${OUT}" && pwd)
SRC=${OUT}/.src
mkdir -p "${SRC}" && git -C "${REPO}" archive "${COMMIT}" | tar -x -C "${SRC}"

# ---- checks: every build has every program, built from this version
for b in rhel7 rhel8 rhel9 rhel10 win64 win32; do
    for p in ${PROGS}; do
	f=${B}/${b}/${p}; case ${b} in win*) f=${f}.exe ;; esac
	[ -f "${f}" ] || { echo "ERROR: ${f} missing"; exit 1; }
	grep -aq "dr_utils" "${f}" || { echo "ERROR: ${f} is not a dr_utils program"; exit 1; }
    done
done
for b in rhel7 rhel8 rhel9 rhel10; do
    if ldd "${B}/${b}/portscan" 2>/dev/null | grep -qiE 'libssl|libcrypto'; then echo "ERROR: ${b} links OpenSSL"; exit 1; fi
done
if [ -x "${B}/rhel10/cputest" ]; then
    "${B}/rhel10/cputest" --version | grep -qE "^cputest version ${V}( |$)" || { echo "ERROR: rhel10/cputest is not version ${V}"; exit 1; }
fi

common() { # target dir, line end (lf or crlf)
    mkdir -p "$1/docs" "$1/hooks"
    cp -a "${SRC}/docs/html" "$1/docs/"
    cp -p "${SRC}"/docs/*.md "$1/docs/"
    cp -a "${SRC}"/hooks/. "$1/hooks/"
    cp -p "${SRC}/LICENSE" "$1/"
    printf '%s\n' "${V}" > "$1/VERSION"
}

# ---- source code
(cd "${OUT}" && git -C "${REPO}" archive --format=tar.gz --prefix="dr_utils-${V}/" -o "${OUT}/dr_utils-${V}.tar.gz" "${COMMIT}")
(cd "${OUT}" && git -C "${REPO}" archive --format=zip --prefix="dr_utils-${V}/" -o "${OUT}/dr_utils-${V}.zip" "${COMMIT}")

# ---- Linux
LIN=dr_utils_${VU}_rhel_7_8_9_10_binaries
L=${OUT}/${LIN}
mkdir -p "${L}/scripts"
for b in rhel7 rhel8 rhel9 rhel10; do
    mkdir -p "${L}/${b}"
    for p in ${PROGS}; do cp -p "${B}/${b}/${p}" "${L}/${b}/"; done
    (cd "${L}/${b}" && sha256sum ${PROGS} > sha256sum.txt)
done
for s in ${SCRIPTS_PY} ${SCRIPTS_SH}; do cp -p "${SRC}/${s}" "${L}/scripts/"; done
(cd "${L}/scripts" && sha256sum * > ../scripts.sha256sum.txt)
cp -p "${SRC}/release/install_dr_utils.sh" "${L}/" && chmod 755 "${L}/install_dr_utils.sh"
common "${L}"
sed "s/@VERSION@/${V}/g" "${SRC}/release/README_linux.txt" > "${L}/README.txt"
(cd "${OUT}" && tar -czf "${LIN}.tar.gz" --owner=0 --group=0 "${LIN}")

# ---- Windows
for arch in x64 x86; do
    if [ ${arch} = x64 ]; then W=dr_utils_${VU}_windows_binaries; b=win64; else W=dr_utils_${VU}_windows_32bit_binaries; b=win32; fi
    D=${OUT}/${W}
    mkdir -p "${D}/bin" "${D}/scripts"
    for p in ${PROGS}; do cp -p "${B}/${b}/${p}.exe" "${D}/bin/"; done
    for s in ${SCRIPTS_PY}; do cp -p "${SRC}/${s}" "${D}/scripts/"; done
    (cd "${D}" && sha256sum bin/*.exe scripts/*.py > sha256sum.txt)
    cp -p "${SRC}/release/install_dr_utils.ps1" "${D}/"
    common "${D}"
    printf '%s\n' "${arch}" > "${D}/ARCH"
    sed "s/@VERSION@/${V}/g; s/@ARCH@/${arch}/g" "${SRC}/release/README_windows.txt" | sed 's/$/\r/' > "${D}/README.txt"
    (cd "${OUT}" && rm -f "${W}.zip" && zip -qr "${W}.zip" "${W}")
done

# ---- checksums
cd "${OUT}"
for f in dr_utils-${V}.tar.gz dr_utils-${V}.zip ${LIN}.tar.gz dr_utils_${VU}_windows_binaries.zip dr_utils_${VU}_windows_32bit_binaries.zip; do
    sha256sum "${f}" > "${f}.sha256sum"
done
cat *.sha256sum > "sha256sum_dr_utils_${V}.txt"
rm -rf "${SRC}"
ls -la "${OUT}"
