// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Source Code File Name: file_purge.cpp
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

Delete (or move) files whose modification time is a given number of
hours or days old or older. A small cron or Task Scheduler cleaner
for log, spool and temporary folders.

Every file gets one line:
YES - name purged (n) hours        the file was deleted or moved
NOT - name not purged (n) hours    too new, or an error
ERR - name <reason>                missing, not a file, bad timestamp
LST - name (n) hours (n) days      -l listing
DRY - name would be purged ...     -n dry run, nothing changed

Changes:
11/18/2026: dr_utils version, based on Douglas Gaer's 2005
file_purge program (version 1.5), with the same switches. New: -n
dry run. Fixed: ages under one hour or one day were counted as zero
(-h0.5 never purged a 40 minute old file, -d0.5 never purged a 20
hour old file). Switches are read once, in any order. Moving across
file systems (copy, then delete) works on Windows too.
*/
// ----------------------------------------------------------- //
#include "gxdlcode.h"

#include <iostream>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "dfileb.h"
#include "drutil.h"

const char *ProgramName = "file_purge";
const char *VersionString = DR_UTILS_VERSION;

// Settings from the command line
int del_hours = 0;
int del_days = 0;
double num_hours = 0;
double num_days = 0;
int del_exists = 0;
int list_file = 0;
int move_file = 0;
int dry_run = 0;
gxString move_dir;
int verbose = 0;

#if defined (__WIN32__)
const char path_sep = '\\';
#else
const char path_sep = '/';
#endif

void HelpMessage(const char *pname)
{
  if(!pname) pname = ProgramName;
  cout << "\n";
  cout << ProgramName << " version " << VersionString
       << " (dr_utils, DataReel 6.4.1)" << "\n";
  cout << "\n";
  cout << "Usage: " << pname << " [switches] file [file ...]" << "\n";
  cout << "file = name of input file to purge (wildcards allowed)" << "\n";
  cout << "Switches:  -?      = Display this help message." << "\n";
  cout << "           -e      = Purge file if it exists without time check."
       << "\n";
  cout << "           -l      = List hours old and days old without purging."
       << "\n";
  cout << "           -d[n]   = Purge by number of day(s)." << "\n";
  cout << "           -h[n]   = Purge by number of hours(s)." << "\n";
  cout << "           -m\"Dir\" = Move file to specified directory if days or "
       << "hours old." << "\n";
  cout << "           -n      = Dry run: print what would be done, change "
       << "nothing." << "\n";
  cout << "           -v      = Turn on verbose output mode." << "\n";
  cout << "\n";
  cout << "This purge program will delete a file if the file mod timestamp "
       << "is equal to" << "\n"
       << "or greater than the number of days or hours specified." << "\n";
  cout << "Number of hours or days can equal 0.5, 1, 2.5, 36, etc." << "\n";
  cout << "Directories are never purged." << "\n";
  cout << "Example 1: " << pname << " -h7 datafile*.log" << "\n";
  cout << "           Delete all datafile logs 7 hours old or older" << "\n";
#if defined (__WIN32__)
  cout << "Example 2: " << pname << " -m\"C:\\temp\" -h7 datafile*.log"
       << "\n";
#else
  cout << "Example 2: " << pname << " -m\"/tmp\" -h7 datafile*.log" << "\n";
#endif
  cout << "           Move all datafile logs 7 hours old or older to a temp "
       << "directory" << "\n";
  cout << "Example 3: " << pname << " -n -d30 *.log" << "\n";
  cout << "           Show which logs 30 days old or older would be deleted"
       << "\n";
  cout << "\n" << flush;
}

