// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Source Code File Name: netpscan.cpp
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

Scan a list of IP addresses for open TCP ports: the ports named in a
services file, or a list given with -p. For each address: its DNS
name, a ping, and every open port with its service name. An audit of
your own hosts for unexpected listeners.

Changes:
11/18/2026: dr_utils version, based on work from the 1999-2004
netpscan program. A connect timeout (-t, default 500 ms), -p for a
port list, and by default only the services file ports below 1024
(-A for every TCP entry; a current /etc/services has thousands).
Each port is tried once even if the services file lists it twice.
Addresses that do not answer a ping are still scanned (-s skips
them). No root needed on Linux for the TCP scan.
11/19/2026: --drsm and --submit: results for the DRSM dashboard through
the DataReel Reporting Agents (hooks/README.md).
*/
// ----------------------------------------------------------- //
#include "gxdlcode.h"

#include <iostream>
#include <fstream>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drnet.h"
#include "drutil.h"
#include "drhook.h"

const char *ProgramName = "netpscan";
const char *VersionString = DR_UTILS_VERSION;

const int MaxServices = 20000;
static drnetService services_list[MaxServices];
static int scan_ports[65536];

void HelpMessage(const char *pname)
{
  cout << ProgramName << " version " << VersionString
       << " (dr_utils, DataReel 6.4.1)" << "\n";
  cout << "\n";
  cout << "Usage: " << pname << " [switches] iplist_file [services_file]"
       << "\n";
  cout << "iplist_file   = one IPv4 address or host name per line, # comments"
       << "\n";
  cout << "services_file = default " << drnet_default_services() << "\n";
  cout << "Switches: -p ports = scan these ports: 22,80,443 or 1-1024"
       << "\n";
  cout << "          -A       = every TCP port in the services file "
       << "(default: below 1024)" << "\n";
  cout << "          -t ms    = connect and ping timeout (default 500)" << "\n";
  cout << "          -s       = skip addresses that do not answer a ping"
       << "\n";
  cout << "\n";
  cout << "Example: " << pname << " iplist.txt" << "\n";
  cout << "Example: " << pname << " -p 22,80,443,3389 iplist.txt" << "\n";
  cout << "\n";
  cout << "Exit status: 0 when the scan ran, 2 on an error." << "\n";
  cout << "Scan only hosts you own or are allowed to scan." << "\n";
  cout << "\n" << flush;
  drhook_usage(ProgramName);
}

// Parse "22,80,443" or "1-1024" into scan_ports. Returns the count, or
// -1 on an error.
int ParsePorts(const char *spec)
{
  static char seen[65536];
  memset(seen, 0, sizeof(seen));
  int n = 0;
  const char *p = spec;
  while(*p) {
    char *end = 0;
    long a = strtol(p, &end, 10);
    if(end == p || a < 1 || a > 65535) return -1;
    long b = a;
    p = end;
    if(*p == '-') {
      p++;
      b = strtol(p, &end, 10);
      if(end == p || b < a || b > 65535) return -1;
      p = end;
    }
    for(long v = a; v <= b; v++) {
      if(!seen[v]) { seen[v] = 1; scan_ports[n++] = (int)v; }
    }
    if(*p == ',') p++;
    else if(*p != 0) return -1;
  }
  return n;
}

// Dashboard output (--drsm, --submit, see common/drhook.h): set
// hook_ready once the arguments are good, so a usage error sends
// nothing
drhookOptions hook("netpscan");
drhookTee hook_tee;
drhookLines hook_summary;
drhookLines hook_table;
int hook_ready = 0;
int hook_nports = 0, hook_scanned = 0, hook_up = 0, hook_skipped = 0;
int hook_unresolved = 0;
long hook_open = 0;

