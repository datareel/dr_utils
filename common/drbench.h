// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Header File Name: drbench.h
// C++ Compiler Used: MSVC, GCC
// Produced By: DataReel Software Development Team
// File Creation Date: 11/18/2026
// Date Last Modified: 11/19/2026
// Copyright (c) 2001-2026 DataReel Software Development
// ----------------------------------------------------------- //
// ---------- Include File Description and Details  ---------- //
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

Helpers shared by the dr_utils benchmarks: a wall clock timer,
sizes with k/m/g suffixes, short number labels for plot files,
--name=value switches, the host name and a console lock for threads.

Sizes use decimal units (1k = 1000) like the 2002-2019 benchmarks,
so results stay comparable.

Changes:
11/18/2026: First version for dr_utils.
11/19/2026: drbenchDat::Hook(), the results as DRSM lines.
*/
// ----------------------------------------------------------- //
#ifndef __DR_UTILS_BENCH_HPP__
#define __DR_UTILS_BENCH_HPP__

#include "gxdlcode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined (__WIN32__)
#include <windows.h>
#else
#include <unistd.h>
#include <sys/time.h>
#endif

#include "gxstring.h"
#include "gxmutex.h"
#include "drhook.h"

// Seconds since an arbitrary start, from a monotonic clock
inline double drbench_now()
{
#if defined (__WIN32__)
  LARGE_INTEGER f, c;
  QueryPerformanceFrequency(&f);
  QueryPerformanceCounter(&c);
  return (double)c.QuadPart / (double)f.QuadPart;
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
#endif
}

// Sleep for a number of milliseconds
inline void drbench_sleep_ms(int ms)
{
  if(ms <= 0) return;
#if defined (__WIN32__)
  Sleep((DWORD)ms);
#else
  struct timespec ts;
  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (long)(ms % 1000) * 1000000L;
  while(nanosleep(&ts, &ts) != 0) ;
#endif
}

// Paces a sender to a rate in bits a second (0 = no limit)
class drbenchPacer
{
public:
  drbenchPacer(double bits_per_sec) { 
    rate = bits_per_sec / 8.0; start = drbench_now(); sent = 0;
  }
  // Call after each send of "bytes"
  void Sent(double bytes) {
    if(rate <= 0) return;
    sent += bytes;
    double ahead = sent / rate - (drbench_now() - start);
    if(ahead > 0.002) drbench_sleep_ms((int)(ahead * 1000.0));
  }
private:
  double rate, start, sent;
};

// A size such as 1000, 24k, 240m or 2.4g (decimal units). Returns 0
// if "s" is not a size greater than zero or larger than "max".
inline int drbench_parse_size(const char *s, double max, double &value)
{
  if(!s || !*s) return 0;
  char *end = 0;
  double v = strtod(s, &end);
  if(!end || end == s) return 0;
  if(*end) {
    switch(*end) {
      case 'k': case 'K': v *= 1e3; break;
      case 'm': case 'M': v *= 1e6; break;
      case 'g': case 'G': v *= 1e9; break;
      default: return 0;
    }
    if(end[1] != 0) return 0;
  }
  if(v < 1 || v > max) return 0;
  value = v;
  return 1;
}

// A count such as 100k or 1m, at least "min" and at most "max"
inline int drbench_parse_count(const char *s, unsigned long min,
			       unsigned long max, unsigned long &value)
{
  double v;
  if(!drbench_parse_size(s, (double)max, v)) return 0;
  if(v < (double)min) return 0;
  value = (unsigned long)(v + 0.5);
  return 1;
}

// A short label for a count: 100000 is 100K, 1500000 is 1.5M
inline gxString drbench_shn(double n)
{
  char buf[64];
  const char *unit = "";
  if(n >= 1e9) { n /= 1e9; unit = "G"; }
  else if(n >= 1e6) { n /= 1e6; unit = "M"; }
  else if(n >= 1e3) { n /= 1e3; unit = "K"; }
  sprintf(buf, "%.1f", n);
  size_t len = strlen(buf);
  if(len > 2 && buf[len-2] == '.' && buf[len-1] == '0') buf[len-2] = 0;
  gxString s = buf;
  s << unit;
  return s;
}

