#!/usr/bin/env python3
# -----------------------------------------------------------
# Python Script: dr_sar.py
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
# Charts of every sar section from sar_stats --csv output
# (host,time,section,item,metric,value), the way the old kSar tool
# drew them: one chart per section and item (the whole system: CPU
# "all", interrupts "sum", or the only item; then each disk, network
# interface, file system ...), the columns of a chart split by unit
# (percent, per second, sizes, counts) into stacked panels. Used by
# dr_plot.py and dr_report.py.
#
# Changes:
# 11/20/2026: First version.
# -----------------------------------------------------------

import csv
import datetime

HEADER = 'host,time,section,item,metric,value'

# Section id -> (title, sar option), the same table as sar_stats
SECTIONS = {
    'cpu': ('CPU utilization', '-u, -P ALL'),
    'pcsw': ('Task creation and context switches', '-w'),
    'intr': ('Interrupts', '-I'),
    'intr_cpu': ('Interrupts per CPU', '-I ALL -P ALL'),
    'swap': ('Swapping', '-W'),
    'paging': ('Paging', '-B'),
    'io': ('I/O and transfer rates', '-b'),
    'memstat': ('Memory page changes', '-R'),
    'mem': ('Memory', '-r'),
    'swpspace': ('Swap space', '-S'),
    'huge': ('Huge pages', '-H'),
    'ktables': ('Kernel tables', '-v'),
    'queue': ('Run queue and load average', '-q'),
    'psi_cpu': ('CPU pressure (PSI)', '-q CPU'),
    'psi_io': ('I/O pressure (PSI)', '-q IO'),
    'psi_mem': ('Memory pressure (PSI)', '-q MEM'),
    'tty': ('Serial lines', '-y'),
    'disk': ('Block devices', '-d'),
    'net_dev': ('Network interfaces', '-n DEV'),
    'net_edev': ('Network interface errors', '-n EDEV'),
    'nfs': ('NFS client', '-n NFS'),
    'nfsd': ('NFS server', '-n NFSD'),
    'sock': ('Sockets (IPv4)', '-n SOCK'),
    'ip': ('IPv4 traffic', '-n IP'),
    'eip': ('IPv4 errors', '-n EIP'),
    'icmp': ('ICMPv4 traffic', '-n ICMP'),
    'eicmp': ('ICMPv4 errors', '-n EICMP'),
    'tcp': ('TCPv4 traffic', '-n TCP'),
    'etcp': ('TCPv4 errors', '-n ETCP'),
    'udp': ('UDPv4 traffic', '-n UDP'),
    'sock6': ('Sockets (IPv6)', '-n SOCK6'),
    'ip6': ('IPv6 traffic', '-n IP6'),
    'eip6': ('IPv6 errors', '-n EIP6'),
    'icmp6': ('ICMPv6 traffic', '-n ICMP6'),
    'eicmp6': ('ICMPv6 errors', '-n EICMP6'),
    'udp6': ('UDPv6 traffic', '-n UDP6'),
    'softnet': ('Software network processing', '-n SOFT'),
    'fchost': ('Fibre Channel hosts', '-n FC'),
    'cpufreq': ('CPU frequency', '-m CPU'),
    'cpuwfreq': ('CPU weighted frequency', '-m FREQ'),
    'fan': ('Fans', '-m FAN'),
    'temp': ('Temperatures', '-m TEMP'),
    'in': ('Voltage inputs', '-m IN'),
    'usb': ('USB devices', '-m USB'),
    'fs': ('File systems', '-F'),
}

# Sections whose items are many and alike: only the whole-system item
# is drawn unless items are asked for
PER_CPU = ('cpu', 'intr_cpu', 'softnet', 'cpufreq', 'cpuwfreq')


def title(sec):
    t = SECTIONS.get(sec)
    if t:
        return '%s (sar %s)' % t
    return sec.replace('other_', '') + ' (sar section)'


def is_sar_all(head):
    return head.startswith(HEADER)


def unit(metric):
    """The panel of a column: percent, per second, size, load or other"""
    m = metric.lower()
    if m.startswith('%'):
        return 'percent'
    if m.endswith('/s'):
        return 'per second'
    if m.startswith('kb') or m.startswith('mb'):
        return 'size (kB, MB)'
    if m.startswith('ldavg'):
        return 'load average'
    return 'count'


def read(paths, sections=None, items=None):
    """{host: {section: {item: {metric: ([times], [values])}}}}"""
    data = {}
    for path in paths:
        with open(path, newline='', errors='replace') as f:
            rd = csv.reader(f)
            next(rd, None)
            for r in rd:
                if len(r) != 6:
                    continue
                host, t, sec, item, metric, value = r
                if sections and sec not in sections:
                    continue
                if items and item not in items:
                    continue
                try:
                    v = float(value)
                    tt = datetime.datetime.strptime(t, '%Y-%m-%d %H:%M:%S')
                except ValueError:
                    continue
                s = data.setdefault(host, {}).setdefault(sec, {}).setdefault(item, {})
                ts, vs = s.setdefault(metric, ([], []))
                ts.append(tt)
                vs.append(v)
    return data