int RunTool(int argc, char **argv)
{
  int timeout_ms = 500;
  int all_services = 0;
  int skip_down = 0;
  const char *port_spec = 0;
  const char *args[2] = { 0, 0 };
  int nargs = 0;
  for(int i = 1; i < argc; i++) {
    if(strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
      timeout_ms = atoi(argv[++i]);
      if(timeout_ms < 1 || timeout_ms > 60000) {
	cout << "The timeout must be 1 to 60000 ms" << "\n" << flush;
	return 2;
      }
    }
    else if(strcmp(argv[i], "-p") == 0 && i + 1 < argc) port_spec = argv[++i];
    else if(strcmp(argv[i], "-A") == 0) all_services = 1;
    else if(strcmp(argv[i], "-s") == 0) skip_down = 1;
    else if(argv[i][0] == '-' || nargs >= 2) {
      HelpMessage(argv[0]);
      return 2;
    }
    else args[nargs++] = argv[i];
  }
  if(nargs < 1) {
    HelpMessage(argv[0]);
    return 2;
  }
  const char *iplist = args[0];
  const char *services = (nargs == 2) ? args[1] : drnet_default_services();

  int num_services = drnet_load_tcp_services(services, services_list,
					     MaxServices);
  if(num_services < 0) {
    if(nargs == 2 || !port_spec) {
      cout << "Cannot open the " << services << " file" << "\n" << flush;
      return 2;
    }
    num_services = 0;
  }

  int nports = 0;
  if(port_spec) {
    nports = ParsePorts(port_spec);
    if(nports <= 0) {
      cout << port_spec << " is not a valid port list (1-65535)" << "\n"
	   << flush;
      return 2;
    }
  }
  else {
    for(int i = 0; i < num_services; i++) {
      if(!all_services && services_list[i].port >= 1024) continue;
      scan_ports[nports++] = services_list[i].port;
    }
    if(nports == 0) {
      cout << "No TCP ports in " << services << "\n" << flush;
      return 2;
    }
  }

  ifstream ipfile(iplist, ios::in);
  if(!ipfile) {
    cout << "Cannot open the " << iplist << " file" << "\n" << flush;
    return 2;
  }
  if(!drnet_init()) {
    cout << "Could not start the socket library" << "\n" << flush;
    return 2;
  }

  hook_ready = 1;
  hook_nports = nports;
  hook_table.Columns();
  hook_table.Cell("Host"); hook_table.Cell("Address"); hook_table.Cell("Name");
  hook_table.Cell("Ping"); hook_table.Cell("Open"); hook_table.Cell("Ports");
  cerr << "Scanning " << nports << " TCP ports per address" << "\n";
  char line[512];
  int hosts = 0;
  while(ipfile.getline(line, sizeof(line)) || ipfile.gcount() > 0) {
    if(ipfile.fail() && !ipfile.eof()) ipfile.clear();
    gxString entry = line;
    entry.DeleteAfterIncluding("#");
    entry.TrimLeadingSpaces();
    entry.TrimTrailingSpaces();
    entry.DeleteAfterIncluding(" ");
    entry.DeleteAfterIncluding("\t");
    entry.DeleteAfterIncluding("\r");
    if(entry.is_null()) continue;

    gxString ip, err;
    cout << "\n" << "Contacting host: " << entry.c_str() << "\n";
    if(!drnet_resolve(entry.c_str(), ip, err)) {
      cout << err.c_str() << "\n" << flush;
      hook_unresolved++;
      hook_table.Row();
      hook_table.Cell(entry.c_str());
      hook_table.Cell(""); hook_table.Cell("");
      hook_table.Cell("not resolved");
      hook_table.Cell(""); hook_table.Cell(err.c_str());
      continue;
    }
    hosts++;
    gxString name;
    gxString hping;
    if(drnet_reverse(ip.c_str(), name)) {
      cout << "DNS name entry: " << name.c_str() << "\n";
    }
    else {
      cout << "This IP address does not have a DNS name entry" << "\n";
    }
    if(ip != entry) cout << "DNS IP address: " << ip.c_str() << "\n";

    double rtt = 0;
    int rv = drnet_ping(ip.c_str(), timeout_ms, rtt, err);
    if(rv == DRNET_REPLY) {
      cout << "Host is up (ping)" << "\n";
      hping = "up";
      hook_up++;
    }
    else if(rv == DRNET_NO_REPLY) {
      cout << "Host not responding to ping." << "\n";
      hping = "no reply";
      if(skip_down) {
        hook_skipped++;
        hook_table.Row();
        hook_table.Cell(entry.c_str());
        hook_table.Cell(ip.c_str());
        hook_table.Cell(name.c_str());
        hook_table.Cell("no reply, not scanned");
        hook_table.Cell(""); hook_table.Cell("");
        cout << flush;
        continue;
      }
    }
    else {
      cout << "Ping skipped: " << err.c_str() << "\n";
      hping = "skipped";
    }

    cerr << "Scanning " << ip.c_str() << "..." << "\n";
    cout << "Scanning specified TCP ports..." << "\n" << flush;
    int open_ports = 0;
    gxString plist;
    for(int i = 0; i < nports; i++) {
      int port = scan_ports[i];
      int crv = drnet_tcp_connect(ip.c_str(), port, timeout_ms, err);
      if(crv == DRNET_ERROR) {
	cout << err.c_str() << "\n" << flush;
	return 2;
      }
      if(crv != 1) continue;
      open_ports++;
      if(!plist.is_null()) plist << " ";
      plist << port;
      cout << "TCP port " << port << " open" << "\n";
      const drnetService *s = drnet_find_service(services_list,
						 num_services, port);
      cout << port << "/tcp";
      if(s) {
	cout << " " << s->name;
	if(s->aliases[0]) cout << " " << s->aliases;
	if(s->comment[0]) cout << " # " << s->comment;
      }
      cout << "\n" << flush;
    }
    cout << open_ports << " open TCP port(s)" << "\n";
    hook_scanned++;
    hook_open += open_ports;
    hook_table.Row();
    hook_table.Cell(entry.c_str());
    hook_table.Cell(ip.c_str());
    hook_table.Cell(name.c_str());
    hook_table.Cell(hping.c_str());
    hook_table.Cell((double)open_ports);
    hook_table.Cell(plist.c_str());
    cout << "Comments:" << "\n" << flush; // Space for written comments
  }
  if(hosts == 0) {
    cout << "No addresses in " << iplist << "\n" << flush;
    return 2;
  }
  return 0;
}
// The scan as DRSM lines: a row per address with its open ports
int HookFinish(int rc)
{
  gxString info;
  info << "netpscan: " << hook_scanned << " address(es) scanned on "
       << hook_nports << " TCP ports, " << hook_open << " open port(s)";
  if(hook_skipped) info << ", " << hook_skipped << " skipped (no ping reply)";
  if(hook_unresolved) info << ", " << hook_unresolved << " not resolved";
  hook_summary.Info(info.c_str());
  drhook_head(hook_summary, ProgramName, VersionString);
  hook_summary.Value("ports_per_host", (double)hook_nports);
  hook_summary.Value("scanned", (double)hook_scanned);
  hook_summary.Value("up", (double)hook_up);
  hook_summary.Value("skipped", (double)hook_skipped);
  hook_summary.Value("not_resolved", (double)hook_unresolved);
  hook_summary.Value("open_ports", (double)hook_open);
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
