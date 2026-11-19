// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Source Code File Name: file_hours.cpp
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

Tell the caller if a file's modification time is a given number of
hours old or older: prints YES, NOT or ERR. A stale-file check for
scripts and cron jobs (a feed that stopped writing, a log that is
no longer updated).

Changes:
11/18/2026: dr_utils version, based on work from the 2005 file_hours
program. Fractional hours work below one hour (0.5 is 30 minutes;
the 2005 version said NOT for any file under one hour old). Output
ends with a new line. -s sets the exit status from the answer.
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
#include "drutil.h"
#include "drhook.h"

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
  drhook_usage(ProgramName);
}

// Dashboard output (--drsm, --submit, see common/drhook.h): set
// hook_ready once the arguments are good, so a usage error sends
// nothing
drhookOptions hook("file_hours");
drhookTee hook_tee;
drhookLines hook_summary;
drhookLines hook_table;
int hook_ready = 0;
gxString hook_file, hook_result;
double hook_limit = 0, hook_age = 0;

int RunTool(int argc, char **argv)
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

  hook_ready = 1;
  hook_file = fname;
  hook_limit = numhours;
  double seconds = 0;
  int rv = drutil_file_age(fname, seconds);
  hook_age = seconds / 3600.0;
  if(rv != DRUTIL_AGE_OK) {
    hook_result = "ERR";
    cout << "ERR - " << fname << " " << drutil_age_message(rv) << "\n"
	 << flush;
    return use_status ? 2 : 1;
  }

  if((seconds / 3600.0) >= numhours) {
    hook_result = "YES";
    cout << "YES" << "\n" << flush;
    return 0;
  }
  hook_result = "NOT";
  cout << "NOT" << "\n" << flush;
  return use_status ? 1 : 0;
}
// The check as DRSM lines: stale is 1 when the file is the limit
// old or older (YES); a file that cannot be checked is an ERROR
int HookFinish(int rc)
{
  int err = hook_result == "ERR";
  char buf[64];
  gxString info;
  info << "file_hours " << hook_file << ": ";
  if(err) {
    info << "cannot be checked";
  }
  else {
    sprintf(buf, "%.2f", hook_age);
    info << buf << " hours old, limit ";
    info << drhook_num(hook_limit) << " hours: " << hook_result;
  }
  hook_summary.Info(info.c_str());
  drhook_head(hook_summary, ProgramName, VersionString);
  hook_summary.Value("file", hook_file.c_str());
  hook_summary.Value("result", hook_result.c_str());
  hook_summary.Value("limit_hours", hook_limit);
  if(!err) {
    sprintf(buf, "%.2f", hook_age);
    hook_summary.Value("age_hours", buf);
    hook_summary.Value("stale", hook_result == "YES" ? "1" : "0");
  }
  hook_table.Columns();
  hook_table.Cell("File"); hook_table.Cell("Age hours");
  hook_table.Cell("Limit hours"); hook_table.Cell("Result");
  hook_table.Row();
  hook_table.Cell(hook_file.c_str());
  sprintf(buf, "%.2f", hook_age);
  hook_table.Cell(err ? "" : buf);
  hook_table.Cell(hook_limit);
  hook_table.Cell(hook_result.c_str());
  return drhook_tool_finish(hook, hook_tee, ProgramName, err,
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
