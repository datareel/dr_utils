// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Header File Name: drbench.h
// C++ Compiler Used: MSVC, GCC
// Produced By: DataReel Software Development Team
// File Creation Date: 11/18/2026
// Date Last Modified: 11/18/2026
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
class drbenchDat
{
public:
  drbenchDat(const char *name, const char *version, const char *title) {
    body << "[BENCHMARK]\n" << "name=" << name << "\n" 
	 << "version=" << version << "\n";
    this->title = title;
  }
  void Set(const char *key, const char *value) {
    body << key << "=" << value << "\n";
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
private:
  gxString body;
  gxString title;
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
