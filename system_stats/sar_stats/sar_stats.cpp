// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Source Code File Name: sar_stats.cpp
// C++ Compiler Used: MSVC, GCC
// Produced By: DataReel Software Development Team
// File Creation Date: 11/20/2026
// Date Last Modified: 11/20/2026
// Copyright (c) 2001-2026 DataReel Software Development
// ----------------------------------------------------------- //
// ------------- Program Description and Details ------------- //
// ----------------------------------------------------------- //
/*
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 3 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library. If not, see
<https://www.gnu.org/licenses/>.

Read every section of sar text reports (sar -A, or any sar report
saved as text, one day or many days in one file) the way the old kSar
tool did: CPU, interrupts, processes and context switches, swapping,
paging, I/O, memory, swap space, huge pages, kernel tables, run queue
and load, pressure stall (PSI), serial lines, block devices, network
interfaces and errors, sockets, IP, ICMP, TCP and UDP (IPv4 and
IPv6), NFS client and server, softnet, Fibre Channel, CPU frequency,
fans, temperatures, voltage inputs, USB devices and file systems.

Sections are found by their column names, so a sysstat version with
new columns or new sections still reads: an unknown section is kept
under its column names. Every row of a section with an item column
(CPU, IFACE, DEV, INTR, FILESYSTEM ...) is kept per item.

Output: a summary of every section (average, lowest and highest of
every column with the time of the highest, per item), --list for the
sections, or --csv for every value as host,time,section,item,metric,
value rows, which dr_plot.py and dr_report.py chart section by
section. Reports from RHEL 5 to RHEL 10 sysstat work: 12 or 24 hour
times, LINUX RESTART lines, several days and several files. The
report is plain text, so it reads on Windows too.

Changes:
11/20/2026: First version, based on work from the 2018
system_stats_analysis_tool, which read the CPU sections only; the
section names follow the old kSar charts.
*/
// ----------------------------------------------------------- //
#include "gxdlcode.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "gxstring.h"
#include "drbench.h"
#include "drutil.h"

const char *ProgramName = "sar_stats";
const char *VersionString = DR_UTILS_VERSION;

// ----------------------------------------------------------- //
// The sar sections, found by the first column after the item column.
// id, title, sar option, first column. A first column that starts
// with "^" is a prefix (i000/s, i001/s ... in the per-CPU interrupts).
// ----------------------------------------------------------- //
struct SectionDef {
  const char *id;
  const char *title;
  const char *option;
  const char *first;
};

const SectionDef SectionDefs[] = {
  { "cpu", "CPU utilization", "-u, -P ALL", "%user" },
  { "cpu", "CPU utilization", "-u, -P ALL", "%usr" },
  { "pcsw", "Task creation and context switches", "-w", "proc/s" },
  { "pcsw", "Task creation and context switches", "-w", "cswch/s" },
  { "intr", "Interrupts", "-I", "intr/s" },
  { "intr_cpu", "Interrupts per CPU", "-I ALL -P ALL", "^i0" },
  { "swap", "Swapping", "-W", "pswpin/s" },
  { "paging", "Paging", "-B", "pgpgin/s" },
  { "io", "I/O and transfer rates", "-b", "tps" },
  { "memstat", "Memory page changes", "-R", "frmpg/s" },
  { "mem", "Memory", "-r", "kbmemfree" },
  { "swpspace", "Swap space", "-S", "kbswpfree" },
  { "huge", "Huge pages", "-H", "kbhugfree" },
  { "ktables", "Kernel tables", "-v", "dentunusd" },
  { "queue", "Run queue and load average", "-q", "runq-sz" },
  { "psi_cpu", "CPU pressure (PSI)", "-q CPU", "%scpu-10" },
  { "psi_io", "I/O pressure (PSI)", "-q IO", "%sio-10" },
  { "psi_mem", "Memory pressure (PSI)", "-q MEM", "%smem-10" },
  { "tty", "Serial lines", "-y", "rcvin/s" },
  { "disk", "Block devices", "-d", "tps" },   // With the DEV column
  { "net_dev", "Network interfaces", "-n DEV", "rxpck/s" },
  { "net_edev", "Network interface errors", "-n EDEV", "rxerr/s" },
  { "nfs", "NFS client", "-n NFS", "call/s" },
  { "nfsd", "NFS server", "-n NFSD", "scall/s" },
  { "sock", "Sockets (IPv4)", "-n SOCK", "totsck" },
  { "ip", "IPv4 traffic", "-n IP", "irec/s" },
  { "eip", "IPv4 errors", "-n EIP", "ihdrerr/s" },
  { "icmp", "ICMPv4 traffic", "-n ICMP", "imsg/s" },
  { "eicmp", "ICMPv4 errors", "-n EICMP", "ierr/s" },
  { "tcp", "TCPv4 traffic", "-n TCP", "active/s" },
  { "etcp", "TCPv4 errors", "-n ETCP", "atmptf/s" },
  { "udp", "UDPv4 traffic", "-n UDP", "idgm/s" },
  { "sock6", "Sockets (IPv6)", "-n SOCK6", "tcp6sck" },
  { "ip6", "IPv6 traffic", "-n IP6", "irec6/s" },
  { "eip6", "IPv6 errors", "-n EIP6", "ihdrer6/s" },
  { "icmp6", "ICMPv6 traffic", "-n ICMP6", "imsg6/s" },
  { "eicmp6", "ICMPv6 errors", "-n EICMP6", "ierr6/s" },
  { "udp6", "UDPv6 traffic", "-n UDP6", "idgm6/s" },
  { "softnet", "Software network processing", "-n SOFT", "total/s" },
  { "fchost", "Fibre Channel hosts", "-n FC", "fch_rxf/s" },
  { "cpufreq", "CPU frequency", "-m CPU", "MHz" },
  { "cpuwfreq", "CPU weighted frequency", "-m FREQ", "wghMHz" },
  { "fan", "Fans", "-m FAN", "rpm" },
  { "temp", "Temperatures", "-m TEMP", "degC" },
  { "in", "Voltage inputs", "-m IN", "inV" },
  { "usb", "USB devices", "-m USB", "idvendor" },
  { "fs", "File systems", "-F", "MBfsfree" },
  { 0, 0, 0, 0 }
};

