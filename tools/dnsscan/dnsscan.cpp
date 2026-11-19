// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Source Code File Name: dnsscan.cpp
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

List the DNS names of the addresses in a class C (/24) range, one
tab-separated line per address: address, reverse DNS name, and with
-f whether the name resolves back to the same address (forward
match). With -p each address is pinged too. A quick check of a
reverse zone, or of which addresses in a range are named.

Changes:
11/18/2026: dr_utils version, based on work from the 1999-2007
dnsscan program (ping sweep with names and a.b.c.x-y ranges). The
ping is optional (-p) so a DNS listing needs no ICMP rights; -f
checks forward and reverse DNS agree; -a lists unnamed addresses.
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

const char *ProgramName = "dnsscan";
const char *VersionString = DR_UTILS_VERSION;

void HelpMessage(const char *pname)
{
  cout << ProgramName << " version " << VersionString
       << " (dr_utils, DataReel 6.4.1)" << "\n";
  cout << "\n";
  cout << "Usage: " << pname << " [-a] [-f] [-p] [-t ms] range" << "\n";
  cout << "range = a.b.c (hosts .1 to .254), a.b.c.d or a.b.c.x-y" << "\n";
  cout << "-a    = also list addresses with no DNS name" << "\n";
  cout << "-f    = check that each name resolves back to the address"
       << "\n";
  cout << "-p    = ping each address too (up or down)" << "\n";
  cout << "-t ms = ping timeout in milliseconds (default 1000)" << "\n";
  cout << "\n";
  cout << "Output, tab separated: address name [forward] [ping]" << "\n";
  cout << "  name    = the reverse DNS name, or no_DNS_entry" << "\n";
  cout << "  forward = fwd_ok, or fwd_mismatch with the address it "
       << "resolves to" << "\n";
  cout << "  ping    = up (ms) or down" << "\n";
  cout << "\n";
  cout << "Example: " << pname << " -f 192.0.2" << "\n";
  cout << "Exit status: 0 if any address has a name, 1 if none, 2 on an"
       << " error." << "\n";
  cout << "\n" << flush;
  drhook_usage(ProgramName);
}

// Dashboard output (--drsm, --submit, see common/drhook.h): set
// hook_ready once the arguments are good, so a usage error sends
// nothing
drhookOptions hook("dnsscan");
drhookTee hook_tee;
drhookLines hook_summary;
drhookLines hook_table;
int hook_ready = 0;
gxString hook_range;
int hook_fwd = 0, hook_ping = 0;
int hook_tried = 0, hook_named = 0, hook_up = 0;
int hook_fwd_ok = 0, hook_fwd_bad = 0;

int RunTool(int argc, char **argv)
{
  int all = 0, fwd = 0, ping = 0;
  int timeout_ms = 1000;
  const char *range = 0;
  for(int i = 1; i < argc; i++) {
    if(strcmp(argv[i], "-a") == 0) all = 1;
    else if(strcmp(argv[i], "-f") == 0) fwd = 1;
    else if(strcmp(argv[i], "-p") == 0) ping = 1;
    else if(strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
      timeout_ms = atoi(argv[++i]);
      if(timeout_ms < 1 || timeout_ms > 60000) {
	cout << "The timeout must be 1 to 60000 ms" << "\n" << flush;
	return 2;
      }
    }
    else if(argv[i][0] == '-' || range) {
      HelpMessage(argv[0]);
      return 2;
    }
    else range = argv[i];
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
  hook_fwd = fwd;
  hook_ping = ping;
  hook_table.Columns();
  hook_table.Cell("Address"); hook_table.Cell("Name");
  if(fwd) hook_table.Cell("Forward");
  if(ping) hook_table.Cell("Ping");
  int named = 0;
  for(int h = first; h <= last; h++) {
    gxString ip;
    ip << clear << base << "." << h;
    gxString name;
    int has_name = drnet_reverse(ip.c_str(), name);
    if(has_name) named++;
    hook_tried++;
    hook_named = named;

    gxString ping_str;
    if(ping) {
      double rtt = 0;
      int rv = drnet_ping(ip.c_str(), timeout_ms, rtt, err);
      if(rv == DRNET_ERROR) {
	cout << ip.c_str() << ": " << err.c_str() << "\n" << flush;
	return 2;
      }
      if(rv == DRNET_REPLY) {
	char buf[64];
	sprintf(buf, "up (%.1f ms)", rtt);
	hook_up++;
	ping_str = buf;
      }
      else {
	ping_str = "down";
      }
    }
    if(!has_name && !all && !(ping && ping_str != "down")) continue;

    gxString fwd_str = "-";
    cout << ip.c_str() << "\t" << (has_name ? name.c_str() : "no_DNS_entry");
    if(fwd && has_name) {
      gxString back, rerr;
      if(drnet_resolve(name.c_str(), back, rerr) && back == ip) {
	cout << "\t" << "fwd_ok";
	fwd_str = "fwd_ok";
	hook_fwd_ok++;
      }
      else if(back.is_null()) {
	cout << "\t" << "fwd_mismatch (does not resolve)";
	fwd_str = "fwd_mismatch (does not resolve)";
	hook_fwd_bad++;
      }
      else {
	cout << "\t" << "fwd_mismatch (" << back.c_str() << ")";
	fwd_str << clear << "fwd_mismatch (" << back << ")";
	hook_fwd_bad++;
      }
    }
    else if(fwd) {
      cout << "\t" << "-";
    }
    if(ping) cout << "\t" << ping_str.c_str();
    hook_table.Row();
    hook_table.Cell(ip.c_str());
    hook_table.Cell(has_name ? name.c_str() : "");
    if(fwd) hook_table.Cell(fwd_str.c_str());
    if(ping) hook_table.Cell(ping_str.c_str());
    cout << "\n" << flush;
  }
  return named ? 0 : 1;
}
// The scan as DRSM lines: names, forward checks, pings, a row per
// address listed
int HookFinish(int rc)
{
  gxString info;
  info << "dnsscan " << hook_range << ": " << hook_named << " of "
       << hook_tried << " addresses have a DNS name";
  if(hook_fwd) info << ", " << hook_fwd_bad << " forward mismatch(es)";
  if(hook_ping) info << ", " << hook_up << " answered a ping";
  hook_summary.Info(info.c_str());
  drhook_head(hook_summary, ProgramName, VersionString);
  hook_summary.Value("range", hook_range.c_str());
  hook_summary.Value("tried", (double)hook_tried);
  hook_summary.Value("named", (double)hook_named);
  if(hook_fwd) {
    hook_summary.Value("fwd_ok", (double)hook_fwd_ok);
    hook_summary.Value("fwd_mismatch", (double)hook_fwd_bad);
  }
  if(hook_ping) hook_summary.Value("up", (double)hook_up);
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
