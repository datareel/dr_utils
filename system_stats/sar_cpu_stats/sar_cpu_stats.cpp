// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Source Code File Name: sar_cpu_stats.cpp
// C++ Compiler Used: MSVC, GCC
// Produced By: DataReel Software Development Team
// File Creation Date: 2018
// Date Last Modified: 11/18/2026
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

Read the CPU sections of sar text reports (sar -u, sar -P ALL, or a
full sar -A report saved as text, one day or many days in one file)
and report CPU use over time: averages and peaks of user, system,
iowait, steal and idle, the intervals where steal or iowait passed a
threshold, gaps in the samples, and samples with impossible values
(a VM that the hypervisor stopped can write steal of 241%). With
--csv it writes every sample as CSV for a spreadsheet or a plot.

Reports from RHEL 5 to RHEL 10 sysstat work: 12 or 24 hour times,
%user or %usr, %system or %sys, with or without %irq, %soft, %guest
and %gnice. Lines of other sar sections, "Average:" lines and LINUX
RESTART lines are skipped. The report is plain text, so it can be
read on any system, Windows too.

Changes:
11/18/2026: dr_utils version, based on Douglas Gaer's 2018 sar CPU
stat reader (system_stats_analysis_tool read_cpu_stats), which parsed
the CPU sections and printed the steal values. Finished: CSV output,
the summary, thresholds, gaps, bad samples, several files, --alarm.
*/
// ----------------------------------------------------------- //
#include "gxdlcode.h"

#include <iostream>
#include <fstream>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "gxstring.h"
#include "drbench.h"
#include "drutil.h"

const char *ProgramName = "sar_cpu_stats";
const char *VersionString = DR_UTILS_VERSION;

// The fields we read, in output order
enum { F_USER, F_NICE, F_SYS, F_IOWAIT, F_STEAL, F_IRQ, F_SOFT, F_GUEST,
       F_GNICE, F_IDLE, F_COUNT };
const char *FieldNames[F_COUNT] = { "user", "nice", "system", "iowait",
				    "steal", "irq", "soft", "guest", "gnice",
				    "idle" };

// Settings
double steal_limit = 10.0;
double iowait_limit = 20.0;
int csv = 0;
int all_cpus = 0;   // -P: every CPU, not only "all"
int max_list = 20;  // Spikes and gaps listed in the summary

const int MaxStepMin = 1440;

// The most common step between samples, in seconds (by minutes)
int CommonStep(const unsigned long *steps)
{
  int best = 0;
  for(int m = 1; m <= MaxStepMin; m++) {
    if(steps[m] > steps[best]) best = m;
  }
  if(steps[best] == 0) return 600; // No steps seen: the sar default
  return best * 60;
}

struct Stats {
  unsigned long samples;
  unsigned long bad;
  double sum[F_COUNT];
  double max[F_COUNT];
  gxString max_when[F_COUNT];
  int has[F_COUNT];  // The report had this column
  double min_idle;
  gxString min_idle_when;
  unsigned long steal_spikes, iowait_spikes, gaps;
  int listed;
  Stats() { Reset(); }
  void Reset() {
    samples = bad = 0;
    for(int i = 0; i < F_COUNT; i++) { sum[i] = 0; max[i] = -1; has[i] = 0; }
    min_idle = 101;
    steal_spikes = iowait_spikes = gaps = 0;
    listed = 0;
  }
};

Stats total;

// Split on spaces and tabs. Returns the count (at most max).
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

// Seconds since midnight from "hh:mm:ss" and an optional AM/PM
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

// Days since 1970-01-01 for a civil date (Howard Hinnant's algorithm)
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

// The date of a "Linux ..." header line: YYYY-MM-DD, MM/DD/YYYY,
// MM/DD/YY or YY-MM-DD. Returns days since 1970, or -1.
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

gxString When(long day, int secs)
{
  int y, m, d;
  CivilFromDays(day, y, m, d);
  char buf[64];
  sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", y, m, d, secs / 3600,
	  (secs / 60) % 60, secs % 60);
  return gxString(buf);
}

void ListLine(Stats &st, const gxString &msg)
{
  if(csv) return;
  if(st.listed < max_list) cout << "  " << msg.c_str() << "\n";
  else if(st.listed == max_list) cout << "  (more not listed)" << "\n";
  st.listed++;
}

