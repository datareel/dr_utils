// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Source Code File Name: getmac.cpp
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

Print the MAC (Media Access Control) address of one host on a local
network. The host is pinged first so the system has its ARP entry.

Note: Windows has its own getmac.exe (the MAC addresses of the local
adapters). This one finds the MAC address of another host; use the
full path or rename it if both are on the PATH.

Changes:
11/18/2026: dr_utils version, based on work from the 1999-2004
getmac program (Linux only, root, SIOCGARP). Now Linux and Windows,
no root for the lookup (/proc/net/arp or SendARP); the interface
name is optional.
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

const char *ProgramName = "getmac";
const char *VersionString = DR_UTILS_VERSION;

void HelpMessage(const char *pname)
{
  cout << ProgramName << " version " << VersionString
       << " (dr_utils, DataReel 6.4.1)" << "\n";
  cout << "\n";
  cout << "Usage: " << pname << " hostname [ethernet_device]" << "\n";
  cout << "ethernet_device = only an ARP entry on this interface (Linux)"
       << "\n";
  cout << "Example: " << pname << " 192.0.2.12" << "\n";
  cout << "Example: " << pname << " printer.example.com eth0" << "\n";
  cout << "\n";
  cout << "Exit status: 0 with the MAC address, 1 if there is no ARP entry"
       << "\n" << "(the host did not answer or is not on a local network), "
       << "2 on an error." << "\n";
  cout << "\n" << flush;
  drhook_usage(ProgramName);
}

// Dashboard output (--drsm, --submit, see common/drhook.h): set
// hook_ready once the arguments are good, so a usage error sends
// nothing
drhookOptions hook("getmac");
drhookTee hook_tee;
drhookLines hook_summary;
drhookLines hook_table;
int hook_ready = 0;
gxString hook_host, hook_ip, hook_ping, hook_mac;

int RunTool(int argc, char **argv)
{
  if(argc < 2 || argc > 3 || argv[1][0] == '-') {
    HelpMessage(argv[0]);
    return 2;
  }
  const char *hostname = argv[1];
  const char *dev = (argc == 3) ? argv[2] : 0;

  if(!drnet_init()) {
    cout << "Could not start the socket library" << "\n" << flush;
    return 2;
  }
  gxString ip, err;
  if(!drnet_resolve(hostname, ip, err)) {
    cout << err.c_str() << "\n" << flush;
    return 2;
  }

  hook_ready = 1;
  hook_host = hostname;
  hook_ip = ip;
  cout << "\n" << "Contacting host: " << hostname << "\n";
  gxString info;
  if(drnet_host_info(hostname, info)) cout << info.c_str();
  else cout << "Could not resolve hostname" << "\n";

  double rtt = 0;
  int rv = drnet_ping(ip.c_str(), 1000, rtt, err);
  hook_ping = rv == DRNET_REPLY ? "up" :
    (rv == DRNET_NO_REPLY ? "no reply" : "skipped");
  if(rv == DRNET_NO_REPLY) {
    cout << "Host not responding to ping." << "\n";
  }
  else if(rv == DRNET_ERROR) {
    cout << "Ping skipped: " << err.c_str() << "\n";
  }

  gxString mac;
  if(!drnet_mac_address(ip.c_str(), dev, mac)) {
    cout << "No entry in the arp cache" << "\n" << "\n" << flush;
    return 1;
  }
  cout << "MAC Address: " << mac.c_str() << "\n" << "\n" << flush;
  hook_mac = mac;
  return 0;
}
// The lookup as DRSM lines: found (1 or 0) and the MAC address
int HookFinish(int rc)
{
  gxString info;
  info << "getmac " << hook_host << " (" << hook_ip << "): ";
  if(hook_mac.is_null()) info << "no ARP entry";
  else info << "MAC address " << hook_mac;
  info << ", ping " << hook_ping;
  hook_summary.Info(info.c_str());
  drhook_head(hook_summary, ProgramName, VersionString);
  hook_summary.Value("host", hook_host.c_str());
  hook_summary.Value("address", hook_ip.c_str());
  hook_summary.Value("ping", hook_ping.c_str());
  hook_summary.Value("found", hook_mac.is_null() ? "0" : "1");
  hook_summary.Value("mac", hook_mac.c_str());
  hook_table.Columns();
  hook_table.Cell("Host"); hook_table.Cell("Address");
  hook_table.Cell("MAC address"); hook_table.Cell("Ping");
  hook_table.Row();
  hook_table.Cell(hook_host.c_str()); hook_table.Cell(hook_ip.c_str());
  hook_table.Cell(hook_mac.is_null() ? "no ARP entry" : hook_mac.c_str());
  hook_table.Cell(hook_ping.c_str());
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
