#!/usr/bin/env python3
# -----------------------------------------------------------
# Python Script File: dr_report.py
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
# Build one HTML report from dr_utils results: benchmark --datfile
# files, sar_cpu_stats --csv files and watch_mem_usage.sh logs. The
# report has a summary table of every run, then a section for each
# benchmark (a comparison chart when there are several runs, and a
# chart for each run), the CPU use of each sar file with its averages
# and peaks, and the memory logs. The charts are inside the HTML file
# (PNG images), so the one file can be mailed, archived or printed.
#
#   dr_report.py -o report.html results/            every result file
#   dr_report.py -o report.html --title "host1 sizing" a.dat b.dat x.csv
#   dr_report.py -o report.html --csv summary.csv results/
#   dr_report.py -o report.html --submit --now results/
#
# --submit hands the HTML file to the DataReel Reporting Agent (drra
# submit --hook dru_report, or drra_svc.exe on Windows), which sends it
# to the dashboard; DRSM shows it with its html display (hook files in
# dr_utils hooks/).
#
# A folder is read for *.dat, *.csv and *.log files; files that are not
# dr_utils results are listed as skipped. Needs Python 3 with
# matplotlib (python3-matplotlib), and dr_plot.py in the same folder.
#
# Changes:
# 11/19/2026: First version.
# 11/19/2026: --submit, --now, --drra, --drra-config for the dashboard.
# -----------------------------------------------------------
import argparse
import base64
import csv
import datetime
import html
import io
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dr_plot  # noqa: E402

SAR_FIELDS = ['user', 'nice', 'system', 'iowait', 'steal', 'irq', 'soft', 'idle']

CSS = """
:root { --bg:#ffffff; --fg:#1d232b; --muted:#5b6673; --line:#d9dee4; --head:#f2f4f7;
        --accent:#1f5f99; --warn:#a1322b; }
@media (prefers-color-scheme: dark) {
  :root { --bg:#14181d; --fg:#e3e7ec; --muted:#9aa6b2; --line:#2e3640; --head:#1c2229;
          --accent:#7db3e8; --warn:#ee8a82; }
}
* { box-sizing: border-box; }
body { margin:0; background:var(--bg); color:var(--fg);
       font:15px/1.5 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif; }
main { max-width:1100px; margin:0 auto; padding:24px 16px 48px; }
h1 { font-size:1.6rem; margin:0 0 4px; }
h2 { font-size:1.2rem; margin:32px 0 8px; padding-bottom:4px; border-bottom:2px solid var(--line); }
h3 { font-size:1rem; margin:20px 0 6px; }
.lead, .note { color:var(--muted); }
.tablewrap { overflow-x:auto; }
table { width:100%; border-collapse:collapse; font-size:.9rem; margin:8px 0; }
th, td { text-align:left; vertical-align:top; padding:6px 8px; border-bottom:1px solid var(--line); }
th { background:var(--head); font-weight:600; }
td.num { text-align:right; font-variant-numeric: tabular-nums; white-space:nowrap; }
.warn { color:var(--warn); font-weight:600; }
img { max-width:100%; height:auto; background:#fff; border:1px solid var(--line); border-radius:4px; }
nav a { color:var(--accent); margin-right:14px; }
@media print { nav { display:none; } h2 { break-before:auto; } img { break-inside:avoid; } }
"""


def esc(s):
    return html.escape(str(s))


def fig_html(plt, fig, alt):
    buf = io.BytesIO()
    fig.savefig(buf, format='png', dpi=90, bbox_inches='tight', pad_inches=0.1)
    plt.close(fig)
    data = base64.b64encode(buf.getvalue()).decode('ascii')
    return '<p><img alt="%s" src="data:image/png;base64,%s"></p>' % (esc(alt), data)


