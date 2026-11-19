// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Source Code File Name: filetimes.cpp
// C++ Compiler Used: MSVC, GCC
// Produced By: DataReel Software Development Team
// File Creation Date: 09/27/2005
// Date Last Modified: 11/19/2026
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

Print a file's access, modification and creation times in local
time and GMT, and how old the file is. With two or more files, -n
prints the name of the newest file and -o the oldest (build and feed
freshness checks).

Changes:
11/18/2026: dr_utils version, based on work from the 2005 filetimes
program. Several files at once, -n/-o to compare them, ages in
days/hours/minutes/seconds. On Linux "creation" is the inode change
time (ctime).
11/19/2026: --drsm and --submit: results for the DRSM dashboard through
the DataReel Reporting Agents (hooks/README.md).
*/
// ----------------------------------------------------------- //
#include "gxdlcode.h"

#include <iostream>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "systime.h"
#include "drutil.h"
#include "drhook.h"

const char *ProgramName = "filetimes";
const char *VersionString = DR_UTILS_VERSION;

void HelpMessage(const char *pname)
{
  cout << ProgramName << " version " << VersionString
       << " (dr_utils, DataReel 6.4.1)" << "\n";
  cout << "\n";
  cout << "Usage: " << pname << " [-n | -o] file [file ...]" << "\n";
  cout << "file = file to read" << "\n";
  cout << "-n   = print only the name of the newest file (mod time)" << "\n";
  cout << "-o   = print only the name of the oldest file (mod time)" << "\n";
  cout << "\n";
  cout << "Without -n or -o, prints the access, modification and" << "\n"
       << "creation times of each file in local time and GMT, and" << "\n"
       << "the file's age. Exit status 1 if any file cannot be read."
       << "\n";
  cout << "\n" << flush;
  drhook_usage(ProgramName);
}

void PrintAge(double seconds)
{
  char buf[128];
  if(seconds < 60) {
    sprintf(buf, "%.0f second(s)", seconds);
  }
  else if(seconds < 3600) {
    sprintf(buf, "%.2f minute(s)", seconds / 60.0);
  }
  else if(seconds < 86400) {
    sprintf(buf, "%.2f hour(s)", seconds / 3600.0);
  }
  else {
    sprintf(buf, "%.2f day(s) (%.2f hours)", seconds / 86400.0,
	    seconds / 3600.0);
  }
  cout << "File is " << buf << " old" << "\n";
}

// Dashboard output (--drsm, --submit, see common/drhook.h): set
// hook_ready once the arguments are good, so a usage error sends
// nothing
drhookOptions hook("filetimes");
drhookTee hook_tee;
drhookLines hook_summary;
drhookLines hook_table;
int hook_ready = 0;
int hook_first = 0, hook_argc = 0;
char **hook_argv = 0;

