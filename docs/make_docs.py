#!/usr/bin/env python3
# -----------------------------------------------------------
# Python Script: make_docs.py
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
# Builds the documentation from the markdown pages in docs/ and
# hooks/README.md:
# - docs/html/: one HTML page each, a side menu, and a search box that
#   works offline (the search index is a script file, so the pages also
#   work opened straight from disk, file://);
# - a README.md in each program folder (tools/<name>, benchmarks/<name>,
#   benchmarks/plot, benchmarks/scripts, system_stats/sar_cpu_stats),
#   with that program's section of the docs.
#
#   python3 docs/make_docs.py            write docs/html/ and the folder READMEs
#   python3 docs/make_docs.py --check    exit 1 if any of them is out of date
#
# Needs Python 3 with the markdown package (python3-markdown, or
# pip install markdown). Edit the markdown, then run this script and
# commit both.
#
# Changes:
# 11/19/2026: First version.
# -----------------------------------------------------------

import argparse
import html
import json
import os
import re
import sys

import markdown

HERE = os.path.dirname(os.path.abspath(__file__))
TOP = os.path.dirname(HERE)
OUT = os.path.join(HERE, 'html')

# (source relative to TOP, output name, menu title), in menu order
PAGES = [
    ('docs/README.md', 'index.html', 'Overview'),
    ('docs/install.md', 'install.html', 'Building and installing'),
    ('docs/file-tools.md', 'file-tools.html', 'File tools'),
    ('docs/network-tools.md', 'network-tools.html', 'Network tools'),
    ('docs/benchmarks.md', 'benchmarks.html', 'Benchmarks'),
    ('docs/sar-stats.md', 'sar-stats.html', 'sar reports (every section)'),
    ('docs/sar-cpu-stats.md', 'sar-cpu-stats.html', 'sar CPU statistics'),
    ('docs/plots-and-reports.md', 'plots-and-reports.html',
     'Plots, reports, watch scripts'),
    ('docs/dashboard-hooks.md', 'dashboard-hooks.html', 'Dashboard hooks'),
    ('hooks/README.md', 'hooks.html', 'Hook files reference'),
    ('docs/quick-reference.md', 'quick-reference.html', 'Quick reference'),
]