def collect(inputs):
    """Returns (files by kind {kind: [(path, data)]}, skipped [(path, why)])"""
    paths, skipped = [], []
    for p in inputs:
        if os.path.isdir(p):
            for name in sorted(os.listdir(p)):
                full = os.path.join(p, name)
                if os.path.isfile(full) and name.endswith(('.dat', '.csv', '.log')):
                    paths.append((full, False))
        else:
            paths.append((p, True))
    kinds = {}
    for path, explicit in paths:
        try:
            kind, data = dr_plot.read_kind(path)
        except dr_plot.PlotError as e:
            if explicit and not os.path.exists(path):
                raise
            skipped.append((path, str(e)))
            continue
        kinds.setdefault(kind, []).append((path, data))
    return kinds, skipped


def settings_text(d):
    keys = dr_plot.SETTINGS.get(d.get('name', ''), [])
    parts = []
    for k in keys:
        if k in d:
            v = dr_plot.shn(d[k]) if k in ('buf_size', 'num_inserts') else d[k]
            parts.append('%s %s' % (k.replace('_', ' '), v))
    return ', '.join(parts)


def summary_rows(kinds):
    rows = []
    for path, (d, host) in kinds.get('bench', []):
        result = '%.4g %s' % (float(d.get('total', 0)), d.get('total_unit', ''))
        extra = ''
        if 'item2_values' in d:
            loss = dr_plot.floats(d['item2_values'])
            if loss:
                extra = ', loss up to %.2f%%' % max(loss)
        rows.append([d.get('name', ''), host.get('host', ''), host.get('date', ''),
                     host.get('build', ''), settings_text(d),
                     '%s (%s)%s' % (result, d.get('total_label', ''), extra),
                     os.path.basename(path)])
    for path, (d, host) in kinds.get('disk', []):
        n = float(d.get('num_inserts', 0)) or 1
        rate = n / float(d.get('insert_time', 0) or 1e-9)
        rows.append(['disktest', host.get('host', ''), host.get('date', ''),
                     host.get('build', ''), '%s keys' % d.get('num_inserts_str', ''),
                     'insert %.3g s, find %.3g s, delete %.3g s (%.0f inserts/s)' % (
                         float(d.get('insert_time', 0)), float(d.get('search_time', 0)),
                         float(d.get('delete_time', 0)), rate),
                     os.path.basename(path)])
    rows.sort(key=lambda r: (r[0], r[1], r[2]))
    return rows


def table(headers, rows, num_cols=()):
    out = ['<div class="tablewrap"><table><tr>']
    out += ['<th>%s</th>' % esc(h) for h in headers]
    out.append('</tr>')
    for r in rows:
        out.append('<tr>' + ''.join(
            '<td class="num">%s</td>' % esc(c) if i in num_cols else '<td>%s</td>' % esc(c)
            for i, c in enumerate(r)) + '</tr>')
    out.append('</table></div>')
    return '\n'.join(out)


def sar_stats(path):
    """Per host: samples, bad, average and peak of each field (all-CPU rows)"""
    hosts = {}
    with open(path, newline='') as f:
        for r in csv.DictReader(f):
            if r.get('cpu') != 'all':
                continue
            h = hosts.setdefault(r['host'], {'n': 0, 'bad': 0, 'sum': {}, 'max': {},
                                             'min_idle': None, 'first': r['time'],
                                             'last': r['time']})
            h['first'] = min(h['first'], r['time'])
            h['last'] = max(h['last'], r['time'])
            if r.get('sample') == 'bad':
                h['bad'] += 1
                continue
            h['n'] += 1
            for k in SAR_FIELDS:
                if r.get(k, '') == '':
                    continue
                v = float(r[k])
                h['sum'][k] = h['sum'].get(k, 0) + v
                if k == 'idle':
                    h['min_idle'] = v if h['min_idle'] is None else min(h['min_idle'], v)
                elif v > h['max'].get(k, (-1, ''))[0]:
                    h['max'][k] = (v, r['time'])
    return hosts


