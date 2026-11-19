// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Source Code File Name: file_hours.cpp
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

Tell the caller if a file's modification time is a given number of
hours old or older: prints YES, NOT or ERR. A stale-file check for
scripts and cron jobs (a feed that stopped writing, a log that is
no longer updated).

Changes:
11/18/2026: dr_utils version, based on work from the 2005 file_hours
program. Fractional hours work below one hour (0.5 is 30 minutes;
the 2005 version said NOT for any file under one hour old). Output
ends with a new line. -s sets the exit status from the answer.
*/
// ----------------------------------------------------------- //
#include "gxdlcode.h"

#include <iostream>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drutil.h"

const char *ProgramName = "file_hours";
const char *VersionString = DR_UTILS_VERSION;

void HelpMessage(const char *pname)
{
  cout << ProgramName << " version " << VersionString
       << " (dr_utils, DataReel 6.4.1)" << "\n";
  cout << "\n";
  cout << "Usage: " << pname << " [-s] filename numhours" << "\n";
  cout << "filename = name of input file" << "\n";
  cout << "numhours = number of hours (0.5, 1, 2.5, 36, etc)" << "\n";
  cout << "-s       = exit status 0 for YES, 1 for NOT, 2 for ERR" << "\n";
  cout << "\n";
  cout << "Prints YES if the file mod timestamp is equal to or greater"
       << "\n" << "than numhours old, NOT if it is newer, and ERR - <reason>"
       << "\n" << "if the file cannot be checked. Without -s the exit"
       << "\n" << "status is 0 for YES or NOT and 1 for ERR." << "\n";
  cout << "\n" << flush;
}

int main(int argc, char **argv)
{
  int use_status = 0;
  int narg = 1;
  if(narg < argc && strcmp(argv[narg], "-s") == 0) {
    use_status = 1;
    narg++;
  }
  if(argc - narg != 2) {
    HelpMessage(argv[0]);
    return use_status ? 2 : 1;
  }

  const char *fname = argv[narg];
  double numhours = 0;
  if(!drutil_parse_amount(argv[narg+1], numhours)) {
    cout << "ERR - " << argv[narg+1] << " is not a number of hours"
	 << "\n" << flush;
    return use_status ? 2 : 1;
  }

  double seconds = 0;
  int rv = drutil_file_age(fname, seconds);
  if(rv != DRUTIL_AGE_OK) {
    cout << "ERR - " << fname << " " << drutil_age_message(rv) << "\n"
	 << flush;
    return use_status ? 2 : 1;
  }

  if((seconds / 3600.0) >= numhours) {
    cout << "YES" << "\n" << flush;
    return 0;
  }
  cout << "NOT" << "\n" << flush;
  return use_status ? 1 : 0;
}
// ----------------------------------------------------------- //
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