int RunTool(int argc, char **argv)
{
  int newest = 0, oldest = 0;
  int narg = 1;
  while(narg < argc && argv[narg][0] == '-' && argv[narg][1] != 0) {
    if(strcmp(argv[narg], "-n") == 0) newest = 1;
    else if(strcmp(argv[narg], "-o") == 0) oldest = 1;
    else if(strcmp(argv[narg], "-?") == 0 ||
	    strcmp(argv[narg], "-h") == 0) {
      HelpMessage(argv[0]);
      return 0;
    }
    else {
      cout << "Unknown switch " << argv[narg] << "\n" << flush;
      return 1;
    }
    narg++;
  }
  if(narg >= argc || (newest && oldest)) {
    HelpMessage(argv[0]);
    return 1;
  }

  hook_ready = 1;
  hook_first = narg;
  hook_argc = argc;
  hook_argv = argv;
  int errors = 0;
  SysTime systime;
  time_t now;
  time(&now);

  if(newest || oldest) {
    gxString pick;
    time_t pick_time = 0;
    for(int i = narg; i < argc; i++) {
      time_t a, m, c;
      double seconds;
      int rv = drutil_file_age(argv[i], seconds, a, m, c);
      if(rv != DRUTIL_AGE_OK && rv != DRUTIL_AGE_FUTURE) {
	if(rv != DRUTIL_AGE_NOT_FILE) { // Directories from a wildcard
	  cerr << argv[i] << " " << drutil_age_message(rv) << "\n";
	  errors++;
	}
	continue;
      }
      if(pick.is_null() || (newest && m > pick_time) ||
	 (oldest && m < pick_time)) {
	pick = argv[i];
	pick_time = m;
      }
    }
    if(pick.is_null()) return 1;
    cout << pick.c_str() << "\n" << flush;
    return errors ? 1 : 0;
  }

  cout << "Current system time: " << systime.MakeFileModTime(now) << "\n";
  cout << "Current system GMT : " << systime.MakeGMTDateTime(now) << "\n";

  for(int i = narg; i < argc; i++) {
    const char *fname = argv[i];
    cout << "\n";
    cout << "Reading time stamps for: " << fname << "\n";
    time_t a, m, c;
    double seconds = 0;
    int rv = drutil_file_age(fname, seconds, a, m, c);
    if(rv != DRUTIL_AGE_OK && rv != DRUTIL_AGE_FUTURE) {
      cout << fname << " " << drutil_age_message(rv) << "\n" << flush;
      errors++;
      continue;
    }
    cout << "File access time: " << systime.MakeFileModTime(a) << "\n";
    cout << "File access GMT : " << systime.MakeGMTDateTime(a) << "\n";
    cout << "File modification time: " << systime.MakeFileModTime(m)
	 << "\n";
    cout << "File modification GMT : " << systime.MakeGMTDateTime(m)
	 << "\n";
#if defined (__WIN32__)
    cout << "File creation time: " << systime.MakeFileModTime(c) << "\n";
    cout << "File creation GMT : " << systime.MakeGMTDateTime(c) << "\n";
#else
    cout << "File change time (ctime): " << systime.MakeFileModTime(c)
	 << "\n";
    cout << "File change GMT  (ctime): " << systime.MakeGMTDateTime(c)
	 << "\n";
#endif
    if(rv == DRUTIL_AGE_FUTURE) {
      cout << "The file timestamp is in the future" << "\n";
    }
    else {
      PrintAge(seconds);
    }
    cout << flush;
  }
  return errors ? 1 : 0;
}
// Every file's modification time and age as DRSM lines (whatever
// -n or -o printed): values for the newest and oldest, a row per file
int HookFinish(int rc)
{
  int files = 0, errors = 0;
  gxString newest, oldest;
  double newest_age = 0, oldest_age = 0;
  char buf[64];
  hook_table.Columns();
  hook_table.Cell("File"); hook_table.Cell("Modified");
  hook_table.Cell("Age hours"); hook_table.Cell("Status");
  for(int i = hook_first; i < hook_argc; i++) {
    const char *fname = hook_argv[i];
    time_t a, m, c;
    double seconds = 0;
    int rv = drutil_file_age(fname, seconds, a, m, c);
    if(rv == DRUTIL_AGE_NOT_FILE) continue; // Directories from a wildcard
    hook_table.Row();
    hook_table.Cell(fname);
    if(rv != DRUTIL_AGE_OK && rv != DRUTIL_AGE_FUTURE) {
      errors++;
      hook_table.Cell(""); hook_table.Cell("");
      hook_table.Cell(drutil_age_message(rv));
      continue;
    }
    files++;
    double hours = rv == DRUTIL_AGE_FUTURE ? 0 : seconds / 3600.0;
    sprintf(buf, "%.2f", hours);
    char when[64];
    strftime(when, sizeof(when), "%Y-%m-%d %H:%M:%S", localtime(&m));
    hook_table.Cell(when);
    hook_table.Cell(buf);
    hook_table.Cell(rv == DRUTIL_AGE_FUTURE ? "in the future" : "ok");
    if(newest.is_null() || hours < newest_age) {
      newest = fname; newest_age = hours;
    }
    if(oldest.is_null() || hours > oldest_age) {
      oldest = fname; oldest_age = hours;
    }
  }
  gxString info;
  info << "filetimes: " << files << " file(s)";
  if(errors) info << ", " << errors << " cannot be read";
  if(files) {
    sprintf(buf, "%.2f", newest_age);
    info << ", newest " << newest << " (" << buf << " hours)";
    sprintf(buf, "%.2f", oldest_age);
    info << ", oldest " << oldest << " (" << buf << " hours)";
  }
  hook_summary.Info(info.c_str());
  drhook_head(hook_summary, ProgramName, VersionString);
  hook_summary.Value("files", (double)files);
  hook_summary.Value("errors", (double)errors);
  if(files) {
    hook_summary.Value("newest_file", newest.c_str());
    sprintf(buf, "%.2f", newest_age);
    hook_summary.Value("newest_age_hours", buf);
    hook_summary.Value("oldest_file", oldest.c_str());
    sprintf(buf, "%.2f", oldest_age);
    hook_summary.Value("oldest_age_hours", buf);
  }
  return drhook_tool_finish(hook, hook_tee, ProgramName, 0,
			    hook_summary, hook_table);
}

int main(int argc, char **argv)
{
  argc = drhook_args(hook, argc, argv);
  if(!hook.Check()) return 1;
  if(hook.Wanted()) hook_tee.Start();
  int rc = RunTool(argc, argv);
  hook_tee.Stop();
  if(!hook.Wanted() || !hook_ready) return rc;
  if(!HookFinish(rc)) return 1;
  return rc;
}
// ----------------------------------------------------------- //
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