// Returns 1 if the file had at least one CPU sample
int ReadReport(const char *fname)
{
  ifstream in(fname, ios::in);
  if(!in) {
    cerr << "ERROR - Cannot open SAR file " << fname << "\n";
    return 0;
  }
  Stats st;
  char line[8192];
  char *tok[64];
  gxString host, kernel;
  long day = -1;
  int col[F_COUNT];
  int cpu_col = -1;
  int in_cpu = 0;
  int last_secs = -1;
  long last_day = -1;
  int interval = 0;        // The shortest step between samples
  static unsigned long steps[MaxStepMin + 1]; // Steps by minutes
  memset(steps, 0, sizeof(steps));
  int restart = 0;
  int day_last_good = -1;  // Last good all-CPU sample of the current day
  const int MaxShort = 400;
  long short_day[MaxShort];
  int short_secs[MaxShort];
  int num_short = 0;
  long first_day = -1, final_day = -1;
  unsigned long days = 0;

  if(!csv) cout << "\n" << "Reading " << fname << "\n";
  while(in.getline(line, sizeof(line)) || in.gcount() > 0) {
    if(in.fail() && !in.eof()) in.clear(); // A long line: the rest follows
    char copy[8192];
    strcpy(copy, line);
    int n = Split(copy, tok, 64);
    if(n == 0) continue;

    if(strcmp(tok[0], "Linux") == 0 && n >= 4) { // A new day
      // A day that stops early, with another day after it in the
      // report: the system stopped writing samples
      int step = CommonStep(steps);
      if(day >= 0 && day_last_good >= 0 && step > 0 &&
	 day_last_good < 86400 - 2 * step - 60 && num_short < MaxShort) {
	short_day[num_short] = day; // Checked at the end of the file
	short_secs[num_short++] = day_last_good;
      }
      day_last_good = -1;
      kernel = tok[1];
      host = tok[2];
      host.FilterChar('('); host.FilterChar(')');
      long d = ParseDate(tok[3]);
      if(d < 0) {
	cerr << "ERROR - Bad date " << tok[3] << " in " << fname << "\n";
	continue;
      }
      day = d;
      days++;
      if(first_day < 0 || d < first_day) first_day = d;
      if(d > final_day) final_day = d;
      in_cpu = 0;
      last_secs = -1;
      continue;
    }
    if(day < 0 || !IsTime(tok[0])) continue; // Average:, other text
    int t = 1;
    const char *ampm = 0;
    if(n > 1 && (strcmp(tok[1], "AM") == 0 || strcmp(tok[1], "PM") == 0)) {
      ampm = tok[1];
      t = 2;
    }
    if(n > t && strcmp(tok[t], "LINUX") == 0) { // LINUX RESTART
      restart = 1;
      in_cpu = 0;
      continue;
    }
    if(n <= t) continue;

    // A header line: a time, then names
    if(!isdigit((unsigned char)tok[t][0]) && strcmp(tok[t], "all") != 0) {
      in_cpu = 0;
      if(strcmp(tok[t], "CPU") != 0) continue;
      for(int f = 0; f < F_COUNT; f++) col[f] = -1;
      int has_idle = 0;
      for(int k = t + 1; k < n; k++) {
	const char *h = tok[k];
	if(*h == '%') h++;
	if(!strcmp(h, "user") || !strcmp(h, "usr")) col[F_USER] = k;
	else if(!strcmp(h, "nice")) col[F_NICE] = k;
	else if(!strcmp(h, "system") || !strcmp(h, "sys")) col[F_SYS] = k;
	else if(!strcmp(h, "iowait")) col[F_IOWAIT] = k;
	else if(!strcmp(h, "steal")) col[F_STEAL] = k;
	else if(!strcmp(h, "irq")) col[F_IRQ] = k;
	else if(!strcmp(h, "soft")) col[F_SOFT] = k;
	else if(!strcmp(h, "guest")) col[F_GUEST] = k;
	else if(!strcmp(h, "gnice")) col[F_GNICE] = k;
	else if(!strcmp(h, "idle")) { col[F_IDLE] = k; has_idle = 1; }
      }
      // A CPU utilization section has %idle and %user or %usr
      if(has_idle && col[F_USER] > 0) {
	in_cpu = 1;
	cpu_col = t;
      }
      continue;
    }
    if(!in_cpu || n <= cpu_col) continue;

    int is_all = (strcmp(tok[cpu_col], "all") == 0);
    if(!is_all && !all_cpus) continue;

    double v[F_COUNT];
    int ok = 1;
    for(int f = 0; f < F_COUNT; f++) {
      v[f] = 0;
      if(col[f] < 0) continue;
      if(col[f] >= n) { ok = 0; break; }
      char *end = 0;
      v[f] = strtod(tok[col[f]], &end);
      if(!end || *end) { ok = 0; break; }
    }
    if(!ok) continue;

    int secs = TimeSecs(tok[0], ampm);
    long sday = day;
    // Past midnight inside one day's report (the last sample, 00:00)
    if(is_all && last_secs >= 0 && secs + 3600 < last_secs) sday = day + 1;
    gxString when = When(sday, secs);

    // Impossible values: a percent over 100, or the parts not adding
    // up to about 100 (time the hypervisor or kernel lost)
    double parts = 0;
    int over = 0;
    for(int f = 0; f < F_COUNT; f++) {
      if(f == F_GUEST || f == F_GNICE) continue; // Inside user and nice
      parts += v[f];
      if(v[f] > 100.0 || v[f] < 0) over = 1;
    }
    int bad = over || fabs(parts - 100.0) > 2.0;

    if(csv) {
      char buf[512];
      sprintf(buf, "%s,%s,%s", host.c_str(), when.c_str(), tok[cpu_col]);
      cout << buf;
      for(int f = 0; f < F_COUNT; f++) {
	if(col[f] < 0) cout << ",";
	else {
	  sprintf(buf, ",%.2f", v[f]);
	  cout << buf;
	}
      }
      cout << "," << (bad ? "bad" : "ok") << "\n";
    }
    if(!is_all) continue; // The summary covers the all-CPU rows

    // Gaps: a sample much later than the usual interval
    if(last_secs >= 0 && last_day == day) {
      int step = (sday == day ? secs : secs + 86400) - last_secs;
      if(step >= 1) {
	if(interval == 0 || step < interval) interval = step;
	if(step / 60 <= MaxStepMin) steps[step / 60]++;
	if(step > interval * 2 + 30 && !restart) {
	  st.gaps++;
	  char buf[256];
	  sprintf(buf, "gap: no samples for %d minutes before %s",
		  step / 60, when.c_str());
	  ListLine(st, gxString(buf));
	}
      }
    }
    restart = 0;
    last_secs = secs;
    last_day = day;

    if(bad) {
      st.bad++;
      char buf[256];
      sprintf(buf, "bad sample at %s: steal %.2f, iowait %.2f, the parts "
	      "add up to %.1f%%", when.c_str(), v[F_STEAL], v[F_IOWAIT],
	      parts);
      ListLine(st, gxString(buf));
      continue; // Not in the averages
    }
    st.samples++;
    if(secs > day_last_good && sday == day) day_last_good = secs;
    for(int f = 0; f < F_COUNT; f++) {
      if(col[f] >= 0) st.has[f] = 1;
      st.sum[f] += v[f];
      if(v[f] > st.max[f]) { st.max[f] = v[f]; st.max_when[f] = when; }
    }
    if(v[F_IDLE] < st.min_idle) {
      st.min_idle = v[F_IDLE]; st.min_idle_when = when;
    }
    if(col[F_STEAL] >= 0 && v[F_STEAL] >= steal_limit) {
      st.steal_spikes++;
      char buf[256];
      sprintf(buf, "steal %.2f%% at %s", v[F_STEAL], when.c_str());
      ListLine(st, gxString(buf));
    }
    if(col[F_IOWAIT] >= 0 && v[F_IOWAIT] >= iowait_limit) {
      st.iowait_spikes++;
      char buf[256];
      sprintf(buf, "iowait %.2f%% at %s", v[F_IOWAIT], when.c_str());
      ListLine(st, gxString(buf));
    }
  }

  // Days that stop early. The last day of a report is often the
  // current day, so only days before the last one count.
  if(day >= 0 && day_last_good >= 0 && num_short < MaxShort) {
    short_day[num_short] = day;
    short_secs[num_short++] = day_last_good;
  }
  int step = CommonStep(steps);
  for(int k = 0; k < num_short && step > 0; k++) {
    if(short_day[k] >= final_day) continue;
    if(short_secs[k] >= 86400 - 2 * step - 60) continue;
    st.gaps++;
    char buf[256];
    sprintf(buf, "gap: samples stop at %s", 
	    When(short_day[k], short_secs[k]).c_str());
    ListLine(st, gxString(buf));
  }

  if(st.samples == 0 && st.bad == 0) {
    if(!csv) cout << "  No CPU samples found" << "\n";
    else cerr << "ERROR - No CPU samples in " << fname << "\n";
    return 0;
  }
  if(!csv) {
    char buf[512];
    char step_str[64];
    if(interval > 0 && interval < 120) sprintf(step_str, "%d seconds", interval);
    else sprintf(step_str, "%d minutes", CommonStep(steps) / 60);
    cout << "Host: " << host.c_str() << ", kernel " << kernel.c_str() << "\n";
    sprintf(buf, "Days: %lu (%s to %s), %lu samples of all CPUs, every %s",
	    days, When(first_day, 0).Left(10).c_str(), 
	    When(final_day, 0).Left(10).c_str(), st.samples, step_str);
    cout << buf << "\n";
    if(st.samples) {
      cout << "           average     peak  peak time" << "\n";
      for(int f = 0; f < F_COUNT; f++) {
	if(!st.has[f]) continue;
	if(f == F_IDLE) {
	  sprintf(buf, "  %-7s %8.2f  %7.2f  %s (lowest)", FieldNames[f],
		  st.sum[f] / st.samples, st.min_idle,
		  st.min_idle_when.c_str());
	}
	else {
	  sprintf(buf, "  %-7s %8.2f  %7.2f  %s", FieldNames[f],
		  st.sum[f] / st.samples, st.max[f],
		  st.max[f] > 0 ? st.max_when[f].c_str() : "-");
	}
	cout << buf << "\n";
      }
    }
    sprintf(buf, "Steal >= %.1f%%: %lu, iowait >= %.1f%%: %lu, gaps: %lu, "
	    "bad samples: %lu", steal_limit, st.steal_spikes, iowait_limit,
	    st.iowait_spikes, st.gaps, st.bad);
    cout << buf << "\n" << flush;
  }
  total.steal_spikes += st.steal_spikes;
  total.iowait_spikes += st.iowait_spikes;
  total.gaps += st.gaps;
  total.bad += st.bad;
  total.samples += st.samples;
  return 1;
}

