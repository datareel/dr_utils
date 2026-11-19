#!/bin/bash
# -----------------------------------------------------------
# Shell Script: watch_system.sh
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
# Record the system a benchmark runs on: release, kernel, CPU,
# memory, partitions, mounts and disk statistics from /proc, each
# between START name and END name lines, then "top -b" until Ctrl-C,
# and the same statistics again at the end.
#
#   watch_system.sh [-i seconds] [-n count] [STATSONLY]
#   STATSONLY   the statistics once, no top
#
# Changes:
# 11/18/2026: dr_utils version, based on work from the 2009-2017
# benchmark watch script: os-release when there is no redhat-release,
# -i and -n for top.
# -----------------------------------------------------------
interval=1; count=0; statsonly=0
while [ $# -gt 0 ]; do
  case "$1" in
    -i) interval=$2; shift 2 ;;
    -n) count=$2; shift 2 ;;
    STATSONLY) statsonly=1; shift ;;
    *) echo "Usage: $0 [-i seconds] [-n count] [STATSONLY]"; exit 1 ;;
  esac
done

Section() { # name file
  echo "START $1"
  if [ -r "$2" ]; then cat "$2"; fi
  echo "END $1"
}

EchoSystemStats() {
  if [ -r /etc/redhat-release ]; then Section RELEASE /etc/redhat-release
  else Section RELEASE /etc/os-release; fi
  Section VERSION /proc/version
  Section CPUINFO /proc/cpuinfo
  Section STAT /proc/stat
  Section ZONEINFO /proc/zoneinfo
  Section MEMINFO /proc/meminfo
  Section PARTITIONS /proc/partitions
  Section MOUNTS /proc/mounts
  Section DISKSTATS /proc/diskstats
  Section DEVICES /proc/devices
}

ExitProc() { EchoSystemStats; exit 0; }

EchoSystemStats
if [ $statsonly = 1 ]; then exit 0; fi
trap ExitProc SIGHUP SIGINT SIGTERM SIGQUIT
echo "Sampling system usage every $interval second(s)"
echo "Press Ctrl-C to stop"
echo ""
if [ "$count" -gt 0 ]; then
  top -b -d "$interval" -n "$count"
  EchoSystemStats
else
  top -b -d "$interval"
fi