def build(args):
    kinds, skipped = collect(args.inputs)
    if not kinds:
        raise dr_plot.PlotError('no dr_utils result files found')
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt

    now = datetime.datetime.now().strftime('%Y-%m-%d %H:%M')
    hosts = set()
    for kind, files in kinds.items():
        for path, data in files:
            if kind in ('bench', 'disk'):
                hosts.add(data[1].get('host', ''))
    for path, _ in kinds.get('sar', []):
        hosts.update(sar_stats(path).keys())

    body = []
    nav = []
    rows = summary_rows(kinds)
    if rows:
        nav.append(('summary', 'Summary'))
        body.append('<h2 id="summary">Summary</h2>')
        body.append(table(['Benchmark', 'Host', 'Date', 'Build', 'Settings', 'Result', 'File'],
                          rows))
        if args.csv:
            with open(args.csv, 'w', newline='') as f:
                w = csv.writer(f, lineterminator='\n')
                w.writerow(['benchmark', 'host', 'date', 'build', 'settings', 'result', 'file'])
                w.writerows(rows)

    # Benchmarks, grouped by name
    groups = {}
    for path, data in kinds.get('bench', []):
        groups.setdefault(data[0].get('name', ''), []).append((path, data))
    if kinds.get('disk'):
        groups['disktest'] = kinds['disk']
    for name in sorted(groups):
        files = groups[name]
        kind = 'disk' if name == 'disktest' else 'bench'
        title = files[0][1][1].get('title', name) if files[0][1][1] else name
        anchor = 'b-' + name
        nav.append((anchor, name))
        body.append('<h2 id="%s">%s (%s)</h2>' % (anchor, esc(title), esc(name)))
        if len(files) > 1:
            body.append('<h3>Comparison of %d runs</h3>' % len(files))
            body.append(fig_html(plt, dr_plot.make_figure(plt, kind, files), name + ' comparison'))
        if not args.no_runs:
            for path, data in files:
                host = data[1]
                body.append('<h3>%s, %s, %s</h3>' % (esc(host.get('host', '')),
                                                      esc(host.get('date', '')),
                                                      esc(os.path.basename(path))))
                body.append(fig_html(plt, dr_plot.make_figure(plt, kind, [(path, data)]),
                                     name + ' ' + os.path.basename(path)))

    # sar CPU use
    if kinds.get('sar'):
        nav.append(('sar', 'CPU use (sar)'))
        body.append('<h2 id="sar">CPU use (sar)</h2>')
        for path, data in kinds['sar']:
            body.append('<h3>%s</h3>' % esc(os.path.basename(path)))
            stats = sar_stats(path)
            srows = []
            for host, h in sorted(stats.items()):
                n = h['n'] or 1
                cells = [host, h['first'][:10] + ' to ' + h['last'][:10], h['n'], h['bad']]
                for k in ('user', 'system', 'iowait', 'steal'):
                    if k in h['sum']:
                        mx = h['max'].get(k, (0, ''))
                        cells.append('%.2f / %.2f' % (h['sum'][k] / n, mx[0]))
                    else:
                        cells.append('-')
                cells.append('%.2f / %.2f' % (h['sum'].get('idle', 0) / n, h['min_idle'] or 0)
                             if h['min_idle'] is not None else '-')
                srows.append(cells)
            body.append(table(['Host', 'Days', 'Samples', 'Bad samples', 'user avg / peak',
                               'system avg / peak', 'iowait avg / peak', 'steal avg / peak',
                               'idle avg / lowest'], srows, num_cols=(2, 3, 4, 5, 6, 7, 8)))
            bad = sum(h['bad'] for h in stats.values())
            if bad:
                body.append('<p class="warn">%d bad samples (impossible values, such as a '
                            'stopped VM writing steal over 100%%): left out of the averages, '
                            'marked in red on the chart.</p>' % bad)
            body.append(fig_html(plt, dr_plot.make_figure(plt, 'sar', [(path, data)],
                                                          cpu='all'), 'CPU use ' + path))

    # Memory logs
    if kinds.get('mem'):
        nav.append(('mem', 'Memory'))
        body.append('<h2 id="mem">Memory use</h2>')
        body.append(fig_html(plt, dr_plot.make_figure(plt, 'mem', kinds['mem']), 'memory'))

    if skipped:
        body.append('<h2>Skipped files</h2>')
        body.append(table(['File', 'Why'], [(os.path.basename(p), w) for p, w in skipped]))

    title = args.title or 'Benchmark report'
    page = ['<!doctype html>', '<html lang="en"><head><meta charset="utf-8">',
            '<meta name="viewport" content="width=device-width, initial-scale=1">',
            '<title>%s</title>' % esc(title), '<style>%s</style></head><body><main>' % CSS,
            '<h1>%s</h1>' % esc(title),
            '<p class="lead">%d result files from %s. Made %s by dr_utils dr_report.py.</p>' % (
                sum(len(v) for v in kinds.values()),
                esc(', '.join(sorted(h for h in hosts if h)) or 'unknown hosts'), now),
            '<nav>' + ''.join('<a href="#%s">%s</a>' % (a, esc(t)) for a, t in nav) + '</nav>']
    page += body
    page.append('<p class="note">Rates use decimal units (1 MB = 1,000,000 bytes). Compare '
                'machines with the same build of each benchmark.</p>')
    page.append('</main></body></html>')
    with open(args.out, 'w') as f:
        f.write('\n'.join(page) + '\n')
    return kinds, skipped


