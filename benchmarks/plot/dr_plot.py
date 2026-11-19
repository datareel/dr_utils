#!/usr/bin/env python3
# -----------------------------------------------------------
# Python Script File: dr_plot.py
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
# Plot the dr_utils benchmark and system statistics files. The kind
# of each file is found from its contents:
#
#   disktest --datfile          insert/find/delete/reinsert times for
#                               every 10% of the keys
#   cputest, memtest, flops,    one bar per thread, client, receiver
#   tcptest, udptest, mctest    or phase, with the total in the title
#   --datfile                   (UDP and multicast also show the loss)
#   sar_cpu_stats --csv         CPU use over time, stacked (user, nice,
#                               system, irq+soft, iowait, steal), with
#                               the bad samples marked
#   watch_mem_usage.sh output   used memory over time
#
# Several benchmark files of the same kind are compared: one bar per
# file (host, date and settings) for the main result. Several disk
# files are compared phase by phase.
#
#   dr_plot.py cpu.dat                       show the plot
#   dr_plot.py -o cpu.png cpu.dat            write a PNG file
#   dr_plot.py -o cpus.png host1.dat host2.dat     compare runs
#   dr_plot.py -o node1.png node1.csv        sar CPU use
#   dr_plot.py --cpu 2 node1.csv             one CPU (needs --all-cpus)
#   dr_plot.py --averages disk.dat           milliseconds per key
#
# Needs Python 3 with matplotlib (python3-matplotlib).
#
# Changes:
# 11/19/2026: First version, from the 2016 Python 2 disk benchmark
# plotter (plot_disk_benchmarks.py) extended to every benchmark and
# to sar and memory logs. It replaces plot_disk_benchmark.py.
# -----------------------------------------------------------
import argparse
import configparser
import csv
import datetime
import os
import sys

DISK_SERIES = [('write_time', 'Write', 'tab:blue'),
               ('read_time', 'Read', 'tab:green'),
               ('remove_time', 'Delete', 'tab:red'),
               ('rewrite_time', 'Rewrite', 'navy')]
DISK_TOTALS = [('insert_time', 'Insert'), ('search_time', 'Find'),
               ('delete_time', 'Delete'), ('reinsert_time', 'Reinsert')]
SAR_STACK = [('user', 'user', 'tab:blue'), ('nice', 'nice', 'tab:cyan'),
             ('system', 'system', 'tab:orange'), ('irqsoft', 'irq + soft', 'tab:olive'),
             ('iowait', 'iowait', 'tab:purple'), ('steal', 'steal', 'tab:red')]
# Settings shown in the comparison labels, by benchmark
SETTINGS = {'cputest': ['threads', 'buf_size'], 'memtest': ['threads', 'num_inserts'],
            'flops': ['threads', 'precision', 'mode'],
            'tcptest': ['clients', 'block', 'direction'],
            'udptest': ['clients', 'datagram'], 'mctest': ['receivers']}


class PlotError(Exception):
    pass


def shn(v):
    """A short count: 1000000 -> 1M"""
    try:
        v = float(v)
    except ValueError:
        return v
    for div, unit in ((1e9, 'G'), (1e6, 'M'), (1e3, 'K')):
        if v >= div:
            s = '%.1f' % (v / div)
            return (s[:-2] if s.endswith('.0') else s) + unit
    return '%g' % v


def floats(text):
    return [float(v) for v in text.split(',') if v.strip() != '']


def read_kind(path):
    """Returns ('disk'|'bench'|'sar'|'mem', data)"""
    if not os.path.isfile(path):
        raise PlotError('cannot open ' + path)
    with open(path, 'r', errors='replace') as f:
        head = f.read(4096)
    if head.startswith('[DISKBENCHMARK]') or head.startswith('[BENCHMARK]'):
        cfg = configparser.ConfigParser(interpolation=None)
        cfg.read(path)
        host = dict(cfg['HOSTINFO']) if cfg.has_section('HOSTINFO') else {}
        if cfg.has_section('DISKBENCHMARK'):
            return 'disk', (dict(cfg['DISKBENCHMARK']), host)
        return 'bench', (dict(cfg['BENCHMARK']), host)
    if head.startswith('host,time,cpu,'):
        return 'sar', None
    first = head.splitlines()[0] if head else ''
    parts = first.split()
    if len(parts) == 3 and parts[2].isdigit() and parts[0].count('-') == 2:
        return 'mem', None
    raise PlotError(path + ' is not a dr_utils benchmark, sar_cpu_stats --csv '
                    'or watch_mem_usage.sh file')


def host_title(host, default):
    if not host:
        return default
    return '%s: %s %s %s build' % (host.get('title', default), host.get('host', ''),
                                   host.get('date', ''), host.get('build', ''))