CSS = r"""
:root{--bg:#ffffff;--fg:#1d2329;--muted:#5b6670;--line:#d9dee3;--side:#f4f6f8;
--code:#f1f3f5;--accent:#0b62a8;--mark:#ffe58a;--hit:#e8f1fa}
@media (prefers-color-scheme: dark){:root:not([data-theme="light"]){
--bg:#14181c;--fg:#dde3e8;--muted:#98a3ad;--line:#2c343b;--side:#1b2026;
--code:#1f262d;--accent:#6cb4f0;--mark:#6b5a12;--hit:#1e2e3d}}
:root[data-theme="dark"]{--bg:#14181c;--fg:#dde3e8;--muted:#98a3ad;--line:#2c343b;
--side:#1b2026;--code:#1f262d;--accent:#6cb4f0;--mark:#6b5a12;--hit:#1e2e3d}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--fg);
font:16px/1.55 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif}
a{color:var(--accent)}
.wrap{display:flex;min-height:100vh}
nav{width:270px;flex:none;background:var(--side);border-right:1px solid var(--line);
padding:16px;position:sticky;top:0;height:100vh;overflow-y:auto}
nav .title{font-weight:700;font-size:18px;margin:0 0 12px}
nav .title a{color:var(--fg);text-decoration:none}
nav ul{list-style:none;padding:0;margin:12px 0 0}
nav li a{display:block;padding:5px 8px;border-radius:6px;color:var(--fg);text-decoration:none}
nav li a:hover{background:var(--hit)}
nav li a.cur{background:var(--hit);font-weight:600}
#q{width:100%;padding:8px 10px;border:1px solid var(--line);border-radius:6px;
background:var(--bg);color:var(--fg);font-size:15px}
main{flex:1;min-width:0;padding:24px 40px 64px;max-width:1000px}
#results{display:none}
#results .r{padding:10px 12px;border:1px solid var(--line);border-radius:8px;margin:0 0 10px}
#results .r a{font-weight:600}
#results .where{color:var(--muted);font-size:13px}
#results .snip{font-size:14px;margin-top:4px}
mark{background:var(--mark);color:inherit}
h1{font-size:30px;margin-top:0}
h2{border-bottom:1px solid var(--line);padding-bottom:4px;margin-top:36px}
code{background:var(--code);padding:1px 5px;border-radius:4px;
font:14px/1.45 ui-monospace,SFMono-Regular,Consolas,"Liberation Mono",monospace}
pre{background:var(--code);padding:12px 14px;border-radius:6px;overflow-x:auto}
pre code{padding:0;background:none}
table{border-collapse:collapse;margin:12px 0;display:block;overflow-x:auto}
th,td{border:1px solid var(--line);padding:6px 10px;text-align:left;vertical-align:top}
th{background:var(--side)}
.foot{color:var(--muted);font-size:13px;margin-top:48px;border-top:1px solid var(--line);padding-top:8px}
.theme{margin-top:16px;font-size:13px}
.theme button{background:none;border:1px solid var(--line);color:var(--fg);
border-radius:6px;padding:3px 8px;cursor:pointer}
#menu-btn{display:none}
@media (max-width:820px){
.wrap{display:block}
nav{width:auto;height:auto;position:static;border-right:0;border-bottom:1px solid var(--line)}
nav ul{display:none}nav.open ul{display:block}
#menu-btn{display:inline-block;margin-top:8px;background:none;border:1px solid var(--line);
color:var(--fg);border-radius:6px;padding:4px 10px}
main{padding:16px}}
"""

JS = r"""
(function(){
var q=document.getElementById('q'),res=document.getElementById('results'),
 body=document.getElementById('content');
function esc(s){return s.replace(/[&<>"]/g,function(c){return{'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c];});}
function hl(s,terms){s=esc(s);terms.forEach(function(t){if(!t)return;
 var re=new RegExp('('+t.replace(/[.*+?^${}()|[\]\\]/g,'\\$&')+')','gi');
 s=s.replace(re,'<mark>$1</mark>');});return s;}
function snippet(text,terms){var low=text.toLowerCase(),at=-1;
 for(var i=0;i<terms.length;i++){at=low.indexOf(terms[i]);if(at>=0)break;}
 var a=Math.max(0,at-80),b=Math.min(text.length,(at<0?0:at)+180);
 return (a>0?'... ':'')+text.slice(a,b)+(b<text.length?' ...':'');}
function search(v){var terms=v.toLowerCase().split(/\s+/).filter(Boolean);
 if(!terms.length){res.style.display='none';body.style.display='';return;}
 var hits=[];DRU_INDEX.forEach(function(s){var t=(s.h+' '+s.t).toLowerCase(),
  score=0;for(var i=0;i<terms.length;i++){var n=t.split(terms[i]).length-1;
  if(!n)return;score+=n+(s.h.toLowerCase().indexOf(terms[i])>=0?20:0);}
  hits.push([score,s]);});
 hits.sort(function(a,b){return b[0]-a[0];});
 var out='<h1>Search: '+esc(v)+'</h1><p>'+hits.length+' section(s) found.</p>';
 hits.slice(0,60).forEach(function(h){var s=h[1];
  out+='<div class="r"><a href="'+s.u+'">'+hl(s.h,terms)+'</a><div class="where">'+
  esc(s.p)+'</div><div class="snip">'+hl(snippet(s.t,terms),terms)+'</div></div>';});
 res.innerHTML=out;res.style.display='block';body.style.display='none';}
var timer;q.addEventListener('input',function(){clearTimeout(timer);
 timer=setTimeout(function(){search(q.value.trim());
 try{history.replaceState(null,'',q.value.trim()?'?q='+encodeURIComponent(q.value.trim()):location.pathname+location.hash);}catch(e){}},120);});
q.addEventListener('keydown',function(e){if(e.key==='Escape'){q.value='';search('');}});
document.addEventListener('keydown',function(e){if(e.key==='/'&&document.activeElement!==q){e.preventDefault();q.focus();}});
var m=/[?&]q=([^&]*)/.exec(location.search);if(m){q.value=decodeURIComponent(m[1].replace(/\+/g,' '));search(q.value);}
var mb=document.getElementById('menu-btn');mb.addEventListener('click',function(){
 document.querySelector('nav').classList.toggle('open');});
var root=document.documentElement;
try{var th=localStorage.getItem('dru-theme');if(th)root.setAttribute('data-theme',th);}catch(e){}
document.getElementById('theme-btn').addEventListener('click',function(){
 var dark=root.getAttribute('data-theme')==='dark'||(!root.getAttribute('data-theme')&&
 matchMedia('(prefers-color-scheme: dark)').matches);var n=dark?'light':'dark';
 root.setAttribute('data-theme',n);try{localStorage.setItem('dru-theme',n);}catch(e){}});
})();
"""