def main_item(sec_data):
    for k in ('all', 'sum'):
        if k in sec_data:
            return k
    if len(sec_data) == 1:
        return next(iter(sec_data))
    return None


def activity(item_data):
    """How busy an item is: the sum of its averages (0 = idle)"""
    tot = 0.0
    for ts, vs in item_data.values():
        if vs:
            tot += abs(sum(vs) / len(vs))
    return tot


def chart_items(sec, sec_data, max_items=6, items_asked=False):
    """The items to draw for a section, whole system first, then the
    busiest items (idle ones are left out)"""
    main = main_item(sec_data)
    out = [main] if main is not None else []
    if items_asked:
        return out + [i for i in sec_data if i != main]
    if sec in PER_CPU and main is not None:
        return out
    rest = sorted((i for i in sec_data if i != main), key=lambda i: -activity(sec_data[i]))
    rest = [i for i in rest if activity(sec_data[i]) > 0]
    return out + rest[:max_items]


def stats(ts, vs):
    i_max = max(range(len(vs)), key=lambda i: vs[i])
    return sum(vs) / len(vs), min(vs), vs[i_max], ts[i_max]


def figures(plt, data, max_items=6, items_asked=False, order=None):
    """Yields (host, section, item, figure), one per chart"""
    for host in sorted(data):
        secs = data[host]
        ids = [s for s in (order or []) if s in secs] + [s for s in secs if s not in (order or [])]
        for sec in ids:
            for item in chart_items(sec, secs[sec], max_items, items_asked):
                yield host, sec, item, figure(plt, host, sec, item, secs[sec][item])


CPU_STACK = ['%user', '%usr', '%nice', '%system', '%sys', '%iowait', '%steal', '%irq', '%soft']


def stack_cpu(ax, item_data, metrics):
    """CPU use stacked like kSar: user, nice, system, iowait, steal, irq,
    soft (guest time is inside user and nice; idle is the rest)"""
    names = [m for m in CPU_STACK if m in item_data]
    base = item_data[names[0]][0]
    index = {t: i for i, t in enumerate(base)}
    rows = []
    for m in names:
        vals = [0.0] * len(base)
        for t, v in zip(*item_data[m]):
            if t in index:
                vals[index[t]] = v
        rows.append(vals)
    order = sorted(range(len(base)), key=lambda i: base[i])
    xs = [base[i] for i in order]
    ax.stackplot(xs, *[[r[i] for i in order] for r in rows], labels=names, alpha=0.85)
    ax.set_ylim(0, 100)
    ax.set_ylabel('percent (busy)', fontsize=8)
    ax.grid(True, linestyle='dotted')
    ax.legend(loc='upper left', fontsize=7, ncol=min(7, len(names)), framealpha=0.85)
    ax.tick_params(labelsize=7)


def figure(plt, host, sec, item, item_data):
    panels = {}
    for metric in item_data:
        panels.setdefault(unit(metric), []).append(metric)
    names = sorted(panels, key=lambda u: ['percent', 'per second', 'size (kB, MB)',
                                          'load average', 'count'].index(u))
    fig, axes = plt.subplots(len(names), 1, sharex=True, squeeze=False,
                             figsize=(11, 2.6 * len(names) + 1.0))
    for ax, u in zip(axes[:, 0], names):
        if sec == 'cpu' and u == 'percent':
            stack_cpu(ax, item_data, panels[u])
            continue
        for metric in panels[u]:
            ts, vs = item_data[metric]
            pairs = sorted(zip(ts, vs))
            # Break the line at gaps so a missing day is not a ramp
            steps = sorted((b[0] - a[0]).total_seconds() for a, b in zip(pairs, pairs[1:]))
            step = steps[len(steps) // 2] if steps else 600
            xs, ys = [], []
            for i, (t, v) in enumerate(pairs):
                if i and (t - pairs[i - 1][0]).total_seconds() > 2 * step + 60:
                    xs.append(t)
                    ys.append(float('nan'))
                xs.append(t)
                ys.append(v)
            ax.plot(xs, ys, linewidth=1, label=metric)
        ax.set_ylabel(u, fontsize=8)
        ax.grid(True, linestyle='dotted')
        ax.legend(loc='upper left', fontsize=7, ncol=min(6, len(panels[u])), framealpha=0.85)
        ax.tick_params(labelsize=7)
        if u == 'percent':
            top = max(max(item_data[m][1]) for m in panels[u])
            ax.set_ylim(0, max(100, top * 1.05) if top > 50 else max(1, top * 1.15))
    first = min(min(item_data[m][0]) for m in item_data)
    last = max(max(item_data[m][0]) for m in item_data)
    label = '' if item == '' else ', %s' % item
    fig.suptitle('%s: %s%s, %s to %s' % (host, title(sec), label, first.strftime('%Y-%m-%d %H:%M'),
                                         last.strftime('%Y-%m-%d %H:%M')),
                 fontsize=10, weight='bold')
    fig.autofmt_xdate()
    fig.tight_layout()
    return fig