def plot_disk(plt, files, averages):
    if len(files) == 1:
        d, host = files[0][1]
        suffix = '_avg' if averages else ''
        labels = d['xticks_str'].split(',')
        x = range(len(labels))
        for key, label, color in DISK_SERIES:
            plt.plot(x, floats(d[key + suffix]), color=color, label=label + ' time', marker='o',
                     markersize=3)
        plt.ylabel('Milliseconds per key' if averages else 'Seconds per 10% of the keys')
        plt.xticks(x, labels, rotation=75)
        plt.xlabel('Number of B-tree index keys')
        plt.legend(loc='upper right', fontsize=8, ncol=4)
        plt.title(host_title(host, 'Disk Speed Benchmark') + '\n' +
                  'B-tree disk speed test for %s keys' % d.get('num_inserts_str', ''),
                  fontsize=10, weight='bold')
        return
    # Compare: grouped bars of the phase totals
    n = len(files)
    width = 0.8 / n
    for i, (path, (d, host)) in enumerate(files):
        vals = [float(d.get(k, 0)) for k, _ in DISK_TOTALS]
        xs = [j + i * width for j in range(len(DISK_TOTALS))]
        label = '%s %s (%s keys)' % (host.get('host', os.path.basename(path)),
                                     host.get('date', ''), d.get('num_inserts_str', ''))
        plt.bar(xs, vals, width=width, label=label)
    plt.xticks([j + width * (n - 1) / 2 for j in range(len(DISK_TOTALS))],
               [label for _, label in DISK_TOTALS])
    plt.ylabel('Seconds')
    plt.legend(fontsize=8)
    plt.title('Disk Speed Benchmark comparison', fontsize=11, weight='bold')


def bench_label(path, d, host):
    parts = [host.get('host', os.path.basename(path)).split('.')[0], host.get('date', '')]
    for key in SETTINGS.get(d.get('name', ''), []):
        if key in d:
            val = shn(d[key]) if key in ('buf_size', 'num_inserts') else d[key]
            parts.append('%s=%s' % (key.replace('_', ' '), val))
    return '\n'.join([parts[0] + ' ' + parts[1], ', '.join(parts[2:])])


def plot_bench(plt, files):
    names = set(d['name'] for _, (d, _) in files)
    if len(names) > 1:
        raise PlotError('cannot compare different benchmarks: ' + ', '.join(sorted(names)))
    if len(files) == 1:
        path, (d, host) = files[0]
        items = d['items'].split(',')
        vals = floats(d['item_values'])
        ax = plt.gca()
        ax.grid(True, which='major', linestyle='dotted', axis='y')
        bars = ax.bar(range(len(items)), vals, color='tab:blue', label=d.get('item_unit', ''))
        ax.bar_label(bars, fmt='%.3g', fontsize=8)
        ax.set_xticks(range(len(items)))
        ax.set_xticklabels(items, rotation=45 if len(items) > 8 else 0, ha='right' if len(items) > 8 else 'center')
        ax.set_ylabel(d.get('item_unit', ''))
        if 'item2_values' in d:
            ax2 = ax.twinx()
            ax2.plot(range(len(items)), floats(d['item2_values']), 'o', color='tab:red',
                     label=d.get('item2_unit', ''))
            ax2.set_ylabel(d.get('item2_unit', ''), color='tab:red')
            lo, hi = ax2.get_ylim()
            ax2.set_ylim(0, max(hi, 1.0))
        total = '%s: %.4g %s' % (d.get('total_label', 'Total'), float(d.get('total', 0)),
                                 d.get('total_unit', ''))
        plt.title(host_title(host, d['name']) + '\n' + total + '   ' +
                  bench_label(path, d, host).split('\n')[1], fontsize=10, weight='bold')
        return
    # Compare the main result of each run
    labels = [bench_label(p, d, h) for p, (d, h) in files]
    vals = [float(d.get('total', 0)) for _, (d, _) in files]
    d0, host0 = files[0][1]
    bars = plt.bar(range(len(vals)), vals, color='tab:blue')
    plt.gca().bar_label(bars, fmt='%.4g', fontsize=8)
    plt.xticks(range(len(vals)), labels, fontsize=7)
    plt.ylabel('%s (%s)' % (d0.get('total_label', 'Total'), d0.get('total_unit', '')))
    plt.title('%s comparison' % host0.get('title', d0['name']), fontsize=11, weight='bold')


def read_sar(path, cpu):
    rows = {}
    with open(path, newline='') as f:
        for r in csv.DictReader(f):
            if r['cpu'] != cpu:
                continue
            try:
                t = datetime.datetime.strptime(r['time'], '%Y-%m-%d %H:%M:%S')
            except ValueError:
                continue
            rows.setdefault(r['host'], []).append((t, r))
    if not rows:
        raise PlotError('no rows for cpu "%s" in %s (use sar_cpu_stats --csv --all-cpus '
                        'for one CPU)' % (cpu, path))
    return rows


def num(r, key):
    try:
        return float(r.get(key) or 0)
    except ValueError:
        return 0.0


