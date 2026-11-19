// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Header File Name: drutil.h
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

Small helpers shared by the dr_utils file tools (file_hours,
file_purge, filetimes): the dr_utils release string and a file's age.

Changes:
11/18/2026: First version for dr_utils.
*/
// ----------------------------------------------------------- //
#ifndef __DR_UTILS_UTIL_HPP__
#define __DR_UTILS_UTIL_HPP__

#include "gxdlcode.h"

#include <stdlib.h>
#include <time.h>
#include "gxstring.h"
#include "futils.h"

// dr_utils release, printed by every tool's help and version output
#define DR_UTILS_VERSION "1.2"

// File age results
const int DRUTIL_AGE_OK = 0;
const int DRUTIL_AGE_MISSING = 1;   // The file does not exist
const int DRUTIL_AGE_NOT_FILE = 2;  // A directory or other non-file
const int DRUTIL_AGE_READ_ERR = 3;  // Could not read the timestamps
const int DRUTIL_AGE_FUTURE = 4;    // Modified in the future

// The age of a file's modification time, in seconds. Sets the three
// timestamps too. Returns one of the DRUTIL_AGE values.
inline int drutil_file_age(const char *fname, double &seconds,
			   time_t &access_time, time_t &mod_time,
			   time_t &create_time)
{
  seconds = 0;
  if(!futils_exists(fname)) return DRUTIL_AGE_MISSING;
  if(!futils_isfile(fname)) return DRUTIL_AGE_NOT_FILE;
  if(futils_timestamp(fname, access_time, mod_time, create_time) != 0) {
    return DRUTIL_AGE_READ_ERR;
  }
  time_t now;
  time(&now);
  if(mod_time > now) return DRUTIL_AGE_FUTURE;
  seconds = difftime(now, mod_time);
  return DRUTIL_AGE_OK;
}

inline int drutil_file_age(const char *fname, double &seconds)
{
  time_t a, m, c;
  return drutil_file_age(fname, seconds, a, m, c);
}

// A message for a DRUTIL_AGE value
inline const char *drutil_age_message(int rv)
{
  switch(rv) {
    case DRUTIL_AGE_OK: return "OK";
    case DRUTIL_AGE_MISSING: return "does not exist";
    case DRUTIL_AGE_NOT_FILE: return "is not a file name";
    case DRUTIL_AGE_READ_ERR: return "error reading file timestamp";
    case DRUTIL_AGE_FUTURE: return "file timestamp is in the future";
    default: break;
  }
  return "unknown error";
}

// Parse a positive number of hours or days ("1", "2.5", "36").
// Returns 0 if "s" is not a number greater than zero.
inline int drutil_parse_amount(const char *s, double &value)
{
  if(!s || !*s) return 0;
  char *end = 0;
  value = strtod(s, &end);
  if(!end || *end != 0) return 0;
  return value > 0;
}

#endif // __DR_UTILS_UTIL_HPP__
// ----------------------------------------------------------- //
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