// Returns 1 if the switch is valid
int ProcessArg(const char *arg)
{
  char sw = arg[1];
  switch(sw) {
    case '?' :
      HelpMessage(0);
      exit(0);
    case 'l':
      list_file = 1;
      break;
    case 'e':
      del_exists = 1;
      break;
    case 'n':
      if(arg[2] != 0) break; // Not -n
      dry_run = 1;
      break;
    case 'm':
      move_file = 1;
      move_dir = &arg[2];
      if(move_dir.is_null()) {
	cout << "\n" << "You must specify a directory after -m" << "\n";
	return 0;
      }
      if(!futils_exists(move_dir.c_str())) {
	cout << "\n" << "Move directory does not exist:" << "\n";
	cout << move_dir.c_str() << "\n";
	return 0;
      }
      if(!futils_isdirectory(move_dir.c_str())) {
	cout << "\n" << "Move directory specified is a file:" << "\n";
	cout << move_dir.c_str() << "\n";
	return 0;
      }
      break;
    case 'h' :
      if(!drutil_parse_amount(&arg[2], num_hours)) {
	cout << "\n" << "Invalid number following switch " << arg << "\n";
	cout << "You must specify a valid number of hours" << "\n";
	return 0;
      }
      del_hours = 1;
      break;
    case 'd':
      if(!drutil_parse_amount(&arg[2], num_days)) {
	cout << "\n" << "Invalid number following switch " << arg << "\n";
	cout << "You must specify a valid number of days" << "\n";
	return 0;
      }
      del_days = 1;
      break;
    case 'v':
      verbose = 1;
      break;
    default:
      cout << "\n" << "Unknown switch " << arg << "\n";
      return 0;
  }
  if(sw == 'n' && arg[2] != 0) {
    cout << "\n" << "Unknown switch " << arg << "\n";
    return 0;
  }
  return 1;
}

// Copy, then delete, for a move to another file system
int CopyAndDelete(const gxString &fname, const gxString &newname)
{
  DiskFileB dfile;
  if(dfile.df_copy(fname.c_str(), newname.c_str()) !=
     DiskFileB::df_NO_ERROR) {
    if(verbose) cout << "Error copying file during move" << "\n";
    return 0;
  }
  if(futils_remove(fname.c_str()) != 0) {
    if(verbose) cout << "Error deleting original file during move" << "\n";
    return 0;
  }
  return 1;
}

int MoveFile(const gxString &fname, const gxString &newname,
	     const char *age_str)
{
  if(dry_run) {
    cout << "DRY - " << fname.c_str() << " would be moved " << age_str
	 << " to " << newname.c_str() << "\n";
    return 1;
  }
  if(futils_exists(newname.c_str())) {
    cout << "NOT - " << fname.c_str() << " not moved, " << newname.c_str()
	 << " exists" << "\n";
    return 0;
  }
  int moved = (futils_rename(fname.c_str(), newname.c_str()) == 0);
  if(!moved) {
#if defined (__WIN32__)
    int other_fs = 1; // MoveFile across volumes fails, try a copy
#else
    int other_fs = (errno == EXDEV);
#endif
    if(other_fs) {
      if(verbose) {
	cout << "Rename failed, moving with a copy followed by delete"
	     << "\n";
      }
      moved = CopyAndDelete(fname, newname);
    }
  }
  if(moved) {
    cout << "YES - " << fname.c_str() << " moved " << age_str << " to "
	 << newname.c_str() << "\n";
    return 1;
  }
  cout << "NOT - " << fname.c_str() << " error moving file" << "\n";
  if(verbose) cout << "Error: " << strerror(errno) << "\n";
  return 0;
}

int DeleteFile(const gxString &fname, const char *age_str)
{
  if(dry_run) {
    cout << "DRY - " << fname.c_str() << " would be purged " << age_str
	 << "\n";
    return 1;
  }
  if(futils_remove(fname.c_str()) == 0) {
    cout << "YES - " << fname.c_str() << " purged " << age_str << "\n";
    return 1;
  }
  cout << "NOT - " << fname.c_str() << " error deleting file" << "\n";
  if(verbose) cout << "Error: " << strerror(errno) << "\n";
  return 0;
}