TEMPLATE = """<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{title} - dr_utils {version}</title>
<link rel="stylesheet" href="dr_utils.css">
</head>
<body>
<div class="wrap">
<nav>
<p class="title"><a href="index.html">dr_utils {version}</a></p>
<input id="q" type="search" placeholder="Search the docs ( / )" aria-label="Search the docs">
<button id="menu-btn" type="button">Pages</button>
<ul>
{menu}
</ul>
<div class="theme"><button id="theme-btn" type="button">Light / dark</button></div>
</nav>
<main>
<div id="results"></div>
<div id="content">
{body}
<p class="foot">dr_utils {version} documentation, GNU LGPL 3 or later. Made from {source} by docs/make_docs.py.</p>
</div>
</main>
</div>
<script src="search_index.js"></script>
<script src="dr_utils.js"></script>
</body>
</html>
"""


def slugify(value, separator):
    # GitHub style anchors, so the links in the markdown work in both
    value = value.strip().lower()
    value = re.sub(r'[^\w\s-]', '', value)
    return re.sub(r'\s', separator, value)


def fix_links(text, src):
    # page.md#anchor -> page.html#anchor for the pages built here
    names = {}
    for s, o, _ in PAGES:
        names[os.path.normpath(os.path.join(TOP, s))] = o

    def repl(m):
        target, anchor = m.group(1), m.group(2) or ''
        if re.match(r'[a-z]+:', target):
            return m.group(0)
        full = os.path.normpath(os.path.join(TOP, os.path.dirname(src), target))
        if full in names:
            return '](' + names[full] + anchor + ')'
        # Other repo files: link to the file in the source tree
        rel = os.path.relpath(full, OUT).replace(os.sep, '/')
        return '](' + rel + anchor + ')'
    return re.sub(r'\]\(([^)#\s]+\.md|[^)#\s]+/)(#[^)\s]*)?\)', repl, text)


def sections(body_html, page_title, page_url):
    # Split a page at h1/h2/h3 for the search index
    parts = re.split(r'(<h[123] id="[^"]*">.*?</h[123]>)', body_html)
    index = []
    head, url = page_title, page_url
    for p in parts:
        m = re.match(r'<h[123] id="([^"]*)">(.*?)</h[123]>', p)
        if m:
            url = page_url + '#' + m.group(1)
            head = html.unescape(re.sub(r'<[^>]+>', '', m.group(2)))
            continue
        text = html.unescape(re.sub(r'<[^>]+>', ' ', p))
        text = re.sub(r'\s+', ' ', text).strip()
        if text or head:
            index.append({'h': head, 'p': page_title, 'u': url, 't': text})
    return index


