#!/bin/bash
# -----------------------------------------------------------
# Shell Script: watch_mem_usage.sh
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
# Log used memory (KB, the "used" column of free) once per INTERVAL
# seconds, one "date time KB" line each, for plotting a benchmark run.
#
#   watch_mem_usage.sh [-i seconds] [-n count] [STATSONLY]
#   STATSONLY   one line and exit
#
# Changes:
# 11/18/2026: dr_utils version, based on work from the 2009-2017
# benchmark watch script: a time on each line, -i and -n.
# -----------------------------------------------------------
interval=1; count=0
while [ $# -gt 0 ]; do
  case "$1" in
    -i) interval=$2; shift 2 ;;
    -n) count=$2; shift 2 ;;
    STATSONLY) count=1; shift ;;
    *) echo "Usage: $0 [-i seconds] [-n count] [STATSONLY]"; exit 1 ;;
  esac
done
trap 'exit 0' SIGHUP SIGINT SIGTERM SIGQUIT
n=0
while :; do
  echo "$(date '+%Y-%m-%d %H:%M:%S') $(free | awk '/^Mem:/ { print $3 }')"
  n=$((n+1))
  if [ "$count" -gt 0 ] && [ $n -ge "$count" ]; then exit 0; fi
  sleep "$interval"
done
