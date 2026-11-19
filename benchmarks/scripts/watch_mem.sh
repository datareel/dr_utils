#!/bin/bash
# -----------------------------------------------------------
# Shell Script: watch_mem.sh
# Produced By: DataReel Software Development Team
# File Creation Date: 07/15/2009
# Date Last Modified: 11/18/2026
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
# Watch total memory use while a benchmark runs: "free" and
# /proc/meminfo every INTERVAL seconds, and once more at the end.
#
#   watch_mem.sh [-i seconds] [-n count] [STATSONLY]
#   STATSONLY   print /proc/meminfo once and exit
#
# Changes:
# 11/18/2026: dr_utils version, based on Douglas Gaer's 2009-2017
# benchmark watch script: -i and -n.
# -----------------------------------------------------------
interval=1; count=0
while [ $# -gt 0 ]; do
  case "$1" in
    -i) interval=$2; shift 2 ;;
    -n) count=$2; shift 2 ;;
    STATSONLY) cat /proc/meminfo; exit 0 ;;
    *) echo "Usage: $0 [-i seconds] [-n count] [STATSONLY]"; exit 1 ;;
  esac
done
ExitProc() { free; cat /proc/meminfo; exit 0; }
trap ExitProc SIGHUP SIGINT SIGTERM SIGQUIT
echo "Sampling total memory usage every $interval second(s)"
echo "Press Ctrl-C to stop"
echo ""
n=0
while :; do
  date '+%Y-%m-%d %H:%M:%S'
  free
  cat /proc/meminfo
  n=$((n+1))
  if [ "$count" -gt 0 ] && [ $n -ge "$count" ]; then exit 0; fi
  sleep "$interval"
done