void HelpMessage()
{
  cout << ProgramName << " version " << VersionString
       << " (dr_utils, DataReel 6.4.1)" << "\n";
  cout << "CPU use from sar text reports: averages, peaks, steal and iowait"
       << " spikes, gaps" << "\n\n";
  cout << "Usage: " << ProgramName << " [args] sar_file [sar_file ...]"
       << "\n";
  cout << "args: " << "\n";
  cout << "     --help                Print help message and exit" << "\n";
  cout << "     --version             Print version number and exit" << "\n";
  cout << "     --csv                 Write every sample as CSV instead of "
       << "the summary" << "\n";
  cout << "     --all-cpus            CSV rows for each CPU too, not only "
       << "\"all\"" << "\n";
  cout << "     --steal=PCT           Steal spike threshold (default 10)"
       << "\n";
  cout << "     --iowait=PCT          Iowait spike threshold (default 20)"
       << "\n";
  cout << "     --list=N              Spikes, gaps and bad samples listed per"
       << " file (default 20)" << "\n";
  cout << "     --alarm               Exit status 3 when there are spikes, "
       << "gaps or bad samples" << "\n";
  cout << "\n";
  cout << "Make a text report with: LC_ALL=C sar -u -f /var/log/sa/saDD > "
       << "file.txt" << "\n";
  cout << "(or sar -A, or sar -P ALL). CSV columns: host,time,cpu,user,nice,"
       << "system,iowait," << "\n"
       << "steal,irq,soft,guest,gnice,idle,ok|bad (empty when the report "
       << "has no such column)." << "\n";
  cout << "Exit status: 0, 1 on an error or no CPU samples, 3 with --alarm."
       << "\n";
  cout << "\n" << flush;
}

