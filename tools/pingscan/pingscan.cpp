// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Source Code File Name: pingscan.cpp
// C++ Compiler Used: MSVC, GCC
// Produced By: DataReel Software Development Team
// File Creation Date: 1999
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

Ping sweep of one class C (/24) network or part of one, with each
host's DNS name. For your own networks only.

Changes:
11/18/2026: dr_utils version, based on work from the 1999-2010
pingscan program. Address ranges (a.b.c.x-y), a timeout switch, a
round trip time per reply, -u (only hosts that answer) and a summary
line. No root on Linux when net.ipv4.ping_group_range allows ICMP
datagram sockets; no administrator on Windows (IcmpSendEcho). One
failed ping no longer stops the sweep.
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
#include "drnet.h"
#include "drutil.h"
#include "drhook.h"

const char *ProgramName = "pingscan";
const char *VersionString = DR_UTILS_VERSION;

void HelpMessage(const char *pname)
{
  cout << ProgramName << " version " << VersionString
       << " (dr_utils, DataReel 6.4.1)" << "\n";
  cout << "\n";
  cout << "Usage: " << pname << " [-t ms] [-u] range" << "\n";
  cout << "range = a.b.c (hosts .1 to .254), a.b.c.d or a.b.c.x-y" << "\n";
  cout << "-t ms = ping timeout in milliseconds (default 1000)" << "\n";
  cout << "-u    = list only the hosts that answer" << "\n";
  cout << "\n";
  cout << "Example: " << pname << " 192.0.2" << "\n";
  cout << "Example: " << pname << " -u -t 300 192.0.2.10-40" << "\n";
  cout << "\n";
  cout << "Exit status: 0 if any host answered, 1 if none did, 2 on an"
       << "\n" << "error. Scan only networks you own or are allowed to scan."
       << "\n";
  cout << "\n" << flush;
  drhook_usage(ProgramName);
}

// Dashboard output (--drsm, --submit, see common/drhook.h): set
// hook_ready once the arguments are good, so a usage error sends
// nothing
drhookOptions hook("pingscan");
drhookTee hook_tee;
drhookLines hook_summary;
drhookLines hook_table;
int hook_ready = 0;
gxString hook_range;
int hook_tried = 0, hook_up = 0;

int RunTool(int argc, char **argv)
{
  int timeout_ms = 1000;
  int only_up = 0;
  const char *range = 0;
  for(int i = 1; i < argc; i++) {
    if(strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
      timeout_ms = atoi(argv[++i]);
      if(timeout_ms < 1 || timeout_ms > 60000) {
	cout << "The timeout must be 1 to 60000 ms" << "\n" << flush;
	return 2;
      }
    }
    else if(strcmp(argv[i], "-u") == 0) {
      only_up = 1;
    }
    else if(argv[i][0] == '-') {
      HelpMessage(argv[0]);
      return 2;
    }
    else if(!range) {
      range = argv[i];
    }
    else {
      HelpMessage(argv[0]);
      return 2;
    }
  }
  if(!range) {
    HelpMessage(argv[0]);
    return 2;
  }

  gxString base, err;
  int first, last;
  if(!drnet_parse_range(range, base, first, last, err)) {
    cout << err.c_str() << "\n" << flush;
    return 2;
  }
  if(!drnet_init()) {
    cout << "Could not start the socket library" << "\n" << flush;
    return 2;
  }

  hook_ready = 1;
  hook_range = range;
  hook_table.Columns();
  hook_table.Cell("Address"); hook_table.Cell("Name");
  hook_table.Cell("Status"); hook_table.Cell("Reply ms");
  cout << "Exploring " << base.c_str() << "." << first;
  if(last != first) cout << "-" << last;
  cout << "\n" << flush;

  int up = 0, tried = 0;
  for(int h = first; h <= last; h++) {
    gxString ip;
    ip << clear << base << "." << h;
    double rtt = 0;
    int rv = drnet_ping(ip.c_str(), timeout_ms, rtt, err);
    if(rv == DRNET_ERROR) {
      cout << ip.c_str() << ": " << err.c_str() << "\n" << flush;
      return 2; // The same error (no ICMP socket) would repeat
    }
    tried++;
    gxString name;
    int has_name = drnet_reverse(ip.c_str(), name);
    if(rv == DRNET_REPLY) up++;
    hook_tried = tried;
    hook_up = up;
    if(!only_up || rv == DRNET_REPLY) {
      char ms[32];
      sprintf(ms, "%.1f", rtt);
      hook_table.Row();
      hook_table.Cell(ip.c_str());
      hook_table.Cell(has_name ? name.c_str() : "");
      hook_table.Cell(rv == DRNET_REPLY ? "up" :
		      (has_name ? "not responding" : "no host"));
      hook_table.Cell(rv == DRNET_REPLY ? ms : "");
    }
    if(only_up && rv != DRNET_REPLY) continue;

    cout << "\n" << "Contacting host: " << ip.c_str() << "\n";
    if(has_name) cout << "Hostname: " << name.c_str() << "\n";
    if(rv == DRNET_REPLY) {
      char buf[64];
      sprintf(buf, "%.1f", rtt);
      if(!has_name) cout << "Host name: Cannot resolve host name" << "\n";
      cout << "Host is up, reply in " << buf << " ms" << "\n";
    }
    else if(has_name) {
      cout << "Host not responding." << "\n";
    }
    else {
      cout << "Host does not exist." << "\n";
    }
    cout << flush;
  }
  cout << "\n" << up << " of " << tried << " hosts answered" << "\n"
       << flush;
  return up ? 0 : 1;
}
// The scan as DRSM lines: hosts up and down, a row per host
int HookFinish(int rc)
{
  gxString info;
  info << "pingscan " << hook_range << ": " << hook_up << " of "
       << hook_tried << " hosts answered";
  hook_summary.Info(info.c_str());
  drhook_head(hook_summary, ProgramName, VersionString);
  hook_summary.Value("range", hook_range.c_str());
  hook_summary.Value("tried", (double)hook_tried);
  hook_summary.Value("up", (double)hook_up);
  hook_summary.Value("down", (double)(hook_tried - hook_up));
  return drhook_tool_finish(hook, hook_tee, ProgramName, rc == 2,
			    hook_summary, hook_table);
}

int main(int argc, char **argv)
{
  argc = drhook_args(hook, argc, argv);
  if(!hook.Check()) return 2;
  if(hook.Wanted()) hook_tee.Start();
  int rc = RunTool(argc, argv);
  hook_tee.Stop();
  if(!hook.Wanted() || !hook_ready) return rc;
  if(!HookFinish(rc)) return 2;
  return rc;
}
// ----------------------------------------------------------- //
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