def default_drra():
    """The agent program: drra on Linux, drra_svc.exe on Windows"""
    if os.name == 'nt':
        pf = os.environ.get('ProgramFiles', r'C:\Program Files')
        return os.path.join(pf, 'DataReel', 'ReportingAgent', 'drra_svc.exe')
    for d in os.environ.get('PATH', '').split(os.pathsep):
        if d and os.access(os.path.join(d, 'drra'), os.X_OK):
            return os.path.join(d, 'drra')
    return '/usr/sbin/drra'


def submit(args):
    """Hand the report to the reporting agent. Returns True if it took it."""
    cmd = [args.drra or default_drra()]
    if args.drra_config:
        cmd += ['-c', args.drra_config]
    cmd += ['submit', '--hook', args.submit]
    if args.now:
        cmd.append('--now')
    cmd.append(args.out)
    try:
        rc = subprocess.call(cmd)
    except OSError as e:
        print('ERROR - could not run %s: %s' % (cmd[0], e))
        return False
    if rc != 0:
        print('ERROR - agent submit failed for hook %s (exit %d)' % (args.submit, rc))
        return False
    print('Submitted to the reporting agent, hook %s%s' % (args.submit, ' (send now)' if args.now else ''))
    return True


def main(argv=None):
    ap = argparse.ArgumentParser(description='Build an HTML report from dr_utils results')
    ap.add_argument('inputs', nargs='+', help='result files or folders')
    ap.add_argument('-o', '--out', required=True, help='the HTML file to write')
    ap.add_argument('--title', help='report title (default "Benchmark report")')
    ap.add_argument('--csv', help='also write the summary table as CSV')
    ap.add_argument('--no-runs', action='store_true',
                    help='only the comparison charts, not one chart per run')
    ap.add_argument('--submit', nargs='?', const='dru_report', metavar='ID',
                    help='hand the report to the reporting agent, hook ID (default dru_report)')
    ap.add_argument('--now', action='store_true', help='with --submit, the agent sends within seconds')
    ap.add_argument('--drra', help='agent program (default drra, or drra_svc.exe on Windows)')
    ap.add_argument('--drra-config', help='agent settings file, drra -c INI (Linux, for tests)')
    args = ap.parse_args(argv)
    if args.submit is not None and not re.match(r'^[a-z0-9_]{1,32}$', args.submit):
        ap.error('--submit ID: the hook ID is a-z, 0-9 and _, up to 32 characters')
    if args.now and args.submit is None:
        ap.error('--now needs --submit')
    try:
        kinds, skipped = build(args)
    except dr_plot.PlotError as e:
        print('ERROR - ' + str(e))
        return 1
    n = sum(len(v) for v in kinds.values())
    print('Report file = %s (%d result files, %d skipped)' % (args.out, n, len(skipped)))
    if args.submit is not None and not submit(args):
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