// A byte rate: 12.3 MB/s (decimal units)
inline gxString drbench_rate(double bytes, double seconds)
{
  char buf[64];
  if(seconds <= 0) seconds = 1e-9;
  double r = bytes / seconds;
  if(r >= 1e9) sprintf(buf, "%.2f GB/s", r / 1e9);
  else if(r >= 1e6) sprintf(buf, "%.2f MB/s", r / 1e6);
  else if(r >= 1e3) sprintf(buf, "%.2f KB/s", r / 1e3);
  else sprintf(buf, "%.0f B/s", r);
  return gxString(buf);
}

// A switch --name=value or --name. Returns 1 if "arg" is --name, and
// sets "value" to what follows "=" (or 0).
inline int drbench_switch(const char *arg, const char *name,
			  const char *&value)
{
  value = 0;
  if(strncmp(arg, "--", 2) != 0) return 0;
  arg += 2;
  size_t n = strlen(name);
  if(strncmp(arg, name, n) != 0) return 0;
  if(arg[n] == 0) return 1;
  if(arg[n] != '=') return 0;
  value = arg + n + 1;
  return 1;
}

// This machine's host name
inline gxString drbench_hostname()
{
  char buf[256];
  buf[0] = 0;
#if defined (__WIN32__)
  DWORD len = sizeof(buf);
  if(!GetComputerNameA(buf, &len)) strcpy(buf, "unknown");
#else
  if(gethostname(buf, sizeof(buf)-1) != 0) strcpy(buf, "unknown");
  buf[sizeof(buf)-1] = 0;
#endif
  return gxString(buf);
}

// Today's date as MM/DD/YYYY
inline gxString drbench_date()
{
  char buf[32];
  time_t now = time(0);
  struct tm *t = localtime(&now);
  strftime(buf, sizeof(buf), "%m/%d/%Y", t);
  return gxString(buf);
}

// The compiler, for the plot file [HOSTINFO] section
inline const char *drbench_build()
{
#if defined (_MSC_VER)
  return "MSVC";
#elif defined (__clang__)
  return "Clang";
#elif defined (__GNUC__)
  return "GCC";
#else
  return "C++";
#endif
}

// A plot data file (--datfile): INI sections that dr_plot.py reads.
//   [BENCHMARK] name, version, settings, the main result (total,
//               total_unit, total_label) and one value per thread,
//               client or phase (items, item_values, item_unit;
//               optional item2_values, item2_unit)
//   [HOSTINFO]  host, date, build, title
// Hook() gives the same results as DRSM lines for the dashboard
// (--drsm, --submit, see drhook.h).
class drbenchDat
{
public:
  drbenchDat(const char *name, const char *version, const char *title) {
    body << "[BENCHMARK]\n" << "name=" << name << "\n" 
	 << "version=" << version << "\n";
    this->title = title;
    this->name = name;
    this->version = version;
    count = 0;
  }
  void Set(const char *key, const char *value) {
    body << key << "=" << value << "\n";
    if(count < MAX_KEYS) {
      keys[count] = key;
      values[count] = value;
      count++;
    }
  }
  void Set(const char *key, double value) {
    char buf[64];
    sprintf(buf, "%.6g", value);
    Set(key, buf);
  }
  // A comma separated list, built one value at a time
  void Add(gxString &list, const char *value) {
    if(!list.is_null()) list << ",";
    list << value;
  }
  void Add(gxString &list, double value) {
    char buf[64];
    sprintf(buf, "%.6g", value);
    Add(list, buf);
  }
  // Write the file. Returns 0 and prints an error if it cannot.
  int Write(const char *fname) {
    FILE *fp = fopen(fname, "w");
    if(!fp) {
      fprintf(stdout, "Error could not write %s\n", fname);
      return 0;
    }
    fprintf(fp, "%s\n[HOSTINFO]\nhost=%s\ndate=%s\nbuild=%s\ntitle=%s\n",
	    body.c_str(), drbench_hostname().c_str(), drbench_date().c_str(),
	    drbench_build(), title.c_str());
    fclose(fp);
    fprintf(stdout, "Plot data written to %s\n", fname);
    fflush(stdout);
    return 1;
  }

  // The value of a key set with Set(), or 0
  const char *Get(const char *key) const {
    for(int i = 0; i < count; i++) {
      if(strcmp(keys[i].c_str(), key) == 0) return values[i].c_str();
    }
    return 0;
  }

