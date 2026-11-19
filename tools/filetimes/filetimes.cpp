// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Source Code File Name: filetimes.cpp
// C++ Compiler Used: MSVC, GCC
// Produced By: DataReel Software Development Team
// File Creation Date: 09/27/2005
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

Print a file's access, modification and creation times in local
time and GMT, and how old the file is. With two or more files, -n
prints the name of the newest file and -o the oldest (build and feed
freshness checks).

Changes:
11/18/2026: dr_utils version, based on work from the 2005 filetimes
program. Several files at once, -n/-o to compare them, ages in
days/hours/minutes/seconds. On Linux "creation" is the inode change
time (ctime).
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

int main(int argc, char **argv)
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
// ----------------------------------------------------------- //
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