// ----------------------------------------------------------- //
// Statistics of one column of one item
// ----------------------------------------------------------- //
struct Metric {
  unsigned long n;
  double sum, lo, hi;
  string lo_when, hi_when;
  Metric() : n(0), sum(0), lo(0), hi(0) { }
  void Add(double v, const string &when) {
    if(n == 0 || v < lo) { lo = v; lo_when = when; }
    if(n == 0 || v > hi) { hi = v; hi_when = when; }
    sum += v;
    n++;
  }
  void Merge(const Metric &o) {
    if(o.n == 0) return;
    if(n == 0 || o.lo < lo) { lo = o.lo; lo_when = o.lo_when; }
    if(n == 0 || o.hi > hi) { hi = o.hi; hi_when = o.hi_when; }
    sum += o.sum;
    n += o.n;
  }
  double Avg() const { return n ? sum / n : 0; }
};

struct Section {
  string id, title, option;
  string key_name;                 // CPU, IFACE, DEV ... or empty
  vector<string> metrics;          // Column names, first seen order
  vector<string> items;            // Item names, first seen order
  map<string, map<string, Metric> > stats; // item -> metric -> stats
  map<string, unsigned long> rows; // item -> samples
  map<string, set<string> > times; // item -> times of its samples
  void AddMetric(const string &m) {
    for(size_t i = 0; i < metrics.size(); i++) if(metrics[i] == m) return;
    metrics.push_back(m);
  }
  void AddItem(const string &it) {
    if(rows.find(it) == rows.end()) { items.push_back(it); rows[it] = 0; }
  }
  // The item that stands for the whole system: "all" CPUs, "sum" of
  // the interrupts, or the only item
  string MainItem() const {
    if(rows.find("all") != rows.end()) return "all";
    if(rows.find("sum") != rows.end()) return "sum";
    if(items.size() == 1) return items[0];
    return string("\001"); // None
  }
  unsigned long Samples() const {
    unsigned long m = 0;
    for(map<string, unsigned long>::const_iterator i = rows.begin();
	i != rows.end(); ++i) if(i->second > m) m = i->second;
    return m;
  }
};

struct Report {
  string host, kernel, cpus;
  long first_day, final_day;
  unsigned long days;
  int step;                         // Shortest step between samples
  string step_item;                 // The item the step is measured on
  int prev_secs;
  map<int, unsigned long> steps;    // Step in seconds -> count
  vector<string> order;             // Section ids, first seen order
  map<string, Section> sections;
  Report() : first_day(-1), final_day(-1), days(0), step(0), prev_secs(-1) { }
  Section &Get(const string &id) {
    map<string, Section>::iterator i = sections.find(id);
    if(i == sections.end()) order.push_back(id);
    return sections[id];
  }
};

