#!/usr/bin/env python3
# -----------------------------------------------------------
# Python Script: make_html.py
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
# Builds the HTML documentation in docs/html/ and the plain text
# documentation in docs/txt/ from the markdown pages in docs/ and
# hooks/README.md. HTML: one page each, a side menu, and a search box
# that works offline (the search index is a script file, so the pages
# also work opened straight from disk, file://). Text: one .txt file
# each, 78 columns, for a terminal, less, Notepad or a printer.
#
#   python3 docs/make_html.py            write docs/html/ and docs/txt/
#   python3 docs/make_html.py --check    exit 1 if either is out of date
#
# Needs Python 3 with the markdown package (python3-markdown, or
# pip install markdown). Edit the markdown, then run this script and
# commit both.
#
# Changes:
# 11/19/2026: First version. Plain text pages in docs/txt/.
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
<p class="foot">dr_utils {version} documentation, GNU LGPL 3 or later. Made from {source} by docs/make_html.py.</p>
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


# ---- Plain text pages -------------------------------------------------
WIDTH = 78


def inline_text(s):
    # Markdown inline markup to plain text
    s = re.sub(r'\[([^\]]+)\]\(([^)]+)\)', link_text, s)
    s = s.replace('\\|', '|')
    s = re.sub(r'\*\*([^*]+)\*\*', r'\1', s)
    s = re.sub(r'`([^`]+)`', r'\1', s)
    return s


def link_text(m):
    label, target = m.group(1), m.group(2)
    if re.match(r'[a-z]+:', target):
        return '%s <%s>' % (label, target)
    page = re.sub(r'#.*$', '', target)
    for s, o, _ in PAGES:
        if page and o.replace('.html', '') == os.path.basename(page).replace('.md', ''):
            return '%s [%s]' % (label, o.replace('.html', '.txt'))
        if page.endswith('hooks/README.md'):
            return '%s [hooks.txt]' % label
    if not page:
        return label
    return '%s (%s)' % (label, page)


def wrap(text, first='', rest=''):
    import textwrap
    return textwrap.fill(text, WIDTH, initial_indent=first,
                         subsequent_indent=rest, break_long_words=False,
                         break_on_hyphens=False)


def table_text(rows):
    rows = [[inline_text(c.strip()) for c in r] for r in rows]
    head, body = rows[0], rows[1:]
    n = len(head)
    widths = [max(len(r[i]) if i < len(r) else 0 for r in rows) for i in range(n)]
    if sum(widths) + 2 * (n - 1) <= WIDTH:
        line = lambda r: '  '.join((r[i] if i < len(r) else '').ljust(widths[i])
                                   for i in range(n)).rstrip()
        out = [line(head), '  '.join('-' * w for w in widths)]
        out += [line(r) for r in body]
        return '\n'.join(out)
    # Too wide: the first column as a label, the others below it
    out = []
    for r in body:
        out.append(wrap(r[0]))
        for i in range(1, n):
            if i < len(r) and r[i]:
                label = head[i] + ': ' if n > 2 else ''
                out.append(wrap(label + r[i], '    ', '    '))
        out.append('')
    return '\n'.join(out).rstrip()


def md_to_text(md, title):
    lines = md.split('\n')
    out, i = [], 0
    para = []

    def flush():
        if para:
            out.append(wrap(inline_text(' '.join(para))))
            out.append('')
            para.clear()
    while i < len(lines):
        ln = lines[i]
        if ln.startswith('```'):
            flush()
            i += 1
            while i < len(lines) and not lines[i].startswith('```'):
                out.append(('    ' + lines[i]).rstrip())
                i += 1
            out.append('')
        elif re.match(r'#{1,6} ', ln):
            flush()
            level = len(ln) - len(ln.lstrip('#'))
            h = inline_text(ln[level:].strip())
            if level == 1:
                out += ['=' * len(h), h, '=' * len(h), '']
            elif level == 2:
                out += ['', h, '-' * len(h), '']
            else:
                out += [h, '~' * len(h), '']
        elif ln.startswith('|'):
            flush()
            rows = []
            while i < len(lines) and lines[i].startswith('|'):
                cells = re.split(r'(?<!\\)\|', lines[i].strip()[1:-1])
                if not all(re.match(r'^\s*:?-+:?\s*$', c) for c in cells):
                    rows.append(cells)
                i += 1
            out += [table_text(rows), '']
            continue
        elif re.match(r'\s*([-*]|\d+\.) ', ln):
            flush()
            m = re.match(r'(\s*)([-*]|\d+\.) (.*)', ln)
            ind, mark, text = len(m.group(1)), m.group(2), m.group(3)
            i += 1
            while (i < len(lines) and lines[i].strip() and
                   not re.match(r'\s*([-*]|\d+\.) ', lines[i]) and
                   not lines[i].lstrip().startswith('```') and
                   lines[i].startswith(' ')):
                text += ' ' + lines[i].strip()
                i += 1
            bullet = '-' if mark in '-*' else mark
            pad = ' ' * ind
            out.append(wrap(inline_text(text), pad + bullet + ' ',
                            pad + ' ' * (len(bullet) + 1)))
            if i < len(lines) and not lines[i].strip():
                out.append('')
            continue
        elif not ln.strip():
            flush()
        else:
            para.append(ln.strip())
        i += 1
    flush()
    text = '\n'.join(out)
    text = re.sub(r'\n{3,}', '\n\n', text).strip() + '\n'
    return text


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
        txt = md_to_text(open(os.path.join(TOP, src), encoding='utf-8').read(), title)
        files['txt/' + out.replace('.html', '.txt')] = (
            txt + '\n(dr_utils %s documentation, made from %s by docs/make_html.py)\n'
            % (version, src))
        files['html/' + out] = TEMPLATE.format(title=html.escape(title), version=version,
                                     menu=menu, body=body, source=src)
        index.extend(sections(body, title, out))
    files['html/search_index.js'] = ('// Made by docs/make_html.py\nvar DRU_INDEX = ' +
                                json.dumps(index, indent=0, ensure_ascii=False) +
                                ';\n')
    return files


def main():
    ap = argparse.ArgumentParser(description='Build docs/html from the markdown docs')
    ap.add_argument('--check', action='store_true',
                    help='exit 1 if docs/html or docs/txt is out of date, change nothing')
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
            print('out of date: docs/' + name)
        return 1 if stale else 0
    for name in stale:
        os.makedirs(os.path.dirname(os.path.join(HERE, name)), exist_ok=True)
        tmp = os.path.join(HERE, name + '.new')
        with open(tmp, 'w', encoding='utf-8', newline='\n') as f:
            f.write(files[name])
        os.replace(tmp, os.path.join(HERE, name))
    print('docs/html, docs/txt: %d file(s) written, %d up to date' %
          (len(stale), len(files) - len(stale)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