  // The results as DRSM lines: an INFO line with the main result, a
  // DETAIL value for each setting and result (the manifest rules and
  // trends use them), and a table of the threads, clients, receivers
  // or phases. "item_header" names the first column (default: the
  // first word of the items when they all share it, else "Item").
  void Hook(drhookLines &lines, const char *item_header = 0) const {
    const char *total = Get("total");
    const char *unit = Get("total_unit");
    const char *label = Get("total_label");
    gxString info, settings;
    for(int i = 0; i < count; i++) {
      if(IsResultKey(keys[i].c_str())) continue;
      if(!settings.is_null()) settings << " ";
      settings << keys[i] << "=" << drhook_value(values[i].c_str());
    }
    info << title << " (" << name << " " << version << "): ";
    if(total) {
      info << (label ? label : "result") << " " << drhook_value(total);
      if(unit) info << " " << unit;
    }
    else {
      info << "done";
    }
    if(!settings.is_null()) info << ", " << settings;
    lines.Info(info.c_str());

    lines.Value("benchmark", name.c_str());
    lines.Value("version", version.c_str());
    lines.Value("build", drbench_build());
    for(int i = 0; i < count; i++) {
      const char *k = keys[i].c_str();
      if(IsListKey(k)) continue;
      lines.Value(k, values[i].c_str());
    }
    // The worst loss of the clients or receivers (udptest, mctest)
    const char *lu = Get("item2_unit");
    const char *lv = Get("item2_values");
    if(lu && lv && strstr(lu, "lost")) {
      double worst = 0;
      const char *p = lv;
      while(*p) {
	gxString f = NextField(p);
	double d = atof(f.c_str());
	if(d > worst) worst = d;
      }
      lines.Value("loss_max_pct", worst);
    }
    lines.Value("collected", drhook_collected().c_str());

    // The table: one row per item
    const char *items = Get("items");
    if(!items || !*items) return;
    const char *v1 = Get("item_values");
    const char *v2 = Get("item2_values");
    const char *u1 = Get("item_unit");
    const char *u2 = Get("item2_unit");
    lines.Columns();
    lines.Cell(item_header ? gxString(item_header).c_str() : 
	       ItemHeader(items).c_str());
    lines.Cell(u1 ? u1 : "Value");
    if(v2) lines.Cell(u2 ? u2 : "Value 2");
    const char *p1 = v1, *p2 = v2, *pi = items;
    while(pi && *pi) {
      gxString item = NextField(pi), a = NextField(p1), b = NextField(p2);
      lines.Row();
      lines.Cell(item.c_str());
      lines.Cell(drhook_value(a.c_str()).c_str());
      if(v2) lines.Cell(drhook_value(b.c_str()).c_str());
    }
  }

private:
  // Lists for the table, not single values
  static int IsListKey(const char *k) {
    return strcmp(k, "items") == 0 || strcmp(k, "item_values") == 0 ||
      strcmp(k, "item2_values") == 0;
  }
  // Results, left out of the settings on the INFO line
  static int IsResultKey(const char *k) {
    return IsListKey(k) || strncmp(k, "total", 5) == 0 ||
      strncmp(k, "item", 4) == 0 || strcmp(k, "elapsed") == 0 ||
      strcmp(k, "loss") == 0 || strcmp(k, "lost") == 0 || 
      strcmp(k, "received") == 0 || strcmp(k, "out_of_order") == 0 ||
      (strlen(k) > 5 && strcmp(k + strlen(k) - 5, "_time") == 0);
  }
  // The next field of a comma separated list
  static gxString NextField(const char *&p) {
    gxString out;
    if(!p) return out;
    while(*p && *p != ',') out << *p++;
    if(*p == ',') p++;
    return out;
  }
  // "Thread" when every item is "Thread n", else "Item"
  static gxString ItemHeader(const char *items) {
    const char *sp = strchr(items, ' ');
    if(!sp || sp == items) return gxString("Item");
    gxString word;
    for(const char *p = items; p < sp; p++) word << *p;
    const char *p = items;
    while(*p) {
      gxString f = NextField(p);
      if(strncmp(f.c_str(), word.c_str(), word.length()) != 0 ||
	 f.c_str()[word.length()] != ' ') return gxString("Item");
    }
    return word;
  }

  enum { MAX_KEYS = 64 };
  gxString body;
  gxString title;
  gxString name;
  gxString version;
  gxString keys[MAX_KEYS];
  gxString values[MAX_KEYS];
  int count;
};

// Console output from several threads, one message at a time
class drbenchConsole
{
public:
  void Write(const gxString &s) {
    lock.MutexLock();
    fputs(s.c_str(), stdout);
    fflush(stdout);
    lock.MutexUnlock();
  }
private:
  gxMutex lock;
};

#endif // __DR_UTILS_BENCH_HPP__
// ----------------------------------------------------------- //
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
