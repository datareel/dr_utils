#!/usr/bin/env python3
# -----------------------------------------------------------
# Python Script File: plot_disk_benchmark.py
# Produced By: DataReel Software Development Team
# File Creation Date: 2016
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
# Plot the disk benchmark data file written by disktest --datfile.
#
#   plot_disk_benchmark.py disk.dat             show the plot
#   plot_disk_benchmark.py disk.dat disk.png    write a PNG file
#   --averages                                  plot the milliseconds
#                                               per key, not the times
#
# Needs Python 3 with matplotlib (python3-matplotlib).
#
# Changes:
# 11/18/2026: dr_utils version, based on Douglas Gaer's 2016 Python 2
# plot_disk_benchmarks.py: Python 3, matplotlib only (the old script
# also imported basemap), --averages instead of plot_type in the file,
# the time values plotted as numbers.
# -----------------------------------------------------------
import argparse
import configparser
import os
import sys


def floats(cfg, key):
    return [float(v) for v in cfg.get('DISKBENCHMARK', key).split(',') if v]


def main():
    ap = argparse.ArgumentParser(description='Plot disktest --datfile output')
    ap.add_argument('datfile')
    ap.add_argument('png', nargs='?', help='write this PNG file instead of showing the plot')
    ap.add_argument('--averages', action='store_true',
                    help='plot milliseconds per key instead of seconds per 10%% of the keys')
    args = ap.parse_args()

    if not os.path.isfile(args.datfile):
        print('ERROR - Cannot open ' + args.datfile)
        return 1
    cfg = configparser.ConfigParser(interpolation=None)
    cfg.read(args.datfile)
    if not cfg.has_section('DISKBENCHMARK'):
        print('ERROR - ' + args.datfile + ' has no DISKBENCHMARK section')
        return 1

    import matplotlib
    if args.png:
        matplotlib.use('Agg')
    import matplotlib.pyplot as plt

    suffix = '_avg' if args.averages else ''
    series = [('write_time', 'Write time', 'blue'),
              ('read_time', 'Read time', 'green'),
              ('remove_time', 'Delete time', 'red'),
              ('rewrite_time', 'Rewrite time', 'darkblue')]
    labels = cfg.get('DISKBENCHMARK', 'xticks_str').split(',')
    for key, label, color in series:
        plt.plot(range(len(labels)), floats(cfg, key + suffix), color=color, label=label)
    plt.ylabel('Milliseconds per key' if args.averages else 'Seconds per 10% of the keys', fontsize=12)
    plt.legend(loc='upper right', fancybox=True, shadow=True,
               prop=dict(size=8, weight='bold'), ncol=4)
    plt.grid(True, which='major', linestyle='dotted')
    plt.xticks(range(len(labels)), labels, rotation=75)
    plt.xlabel('Number of Btree Index Keys', fontsize=12)

    title = 'Btree Disk Speed Test For ' + cfg.get('DISKBENCHMARK', 'num_inserts_str') + ' Keys'
    if cfg.has_section('HOSTINFO'):
        h = cfg['HOSTINFO']
        title = '%s: %s %s %s build' % (h.get('title', 'Disk Speed Benchmark'), h.get('host', ''),
                                        h.get('date', ''), h.get('build', ''))
    plt.title(title, fontsize=12, weight='bold')
    plt.subplots_adjust(left=.14, bottom=.16, right=.91, top=.90)
    dpi = cfg.getint('PLOTINFO', 'DPI', fallback=75)
    if args.png:
        plt.savefig(args.png, dpi=dpi, bbox_inches='tight', pad_inches=0.1)
        print('Plot file = ' + args.png)
    else:
        plt.show()
    return 0


if __name__ == '__main__':
    sys.exit(main())