def plot_sar(plt, files, cpu):
    hosts = {}
    for path, _ in files:
        for host, rows in read_sar(path, cpu).items():
            hosts.setdefault(host, []).extend(rows)
    fig = plt.gcf()
    axes = fig.subplots(len(hosts), 1, squeeze=False)[:, 0]
    fig.set_size_inches(11, 3.6 * len(hosts) + 0.8)
    for ax, (host, rows) in zip(axes, sorted(hosts.items())):
        rows.sort(key=lambda x: x[0])
        good = [(t, r) for t, r in rows if r.get('sample') != 'bad']
        bad = [t for t, r in rows if r.get('sample') == 'bad']
        times = [t for t, _ in good]
        series = []
        for key, label, color in SAR_STACK:
            if key == 'irqsoft':
                vals = [num(r, 'irq') + num(r, 'soft') for _, r in good]
            else:
                vals = [num(r, key) for _, r in good]
            series.append((vals, label, color))
        # Break the plot at gaps so a missing day is not drawn as a ramp
        steps = sorted((b - a).total_seconds() for a, b in zip(times, times[1:]))
        step = steps[len(steps) // 2] if steps else 600
        seg_start = 0
        segments = []
        for i in range(1, len(times) + 1):
            if i == len(times) or (times[i] - times[i - 1]).total_seconds() > 2 * step + 60:
                segments.append((seg_start, i))
                seg_start = i
        for s, e in segments:
            ax.stackplot(times[s:e], *[v[s:e] for v, _, _ in series],
                         colors=[c for _, _, c in series],
                         labels=[l if s == 0 else '_nolegend_' for _, l, _ in series],
                         alpha=0.85)
        for t in bad:
            ax.axvline(t, color='red', linewidth=1.2, linestyle='--')
        if bad:
            ax.plot([], [], color='red', linestyle='--', label='bad sample')
        ax.set_ylim(0, 100)
        ax.set_ylabel('CPU %% (cpu %s)' % cpu)
        ax.grid(True, linestyle='dotted')
        ax.set_title('%s: %d samples, %d bad, %s to %s' % (
            host, len(good), len(bad), rows[0][0].strftime('%Y-%m-%d'),
            rows[-1][0].strftime('%Y-%m-%d')), fontsize=10, weight='bold')
        ax.legend(loc='upper left', fontsize=7, ncol=7)
    fig.autofmt_xdate()


def plot_mem(plt, files):
    for path, _ in files:
        times, vals = [], []
        with open(path) as f:
            for line in f:
                p = line.split()
                if len(p) != 3 or not p[2].isdigit():
                    continue
                try:
                    times.append(datetime.datetime.strptime(p[0] + ' ' + p[1], '%Y-%m-%d %H:%M:%S'))
                except ValueError:
                    continue
                vals.append(int(p[2]) / 1048576.0)
        plt.plot(times, vals, label=os.path.basename(path))
    plt.ylabel('Used memory (GB)')
    plt.grid(True, linestyle='dotted')
    plt.legend(fontsize=8)
    plt.title('Memory use', fontsize=11, weight='bold')
    plt.gcf().autofmt_xdate()


def load(paths):
    """Read files, all of one kind. Returns (kind, [(path, data)])."""
    files = [(p,) + read_kind(p) for p in paths]
    kinds = set(k for _, k, _ in files)
    if len(kinds) > 1:
        raise PlotError('cannot plot different kinds of files together: ' +
                        ', '.join(sorted(kinds)))
    return kinds.pop(), [(p, data) for p, _, data in files]


def make_figure(plt, kind, pairs, averages=False, cpu='all'):
    """Draw one figure for files of one kind. Returns the figure."""
    plt.figure(figsize=(9, 5.5))
    if kind == 'disk':
        plot_disk(plt, pairs, averages)
    elif kind == 'bench':
        plot_bench(plt, pairs)
    elif kind == 'sar':
        plt.clf()
        plot_sar(plt, pairs, cpu)
    else:
        plot_mem(plt, pairs)
    if kind in ('disk', 'mem') or (kind == 'bench' and len(pairs) > 1):
        plt.grid(True, which='major', linestyle='dotted', axis='y')
    plt.tight_layout()
    return plt.gcf()


def main(argv=None):
    ap = argparse.ArgumentParser(description='Plot dr_utils benchmark and system stats files')
    ap.add_argument('files', nargs='+')
    ap.add_argument('-o', '--out', help='write this PNG file instead of showing the plot')
    ap.add_argument('--averages', action='store_true',
                    help='disktest: milliseconds per key instead of seconds per 10%% of the keys')
    ap.add_argument('--cpu', default='all', help='sar CSV: the CPU to plot (default all)')
    ap.add_argument('--dpi', type=int, default=100)
    args = ap.parse_args(argv)

    try:
        kind, pairs = load(args.files)
        import matplotlib
        if args.out:
            matplotlib.use('Agg')
        import matplotlib.pyplot as plt
        make_figure(plt, kind, pairs, args.averages, args.cpu)
        if args.out:
            plt.savefig(args.out, dpi=args.dpi, bbox_inches='tight', pad_inches=0.1)
            print('Plot file = ' + args.out)
        else:
            plt.show()
    except PlotError as e:
        print('ERROR - ' + str(e))
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