# ---- README.md in each program folder ---------------------------------
# (folder, title, [(docs page, "## heading" of the section to copy)])
FOLDERS = [
    ('tools/file_hours', 'file_hours', [('file-tools.md', 'file_hours')]),
    ('tools/file_purge', 'file_purge', [('file-tools.md', 'file_purge')]),
    ('tools/filetimes', 'filetimes', [('file-tools.md', 'filetimes')]),
    ('tools/internet-test', 'internet-test',
     [('network-tools.md', 'internet-test')]),
    ('tools/pingscan', 'pingscan', [('network-tools.md', 'pingscan')]),
    ('tools/dnsscan', 'dnsscan', [('network-tools.md', 'dnsscan')]),
    ('tools/portscan', 'portscan', [('network-tools.md', 'portscan')]),
    ('tools/netpscan', 'netpscan', [('network-tools.md', 'netpscan')]),
    ('tools/arpscan', 'arpscan', [('network-tools.md', 'arpscan')]),
    ('tools/getmac', 'getmac', [('network-tools.md', 'getmac')]),
    ('benchmarks/cputest', 'cputest', [('benchmarks.md', 'cputest')]),
    ('benchmarks/flops', 'flops', [('benchmarks.md', 'flops')]),
    ('benchmarks/memtest', 'memtest', [('benchmarks.md', 'memtest')]),
    ('benchmarks/disktest', 'disktest', [('benchmarks.md', 'disktest')]),
    ('benchmarks/tcpserv', 'tcpserv', [('benchmarks.md', 'tcpserv and tcptest')]),
    ('benchmarks/tcptest', 'tcptest', [('benchmarks.md', 'tcpserv and tcptest')]),
    ('benchmarks/udpserv', 'udpserv', [('benchmarks.md', 'udpserv and udptest')]),
    ('benchmarks/udptest', 'udptest', [('benchmarks.md', 'udpserv and udptest')]),
    ('benchmarks/mcserv', 'mcserv', [('benchmarks.md', 'mcserv and mctest')]),
    ('benchmarks/mctest', 'mctest', [('benchmarks.md', 'mcserv and mctest')]),
    ('benchmarks/plot', 'dr_plot.py and dr_report.py',
     [('plots-and-reports.md', 'Plot data files (`--datfile`)'),
      ('plots-and-reports.md', 'dr_plot.py'),
      ('plots-and-reports.md', 'dr_report.py')]),
    ('benchmarks/scripts', 'Watch scripts',
     [('plots-and-reports.md', 'Watch scripts (Linux)')]),
    ('system_stats/sar_cpu_stats', 'sar_cpu_stats',
     [('sar-cpu-stats.md', None)]),
    ('system_stats/sar_stats', 'sar_stats',
     [('sar-stats.md', None)]),
]

# Notes that a copied section needs from the top of its page
PAGE_NOTES = {
    'network-tools.md': 'Ranges (`192.0.2`, `192.0.2.7`, `192.0.2.10-40`), '
    'services files and the privileges ping needs are on the '
    '[Network tools](NETWORK) page.',
    'benchmarks.md': 'Sizes take decimal suffixes (`1k` = 1000, `1m`, `1g`). '
    'The dashboard options (`--drsm`, `--submit`, `--now`, `--drra`, '
    '`--drra-config`) are on the [Dashboard](DASH) page.',
}


def md_section(text, heading):
    # The "## heading" section of a page, without the heading line
    if heading is None:
        return re.sub(r'\A# [^\n]*\n', '', text)
    m = re.search(r'^## ' + re.escape(heading) + r'\n(.*?)(?=^## |\Z)', text,
                  re.M | re.S)
    if not m:
        raise SystemExit('make_docs.py: no "## %s" section' % heading)
    return m.group(1)


def relink(text, folder):
    # Links in docs/ pages point next to them: make them work from folder
    def repl(m):
        target, anchor = m.group(1), m.group(2) or ''
        if re.match(r'[a-z]+:', target):
            return m.group(0)
        full = os.path.normpath(os.path.join(HERE, target))
        rel = os.path.relpath(full, os.path.join(TOP, folder))
        return '](' + rel.replace(os.sep, '/') + anchor + ')'
    return re.sub(r'\]\(([^)#\s]+\.md)(#[^)\s]*)?\)', repl, text)