int PurgeFile(const gxString &fname)
{
  if(verbose && !move_file) {
    cout << "\n" << "Purging file: " << fname.c_str() << "\n";
  }

  double seconds = 0;
  int rv = drutil_file_age(fname.c_str(), seconds);
  if(rv != DRUTIL_AGE_OK) {
    if(del_exists && rv == DRUTIL_AGE_FUTURE) {
      rv = DRUTIL_AGE_OK; // -e does not look at the time
    }
    else {
      cout << "ERR - " << fname.c_str() << " " << drutil_age_message(rv)
	   << "\n";
      return 0;
    }
  }
  double hours = seconds / 3600.0;
  double days = seconds / 86400.0;
  char age_str[128];

  if(list_file) {
    sprintf(age_str, "(%.2f) hours (%.2f) days", hours, days);
    cout << "LST - " << fname.c_str() << " " << age_str << "\n";
    return 1;
  }

  gxString newname;
  if(move_file) {
    gxString base = fname;
    // The file's own name, without its folder
    gxString tmp = base;
#if defined (__WIN32__)
    tmp.ReplaceString("/", "\\");
#endif
    char sep[2] = { path_sep, 0 };
    if(tmp.Find(sep) != -1) tmp.DeleteBeforeLastIncluding(sep);
    base = tmp;
    newname = move_dir;
    if(newname[newname.length()-1] != path_sep &&
       newname[newname.length()-1] != '/') {
      newname << path_sep;
    }
    newname << base;
    if(verbose) {
      cout << "\n" << "Moving file: " << fname.c_str() << "\n";
      cout << "To: " << newname.c_str() << "\n";
    }
  }

  if(del_exists) {
    sprintf(age_str, "(existing file)");
    if(move_file) return MoveFile(fname, newname, age_str);
    if(dry_run) {
      cout << "DRY - " << fname.c_str() << " existing file would be purged"
	   << "\n";
      return 1;
    }
    if(futils_remove(fname.c_str()) == 0) {
      cout << "YES - " << fname.c_str() << " existing file purged" << "\n";
      return 1;
    }
    cout << "NOT - " << fname.c_str() << " error deleting existing file"
	 << "\n";
    return 0;
  }

  if(del_hours) {
    sprintf(age_str, "(%.2f) hours", hours);
    if(hours >= num_hours) {
      return move_file ? MoveFile(fname, newname, age_str) :
	DeleteFile(fname, age_str);
    }
    cout << "NOT - " << fname.c_str() << " not purged " << age_str << "\n";
    return 0;
  }
  if(del_days) {
    sprintf(age_str, "(%.2f) days", days);
    if(days >= num_days) {
      return move_file ? MoveFile(fname, newname, age_str) :
	DeleteFile(fname, age_str);
    }
    cout << "NOT - " << fname.c_str() << " not purged " << age_str << "\n";
    return 0;
  }
  cout << "NOT - " << fname.c_str() << " error no operation specified"
       << "\n";
  return 0;
}

int main(int argc, char **argv)
{
  if(argc < 2) {
    HelpMessage(argv[0]);
    return 1;
  }

  // Switches first, in any position
  int i;
  for(i = 1; i < argc; i++) {
    if(argv[i][0] == '-' && argv[i][1] != 0) {
      if(!ProcessArg(argv[i])) {
	cout << "Exiting..." << "\n" << "\n" << flush;
	return 1;
      }
    }
  }
  if(del_hours && del_days) {
    cout << "\n" << "You cannot combine the -h and -d switches" << "\n";
    cout << "Exiting..." << "\n" << "\n" << flush;
    return 1;
  }
  if(list_file) { // -l never changes anything
    del_exists = del_hours = del_days = move_file = 0;
  }
  else if(del_exists) {
    del_hours = del_days = 0;
  }
  else if(!del_hours && !del_days) {
    cout << "\n" << "You must specify -h, -d, -e or -l" << "\n";
    cout << "Exiting..." << "\n" << "\n" << flush;
    return 1;
  }

  int num_files = 0;
  for(i = 1; i < argc; i++) {
    if(argv[i][0] == '-' && argv[i][1] != 0) continue;
    if(futils_isdirectory(argv[i])) continue; // Never directories
    gxString fname = argv[i];
    num_files++;
    PurgeFile(fname);
  }
  cout << flush;

  if(num_files == 0) {
    cout << "\n" << "You must enter a file name." << "\n";
    cout << "Exiting..." << "\n" << "\n" << flush;
    return 1;
  }
  return 0;
}
// ----------------------------------------------------------- //
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