// Settings
int csv = 0;
int list_only = 0;
int max_items = 20;
vector<string> want_sections;
vector<string> want_items;

drhookOptions hook("sar_stats");
Report total;       // Every file, for the dashboard
int total_files = 0;

// ----------------------------------------------------------- //
// Helpers
// ----------------------------------------------------------- //
int Split(char *line, char **tok, int max)
{
  int n = 0;
  char *p = line;
  while(*p && n < max) {
    while(*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    if(!*p) break;
    tok[n++] = p;
    while(*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') p++;
    if(*p) *p++ = 0;
  }
  return n;
}

int IsTime(const char *s)
{
  int h, m, sec;
  char extra;
  return sscanf(s, "%d:%d:%d%c", &h, &m, &sec, &extra) == 3;
}

int TimeSecs(const char *s, const char *ampm)
{
  int h = 0, m = 0, sec = 0;
  sscanf(s, "%d:%d:%d", &h, &m, &sec);
  if(ampm) {
    if(strcmp(ampm, "PM") == 0 && h < 12) h += 12;
    if(strcmp(ampm, "AM") == 0 && h == 12) h = 0;
  }
  return h * 3600 + m * 60 + sec;
}

int IsNumber(const char *s, double *v = 0)
{
  if(!*s) return 0;
  char *end = 0;
  double d = strtod(s, &end);
  if(!end || *end) return 0;
  if(v) *v = d;
  return 1;
}

// Days since 1970-01-01 for a civil date
long DaysFromCivil(int y, int m, int d)
{
  y -= m <= 2;
  long era = (y >= 0 ? y : y - 399) / 400;
  unsigned yoe = (unsigned)(y - era * 400);
  unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + (long)doe - 719468;
}

void CivilFromDays(long z, int &y, int &m, int &d)
{
  z += 719468;
  long era = (z >= 0 ? z : z - 146096) / 146097;
  unsigned doe = (unsigned)(z - era * 146097);
  unsigned yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;
  long yy = (long)yoe + era * 400;
  unsigned doy = doe - (365*yoe + yoe/4 - yoe/100);
  unsigned mp = (5*doy + 2)/153;
  d = (int)(doy - (153*mp+2)/5 + 1);
  m = (int)(mp < 10 ? mp+3 : mp-9);
  y = (int)(yy + (m <= 2));
}

long ParseDate(const char *s)
{
  int a, b, c;
  if(sscanf(s, "%d-%d-%d", &a, &b, &c) == 3) {
    if(a < 100) a += 2000;
    return DaysFromCivil(a, b, c);
  }
  if(sscanf(s, "%d/%d/%d", &a, &b, &c) == 3) {
    if(c < 100) c += 2000;
    return DaysFromCivil(c, a, b);
  }
  return -1;
}

string When(long day, int secs)
{
  int y, m, d;
  CivilFromDays(day, y, m, d);
  char buf[64];
  sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", y, m, d, secs / 3600,
	  (secs / 60) % 60, secs % 60);
  return string(buf);
}

// A key (item) column: a name in capitals, such as CPU, IFACE, DEV,
// INTR, TTY, BUS, FAN, TEMP, IN, DEVICE, FILESYSTEM, FCHOST
int IsKeyName(const char *s)
{
  if(strlen(s) < 2) return 0;
  for(const char *p = s; *p; p++) if(!isupper((unsigned char)*p)) return 0;
  return 1;
}

// The section of a header: the columns without the key columns
void FindSection(const vector<string> &metrics, const string &key,
		 string &id, string &title, string &option)
{
  string first = metrics.empty() ? string("") : metrics[0];
  for(int i = 0; SectionDefs[i].id; i++) {
    const SectionDef &d = SectionDefs[i];
    int match;
    if(d.first[0] == '^') match = first.compare(0, strlen(d.first) - 1, d.first + 1) == 0;
    else match = first == d.first;
    if(!match) continue;
    // tps opens both -b (no item) and -d (DEV)
    if(strcmp(d.first, "tps") == 0 && (strcmp(d.id, "disk") == 0) != (key == "DEV")) continue;
    if(strcmp(d.first, "rcvin/s") == 0 && key != "TTY") continue;
    id = d.id; title = d.title; option = d.option;
    return;
  }
  // A section this program does not know: kept under its columns
  id = "other_";
  for(size_t i = 0; i < first.size(); i++) {
    char c = first[i];
    id += isalnum((unsigned char)c) ? (char)tolower((unsigned char)c) : '_';
  }
  title = "";
  if(!key.empty()) title = key + " ";
  for(size_t i = 0; i < metrics.size() && i < 4; i++) {
    title += metrics[i];
    if(i + 1 < metrics.size()) title += " ";
  }
  if(metrics.size() > 4) title += "...";
  option = "?";
}

int Wanted(const vector<string> &list, const string &s, const string &s2 = "")
{
  if(list.empty()) return 1;
  for(size_t i = 0; i < list.size(); i++) {
    if(list[i] == s || (!s2.empty() && list[i] == s2)) return 1;
  }
  return 0;
}

// A name for the dashboard: %usr -> pct_usr, rxkB/s -> rxkb_s
string ValueName(const string &s)
{
  string out;
  for(size_t i = 0; i < s.size(); i++) {
    char c = s[i];
    if(c == '%') { out += "pct_"; continue; }
    if(isalnum((unsigned char)c)) out += (char)tolower((unsigned char)c);
    else if(!out.empty() && out[out.size() - 1] != '_') out += '_';
  }
  while(!out.empty() && out[out.size() - 1] == '_') out.erase(out.size() - 1);
  return out;
}

string Num(double v)
{
  char buf[64];
  double a = fabs(v);
  if(a >= 1e7) sprintf(buf, "%.0f", v);
  else sprintf(buf, "%.2f", v);
  return string(buf);
}

string CsvField(const string &s)
{
  if(s.find_first_of(",\"\n") == string::npos) return s;
  string out = "\"";
  for(size_t i = 0; i < s.size(); i++) {
    if(s[i] == '"') out += "\"\"";
    else out += s[i];
  }
  return out + "\"";
}

// ----------------------------------------------------------- //
// Reading
// ----------------------------------------------------------- //
int ReadReport(const char *fname, Report &rep)
{
  ifstream in(fname, ios::in);
  if(!in) {
    cerr << "ERROR - Cannot open SAR file " << fname << "\n";
    return 0;
  }
  long day = -1;
  Section *cur = 0;
  vector<string> cols;         // Header names after the time
  vector<int> keycol;          // Positions of the key columns in cols
  int last_secs = -1;
  int day_offset = 0;
  unsigned long values = 0;
  string line;
  static char buf[65536];
  char *tok[1024];

  while(getline(in, line)) {
    if(line.size() >= sizeof(buf)) continue;
    strcpy(buf, line.c_str());
    int n = Split(buf, tok, 1024);
    if(n == 0) continue;

    if(strcmp(tok[0], "Linux") == 0 && n >= 4) { // A new day
      rep.kernel = tok[1];
      string h = tok[2];
      if(h.size() > 2 && h[0] == '(') h = h.substr(1, h.size() - 2);
      rep.host = h;
      for(int k = 4; k < n; k++) {
	if(tok[k][0] == '(' && k + 1 < n && strcmp(tok[k + 1], "CPU)") == 0) {
	  rep.cpus = tok[k] + 1;
	}
      }
      long d = ParseDate(tok[3]);
      if(d < 0) {
	cerr << "ERROR - Bad date " << tok[3] << " in " << fname << "\n";
	continue;
      }
      day = d;
      rep.days++;
      if(rep.first_day < 0 || d < rep.first_day) rep.first_day = d;
      if(d > rep.final_day) rep.final_day = d;
      cur = 0;
      continue;
    }
    if(day < 0 || !IsTime(tok[0])) continue; // Average:, Summary:, text
    int t = 1;
    const char *ampm = 0;
    if(n > 1 && (strcmp(tok[1], "AM") == 0 || strcmp(tok[1], "PM") == 0)) {
      ampm = tok[1];
      t = 2;
    }
    if(n <= t) continue;
    if(strcmp(tok[t], "LINUX") == 0) { cur = 0; continue; } // RESTART

    // A header: no number after the time
    int numbers = 0;
    for(int k = t; k < n; k++) if(IsNumber(tok[k])) { numbers = 1; break; }
    if(!numbers) {
      cols.clear(); keycol.clear();
      vector<string> metrics;
      string key;
      for(int k = t; k < n; k++) {
	cols.push_back(tok[k]);
	if(IsKeyName(tok[k])) {
	  keycol.push_back(k - t);
	  if(!key.empty()) key += "/";
	  key += tok[k];
	}
	else metrics.push_back(tok[k]);
      }
      if(metrics.empty()) { cur = 0; continue; }
      string id, title, option;
      FindSection(metrics, key, id, title, option);
      cur = &rep.Get(id);
      if(cur->id.empty()) {
	cur->id = id; cur->title = title; cur->option = option;
	cur->key_name = key;
      }
      for(size_t i = 0; i < metrics.size(); i++) cur->AddMetric(metrics[i]);
      last_secs = -1;
      day_offset = 0;
      continue;
    }
    if(!cur) continue;

    // A data row
    int nv = n - t;
    int nc = (int)cols.size();
    if(nv < nc) continue;
    // Extra words: the last key column has spaces (a file system or
    // device name), or text columns (USB names): join into the last
    // column when it is a key column, else drop them
    vector<string> v;
    for(int k = 0; k < nc; k++) v.push_back(tok[t + k]);
    if(nv > nc) {
      if(!keycol.empty() && keycol.back() == nc - 1) {
	for(int k = nc; k < nv; k++) v[nc - 1] += string(" ") + tok[t + k];
      }
    }
    string item;
    for(size_t k = 0; k < keycol.size(); k++) {
      if(!item.empty()) item += "/";
      item += v[keycol[k]];
    }
    int secs = TimeSecs(tok[0], ampm);
    // Past midnight inside one day's section (the 00:00 sample)
    if(last_secs >= 0 && secs + 3600 < last_secs) day_offset = 1;
    last_secs = secs;
    string when = When(day + day_offset, secs);
    if(!Wanted(want_sections, cur->id)) continue;
    if(!Wanted(want_items, item)) continue;
    cur->AddItem(item);
    // RHEL 5 prints proc/s and cswch/s as two sections: one sample
    // per time
    if(cur->times[item].insert(when).second) cur->rows[item]++;
    map<string, Metric> &ms = cur->stats[item];
    for(int k = 0; k < nc; k++) {
      int is_key = 0;
      for(size_t q = 0; q < keycol.size(); q++) if(keycol[q] == k) is_key = 1;
      if(is_key) continue;
      // USB ids and names are labels, not values
      if(cols[k] == "idvendor" || cols[k] == "idprod" || cols[k] == "manufact" ||
	 cols[k] == "product") continue;
      double x;
      if(!IsNumber(v[k].c_str(), &x)) continue;
      ms[cols[k]].Add(x, when);
      values++;
      if(csv) {
	cout << CsvField(rep.host) << "," << when << "," << cur->id << ","
	     << CsvField(item) << "," << CsvField(cols[k]) << "," << v[k]
	     << "\n";
      }
    }
    // Shortest step between samples, on the first item seen
    if(rep.step_item.empty()) rep.step_item = cur->id + "/" + item;
    if(rep.step_item == cur->id + "/" + item) {
      if(rep.prev_secs >= 0 && secs > rep.prev_secs) {
	int st = secs - rep.prev_secs;
	if(st >= 120) st = (st + 30) / 60 * 60; // Whole minutes
	unsigned long c = ++rep.steps[st];
	if(rep.step == 0 || c > rep.steps[rep.step]) rep.step = st;
      }
      rep.prev_secs = secs;
    }
  }
  if(values == 0) {
    cerr << "ERROR - No sar samples in " << fname << "\n";
    return 0;
  }
  return 1;
}

// ----------------------------------------------------------- //
// Output
// ----------------------------------------------------------- //
void PrintHeader(const char *fname, Report &rep)
{
  cout << "\n" << "Reading " << fname << "\n";
  cout << "Host: " << rep.host << ", kernel " << rep.kernel;
  if(!rep.cpus.empty()) cout << ", " << rep.cpus << " CPUs";
  cout << "\n";
  char buf[256];
  string step;
  if(rep.step > 0 && rep.step < 120) sprintf(buf, "%d seconds", rep.step);
  else sprintf(buf, "%d minutes", rep.step > 0 ? rep.step / 60 : 10);
  step = buf;
  sprintf(buf, "Days: %lu (%s to %s), samples every %s, %d sections",
	  rep.days, When(rep.first_day, 0).substr(0, 10).c_str(),
	  When(rep.final_day, 0).substr(0, 10).c_str(), step.c_str(),
	  (int)rep.order.size());
  cout << buf << "\n";
}

void PrintList(Report &rep)
{
  char buf[512];
  cout << "  id          sar option      items  samples  title" << "\n";
  for(size_t i = 0; i < rep.order.size(); i++) {
    Section &s = rep.sections[rep.order[i]];
    if(s.items.empty()) continue;
    sprintf(buf, "  %-11s %-14s %6d %8lu  %s", s.id.c_str(), s.option.c_str(),
	    (int)s.items.size(), s.Samples(), s.title.c_str());
    cout << buf << "\n";
    string c = "      columns:";
    if(!s.key_name.empty()) c += " [" + s.key_name + "]";
    for(size_t m = 0; m < s.metrics.size(); m++) c += " " + s.metrics[m];
    cout << c << "\n";
  }
}

void PrintSection(Section &s)
{
  if(s.items.empty()) return;
  char buf[1024];
  cout << "\n" << "== " << s.id << ": " << s.title;
  if(s.option != "?") cout << " (sar " << s.option << ")";
  if(!s.key_name.empty()) cout << ", " << s.items.size() << " " << s.key_name;
  cout << ", " << s.Samples() << " samples" << "\n";

  string main = s.MainItem();
  if(main != "\001") {
    if(s.items.size() > 1) cout << "  " << s.key_name << " " << main << ":\n";
    cout << "  column            average       lowest      highest  highest at"
	 << "\n";
    map<string, Metric> &ms = s.stats[main];
    for(size_t m = 0; m < s.metrics.size(); m++) {
      map<string, Metric>::iterator it = ms.find(s.metrics[m]);
      if(it == ms.end() || it->second.n == 0) continue;
      Metric &x = it->second;
      sprintf(buf, "  %-14s %12s %12s %12s  %s", s.metrics[m].c_str(),
	      Num(x.Avg()).c_str(), Num(x.lo).c_str(), Num(x.hi).c_str(),
	      x.hi == x.lo ? "-" : x.hi_when.c_str());
      cout << buf << "\n";
    }
  }
  // Every item: the averages, then the highest value of each column
  if(s.items.size() > 1 || main == "\001") {
    vector<string> rows;
    for(size_t i = 0; i < s.items.size(); i++) {
      if(s.items[i] != main) rows.push_back(s.items[i]);
    }
    if(rows.empty()) return;
    cout << "  averages per " << (s.key_name.empty() ? "item" : s.key_name)
	 << ":" << "\n";
    string head = "  ";
    sprintf(buf, "%-16s", s.key_name.empty() ? "item" : s.key_name.c_str());
    head += buf;
    for(size_t m = 0; m < s.metrics.size(); m++) {
      sprintf(buf, " %11s", s.metrics[m].substr(0, 11).c_str());
      head += buf;
    }
    cout << head << "\n";
    int shown = 0;
    for(size_t i = 0; i < rows.size(); i++) {
      if(shown >= max_items) {
	cout << "  (" << rows.size() - shown << " more, use --item= or --items=)"
	     << "\n";
	break;
      }
      string l = "  ";
      string name = rows[i];
      if(name.size() > 16) name = name.substr(0, 15) + "~";
      sprintf(buf, "%-16s", name.c_str());
      l += buf;
      map<string, Metric> &ms = s.stats[rows[i]];
      for(size_t m = 0; m < s.metrics.size(); m++) {
	map<string, Metric>::iterator it = ms.find(s.metrics[m]);
	if(it == ms.end() || it->second.n == 0) sprintf(buf, " %11s", "-");
	else sprintf(buf, " %11s", Num(it->second.Avg()).c_str());
	l += buf;
      }
      cout << l << "\n";
      shown++;
    }
    cout << "  highest:" << "\n";
    for(size_t m = 0; m < s.metrics.size(); m++) {
      double best = 0;
      string who, when;
      int found = 0;
      for(size_t i = 0; i < rows.size(); i++) {
	map<string, Metric> &ms = s.stats[rows[i]];
	map<string, Metric>::iterator it = ms.find(s.metrics[m]);
	if(it == ms.end() || it->second.n == 0) continue;
	if(!found || it->second.hi > best) {
	  best = it->second.hi; who = rows[i]; when = it->second.hi_when;
	  found = 1;
	}
      }
      if(!found) continue;
      if(best == 0) sprintf(buf, "  %-14s %12s", s.metrics[m].c_str(), "0.00");
      else sprintf(buf, "  %-14s %12s  %s at %s", s.metrics[m].c_str(),
		   Num(best).c_str(), who.c_str(), when.c_str());
      cout << buf << "\n";
    }
  }
}

void MergeInto(Report &to, Report &from)
{
  if(to.host.empty()) to.host = from.host;
  else if(to.host != from.host) to.host = "(several)";
  to.days += from.days;
  if(to.first_day < 0 || (from.first_day >= 0 && from.first_day < to.first_day))
    to.first_day = from.first_day;
  if(from.final_day > to.final_day) to.final_day = from.final_day;
  for(size_t i = 0; i < from.order.size(); i++) {
    Section &f = from.sections[from.order[i]];
    Section &t = to.Get(f.id);
    if(t.id.empty()) {
      t.id = f.id; t.title = f.title; t.option = f.option;
      t.key_name = f.key_name;
    }
    for(size_t m = 0; m < f.metrics.size(); m++) t.AddMetric(f.metrics[m]);
    for(size_t k = 0; k < f.items.size(); k++) {
      const string &it = f.items[k];
      t.AddItem(it);
      t.rows[it] += f.rows[it];
      map<string, Metric> &src = f.stats[it];
      for(map<string, Metric>::iterator q = src.begin(); q != src.end(); ++q) {
	t.stats[it][q->first].Merge(q->second);
      }
    }
  }
}

// DRSM lines: a value per column of the whole-system item of each
// section (average and highest), the highest of every column over the
// items of the other sections, and a table row per section
int HookFinish(int nfiles, int good)
{
  drhookLines lines;
  char buf[512];
  int nsec = 0;
  for(size_t i = 0; i < total.order.size(); i++) {
    if(!total.sections[total.order[i]].items.empty()) nsec++;
  }
  sprintf(buf, "sar report (%s %s): %d file(s), %d sections, host %s, %s to %s",
	  ProgramName, VersionString, good, nsec, total.host.c_str(),
	  total.first_day >= 0 ? When(total.first_day, 0).substr(0, 10).c_str() : "-",
	  total.final_day >= 0 ? When(total.final_day, 0).substr(0, 10).c_str() : "-");
  if(good) lines.Info(buf);
  else lines.Error("No sar samples read");
  lines.Value("program", ProgramName);
  lines.Value("version", VersionString);
  lines.Value("files", (double)nfiles);
  lines.Value("files_read", (double)good);
  lines.Value("files_failed", (double)(nfiles - good));
  lines.Value("sar_host", total.host.c_str());
  lines.Value("sections", (double)nsec);
  for(size_t i = 0; i < total.order.size(); i++) {
    Section &s = total.sections[total.order[i]];
    if(s.items.empty()) continue;
    string main = s.MainItem();
    for(size_t m = 0; m < s.metrics.size(); m++) {
      string base = ValueName(s.id) + "_" + ValueName(s.metrics[m]);
      if(main != "\001") {
	Metric &x = s.stats[main][s.metrics[m]];
	if(x.n == 0) continue;
	lines.Value((base + "_avg").c_str(), Num(x.Avg()).c_str());
	lines.Value((base + "_max").c_str(), Num(x.hi).c_str());
      }
      else {
	Metric best;
	for(size_t k = 0; k < s.items.size(); k++) {
	  best.Merge(s.stats[s.items[k]][s.metrics[m]]);
	}
	if(best.n == 0) continue;
	lines.Value((base + "_max").c_str(), Num(best.hi).c_str());
      }
    }
    lines.Value((ValueName(s.id) + "_items").c_str(), (double)s.items.size());
  }
  lines.Value("collected", drhook_collected().c_str());
  lines.Columns();
  const char *cols[] = { "Section", "sar", "Title", "Items", "Samples",
			 "Whole system (average)", 0 };
  for(int i = 0; cols[i]; i++) lines.Cell(cols[i]);
  for(size_t i = 0; i < total.order.size(); i++) {
    Section &s = total.sections[total.order[i]];
    if(s.items.empty()) continue;
    lines.Row();
    lines.Cell(s.id.c_str());
    lines.Cell(s.option.c_str());
    lines.Cell(s.title.c_str());
    lines.Cell((double)s.items.size());
    lines.Cell((double)s.Samples());
    string main = s.MainItem(), text;
    if(main != "\001") {
      for(size_t m = 0; m < s.metrics.size() && m < 6; m++) {
	Metric &x = s.stats[main][s.metrics[m]];
	if(x.n == 0) continue;
	if(!text.empty()) text += ", ";
	text += s.metrics[m] + " " + Num(x.Avg());
      }
    }
    lines.Cell(text.c_str());
  }
  return hook.Finish(lines);
}

void HelpMessage()
{
  cout << ProgramName << " version " << VersionString
       << " (dr_utils, DataReel 6.4.1)" << "\n";
  cout << "Every section of sar text reports: CPU, memory, swap, paging, I/O,"
       << " disks," << "\n"
       << "network, sockets, NFS, load, interrupts, power, file systems ..."
       << "\n\n";
  cout << "Usage: " << ProgramName << " [args] sar_file [sar_file ...]" << "\n";
  cout << "args: " << "\n";
  cout << "     --help                Print help message and exit" << "\n";
  cout << "     --version             Print version number and exit" << "\n";
  cout << "     --list                List the sections, their items and "
       << "columns" << "\n";
  cout << "     --csv                 Every value as CSV: host,time,section,"
       << "item,metric,value" << "\n";
  cout << "     --section=ID[,ID]     Only these sections (ids from --list: "
       << "cpu,mem,disk,net_dev ...)" << "\n";
  cout << "     --item=NAME[,NAME]    Only these items (all, 0, eth0, sda, "
       << "dev8-0 ...)" << "\n";
  cout << "     --items=N             Items listed per section in the "
       << "summary (default 20)" << "\n";
  cout << "\n";
  cout << "Make a text report with: LC_ALL=C sar -A -f /var/log/sa/saDD > "
       << "file.txt" << "\n";
  cout << "(sar -A, or any sar report, one day or several days). dr_plot.py "
       << "and dr_report.py" << "\n"
       << "chart the --csv output section by section." << "\n";
  cout << "Exit status: 0, 1 on an error or no samples." << "\n";
  cout << "\n" << flush;
  drhook_usage(ProgramName);
}

void SplitList(const char *v, vector<string> &out)
{
  string s = v ? v : "";
  size_t p = 0;
  while(p <= s.size()) {
    size_t c = s.find(',', p);
    if(c == string::npos) c = s.size();
    if(c > p) out.push_back(s.substr(p, c - p));
    p = c + 1;
  }
}

int main(int argc, char **argv)
{
  int nfiles = 0;
  for(int i = 1; i < argc; i++) {
    const char *a = argv[i];
    const char *v;
    if(drbench_switch(a, "help", v) || strcmp(a, "-?") == 0) {
      HelpMessage();
      return 0;
    }
    else if(drbench_switch(a, "version", v)) {
      cout << ProgramName << " version " << VersionString << "\n" << flush;
      return 0;
    }
    else if(hook.Arg(a)) ;
    else if(drbench_switch(a, "csv", v)) csv = 1;
    else if(drbench_switch(a, "list", v)) list_only = 1;
    else if(drbench_switch(a, "section", v)) {
      if(!v || !*v) { cout << "Error --section needs ids" << "\n"; return 1; }
      SplitList(v, want_sections);
    }
    else if(drbench_switch(a, "items", v)) {
      unsigned long n;
      if(!drbench_parse_count(v, 1, 100000, n)) {
	cout << "Error --items needs a number" << "\n" << flush;
	return 1;
      }
      max_items = (int)n;
    }
    else if(drbench_switch(a, "item", v)) {
      if(!v || !*v) { cout << "Error --item needs names" << "\n"; return 1; }
      SplitList(v, want_items);
    }
    else if(a[0] == '-' && a[1] != 0) {
      cout << "Error unknown command line switch " << a << "\n" << flush;
      return 1;
    }
    else nfiles++;
  }
  if(!hook.Check()) return 1;
  if(nfiles == 0) {
    HelpMessage();
    return 1;
  }
  if(csv) cout << "host,time,section,item,metric,value" << "\n";
  int good = 0, failed = 0;
  for(int i = 1; i < argc; i++) {
    if(argv[i][0] == '-' && argv[i][1] != 0) continue;
    Report rep;
    if(!ReadReport(argv[i], rep)) { failed++; continue; }
    good++;
    total_files++;
    if(!csv) {
      PrintHeader(argv[i], rep);
      if(list_only) PrintList(rep);
      else {
	for(size_t k = 0; k < rep.order.size(); k++) {
	  PrintSection(rep.sections[rep.order[k]]);
	}
      }
    }
    if(hook.Wanted()) MergeInto(total, rep);
  }
  cout << flush;
  if(hook.Wanted() && !HookFinish(nfiles, good)) return 1;
  if(failed || good == 0) return 1;
  return 0;
}
// ----------------------------------------------------------- //
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