def folder_readmes(version):
    files = {}
    for folder, title, parts in FOLDERS:
        body = []
        for page, heading in parts:
            text = open(os.path.join(HERE, page), encoding='utf-8').read()
            sec = md_section(text, heading).strip('\n')
            if len(parts) > 1:
                sec = '## ' + heading + '\n\n' + sec
            else:
                sec = re.sub(r'^###', '##', sec, flags=re.M)
            body.append(sec)
        notes = []
        for page in sorted({pg for pg, _ in parts}):
            if page in PAGE_NOTES:
                notes.append(PAGE_NOTES[page].replace('(NETWORK)', '(network-tools.md)')
                             .replace('(DASH)', '(dashboard-hooks.md)'))
        page = parts[0][0]
        anchor = '#' + slugify(parts[0][1], '-') if parts[0][1] and len(parts) == 1 else ''
        head = ('# %s\n\n'
                'Part of dr_utils %s. This file is made from [docs/%s](%s%s) by '
                'docs/make_docs.py: edit that page, not this file. All the '
                'documentation: [docs/](README.md), or the searchable HTML in '
                '`docs/html/index.html`.\n' %
                (title, version, page, page, anchor))
        out = head + ''.join('\n' + n + '\n' for n in notes) + '\n' + \
            '\n\n'.join(body) + '\n'
        files[os.path.join('..', folder, 'README.md')] = relink(out, folder)
    return files


def build():
    version = open(os.path.join(TOP, 'version.txt')).read().strip()
    files = {'html/dr_utils.css': CSS.lstrip(), 'html/dr_utils.js': JS.lstrip()}
    index = []
    for src, out, title in PAGES:
        text = open(os.path.join(TOP, src), encoding='utf-8').read()
        text = fix_links(text, src)
        md = markdown.Markdown(
            extensions=['tables', 'fenced_code', 'toc', 'sane_lists'],
            extension_configs={'toc': {'slugify': slugify}})
        # GitHub tables take \| for a | inside code; python-markdown keeps the backslash
        body = md.convert(text).replace('\\|', '|')
        menu = '\n'.join(
            '<li><a href="%s"%s>%s</a></li>' %
            (o, ' class="cur"' if o == out else '', html.escape(t))
            for _, o, t in PAGES)
        files['html/' + out] = TEMPLATE.format(title=html.escape(title), version=version,
                                     menu=menu, body=body, source=src)
        index.extend(sections(body, title, out))
    files.update(folder_readmes(version))
    files['html/search_index.js'] = ('// Made by docs/make_docs.py\nvar DRU_INDEX = ' +
                                json.dumps(index, indent=0, ensure_ascii=False) +
                                ';\n')
    return files


def main():
    ap = argparse.ArgumentParser(description='Build docs/html from the markdown docs')
    ap.add_argument('--check', action='store_true',
                    help='exit 1 if docs/html or a folder README is out of date, change nothing')
    args = ap.parse_args()
    files = build()
    stale = []
    for name, data in files.items():
        path = os.path.join(HERE, name)
        old = None
        if os.path.exists(path):
            with open(path, encoding='utf-8') as f:
                old = f.read()
        if old != data:
            stale.append(name)
    if args.check:
        for name in stale:
            print('out of date: ' + os.path.relpath(os.path.join(HERE, name), TOP))
        return 1 if stale else 0
    for name in stale:
        os.makedirs(os.path.dirname(os.path.join(HERE, name)), exist_ok=True)
        tmp = os.path.join(HERE, name + '.new')
        with open(tmp, 'w', encoding='utf-8', newline='\n') as f:
            f.write(files[name])
        os.replace(tmp, os.path.join(HERE, name))
    print('docs/html and folder READMEs: %d file(s) written, %d up to date' %
          (len(stale), len(files) - len(stale)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