int main(int argc, char **argv)
{
  int alarm = 0;
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
    else if(drbench_switch(a, "csv", v)) csv = 1;
    else if(drbench_switch(a, "all-cpus", v)) all_cpus = 1;
    else if(drbench_switch(a, "alarm", v)) alarm = 1;
    else if(drbench_switch(a, "steal", v) || drbench_switch(a, "iowait", v)) {
      double d;
      if(!v || !drutil_parse_amount(v, d) || d > 100) {
	cout << "Error " << a << " needs a percent above 0" << "\n" << flush;
	return 1;
      }
      if(strncmp(a, "--steal", 7) == 0) steal_limit = d;
      else iowait_limit = d;
    }
    else if(drbench_switch(a, "list", v)) {
      unsigned long n;
      if(!drbench_parse_count(v, 1, 100000, n)) {
	cout << "Error --list needs a number" << "\n" << flush;
	return 1;
      }
      max_list = (int)n;
    }
    else if(a[0] == '-' && a[1] != 0) {
      cout << "Error unknown command line switch " << a << "\n" << flush;
      return 1;
    }
    else nfiles++;
  }
  if(nfiles == 0) {
    HelpMessage();
    return 1;
  }
  if(csv) {
    cout << "host,time,cpu";
    for(int f = 0; f < F_COUNT; f++) cout << "," << FieldNames[f];
    cout << ",sample" << "\n";
  }
  int good = 0, failed = 0;
  for(int i = 1; i < argc; i++) {
    if(argv[i][0] == '-' && argv[i][1] != 0) continue;
    if(ReadReport(argv[i])) good++;
    else failed++;
  }
  cout << flush;
  if(failed || good == 0) return 1;
  if(alarm && (total.steal_spikes || total.iowait_spikes || total.gaps ||
	       total.bad)) {
    return 3;
  }
  return 0;
}
// ----------------------------------------------------------- //
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
