// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Source Code File Name: portscan.cpp
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

Connectivity check of one host: its DNS names and addresses, a ping,
and a full TCP connect to one or more ports, with the services file
entry of each open port. For your own hosts only.

Changes:
11/18/2026: dr_utils version, based on work from the 1999-2004
portscan program. Port lists and ranges (22,80,443 or 8000-8100, up
to 1024 ports), a connect timeout (a filtered port no longer waits
two minutes), and the ping is informational: a host that drops ICMP
behind a firewall is still scanned. The Windows services file is
read from %SystemRoot%\System32\drivers\etc.
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

const char *ProgramName = "portscan";
const char *VersionString = DR_UTILS_VERSION;
const int MaxPorts = 1024;

void HelpMessage(const char *pname)
{
  cout << ProgramName << " version " << VersionString
       << " (dr_utils, DataReel 6.4.1)" << "\n";
  cout << "\n";
  cout << "Usage: " << pname << " [-t ms] [-n] hostname ports [services_file]"
       << "\n";
  cout << "ports = a port, a list or a range: 22  22,80,443  8000-8100"
       << "\n";
  cout << "        (up to " << MaxPorts << " ports)" << "\n";
  cout << "-t ms = connect and ping timeout in milliseconds (default 2000)"
       << "\n";
  cout << "-n    = do not ping the host first" << "\n";
  cout << "services_file = default " << drnet_default_services() << "\n";
  cout << "\n";
  cout << "Exit status: 0 if any port is open, 1 if none is, 2 on an error."
       << "\n";
  cout << "Scan only hosts you own or are allowed to scan." << "\n";
  cout << "\n" << flush;
  drhook_usage(ProgramName);
}

// Parse "22", "22,80,443", "8000-8100" or a mix. Returns the count,
// or -1 on an error.
int ParsePorts(const char *spec, int *ports, int max_ports)
{
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
      if(n >= max_ports) return -1;
      ports[n++] = (int)v;
    }
    if(*p == ',') p++;
    else if(*p != 0) return -1;
  }
  return n;
}

// Services file TCP entries
const int MaxServices = 20000;
static drnetService services_list[MaxServices];
int num_services = 0;

// Print the services file entry for a TCP port
int FindServiceByPort(int port)
{
  const drnetService *s = drnet_find_service(services_list, num_services,
					     port);
  if(!s) return 0;
  cout << "  Name:     " << s->name << "\n";
  if(s->aliases[0]) cout << "  Aliases:  " << s->aliases << "\n";
  if(s->comment[0]) cout << "  Comment:  " << s->comment << "\n";
  return 1;
}

// Dashboard output (--drsm, --submit, see common/drhook.h): set
// hook_ready once the arguments are good, so a usage error sends
// nothing
drhookOptions hook("portscan");
drhookTee hook_tee;
drhookLines hook_summary;
drhookLines hook_table;
int hook_ready = 0;
gxString hook_host, hook_ip, hook_ping = "not run", hook_ping_ms;
int hook_nports = 0, hook_open = 0;
drhookLines hook_ports;  // port_<n>=open|closed (64 ports or fewer)

int RunTool(int argc, char **argv)
{
  int timeout_ms = 2000;
  int do_ping = 1;
  const char *args[3] = { 0, 0, 0 };
  int nargs = 0;
  for(int i = 1; i < argc; i++) {
    if(strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
      timeout_ms = atoi(argv[++i]);
      if(timeout_ms < 1 || timeout_ms > 60000) {
	cout << "The timeout must be 1 to 60000 ms" << "\n" << flush;
	return 2;
      }
    }
    else if(strcmp(argv[i], "-n") == 0) do_ping = 0;
    else if(argv[i][0] == '-' || nargs >= 3) {
      HelpMessage(argv[0]);
      return 2;
    }
    else args[nargs++] = argv[i];
  }
  if(nargs < 2) {
    HelpMessage(argv[0]);
    return 2;
  }
  const char *hostname = args[0];
  const char *services = (nargs == 3) ? args[2] : drnet_default_services();

  static int ports[MaxPorts];
  int nports = ParsePorts(args[1], ports, MaxPorts);
  if(nports <= 0) {
    cout << args[1] << " is not a valid port list (1-65535, at most "
	 << MaxPorts << " ports)" << "\n" << flush;
    return 2;
  }

  num_services = drnet_load_tcp_services(services, services_list, 
					 MaxServices);
  if(num_services < 0) {
    if(nargs == 3) {
      cout << "Cannot open the " << services << " file" << "\n" << flush;
      return 2;
    }
    num_services = 0; // No services names on this system
  }

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
  hook_nports = nports;
  hook_table.Columns();
  hook_table.Cell("Port"); hook_table.Cell("State");
  hook_table.Cell("Service");
  cout << "Contacting host: " << hostname << "\n";
  gxString info;
  if(drnet_host_info(hostname, info)) cout << info.c_str();

  if(do_ping) {
    double rtt = 0;
    int rv = drnet_ping(ip.c_str(), timeout_ms, rtt, err);
    if(rv == DRNET_REPLY) {
      char buf[64];
      sprintf(buf, "%.1f", rtt);
      cout << "Ping reply in " << buf << " ms" << "\n";
      hook_ping = "up";
      hook_ping_ms = buf;
    }
    else if(rv == DRNET_NO_REPLY) {
      hook_ping = "no reply";
      cout << "No ping reply (the host or a firewall may drop ICMP)"
	   << "\n";
    }
    else {
      cout << "Ping skipped: " << err.c_str() << "\n";
      hook_ping = "skipped";
    }
  }
  cout << flush;

  int open_ports = 0;
  for(int i = 0; i < nports; i++) {
    int rv = drnet_tcp_connect(ip.c_str(), ports[i], timeout_ms, err);
    if(rv == DRNET_ERROR) {
      cout << err.c_str() << "\n" << flush;
      return 2;
    }
    if(rv == 1) {
      open_ports++;
      cout << "TCP port " << ports[i] << " open" << "\n";
      const drnetService *hs = drnet_find_service(services_list,
						  num_services, ports[i]);
      hook_table.Row();
      hook_table.Cell((double)ports[i]);
      hook_table.Cell("open");
      hook_table.Cell(hs ? hs->name : "");
      if(!FindServiceByPort(ports[i])) {
	cout << "  Service type unknown" << "\n";
      }
    }
    else if(nports <= 16) { // A short list shows every port
      cout << "TCP port " << ports[i] << " closed or filtered" << "\n";
      hook_table.Row();
      hook_table.Cell((double)ports[i]);
      hook_table.Cell("closed or filtered");
      hook_table.Cell("");
    }
    if(nports <= 64) {
      char vn[32];
      sprintf(vn, "port_%d", ports[i]);
      hook_ports.Value(vn, rv == 1 ? "open" : "closed");
    }
    hook_open = open_ports;
    cout << flush;
  }
  if(nports > 1) {
    cout << open_ports << " of " << nports << " TCP ports open" << "\n"
	 << flush;
  }
  return open_ports ? 0 : 1;
}
// The scan as DRSM lines: the host, ping, open ports, a value per
// port (port_22=open) for rules and a row per open port (every port
// when there are 16 or fewer)
int HookFinish(int rc)
{
  gxString info;
  info << "portscan " << hook_host << " (" << hook_ip << "): " << hook_open
       << " of " << hook_nports << " TCP ports open, ping " << hook_ping;
  hook_summary.Info(info.c_str());
  drhook_head(hook_summary, ProgramName, VersionString);
  hook_summary.Value("host", hook_host.c_str());
  hook_summary.Value("address", hook_ip.c_str());
  hook_summary.Value("ping", hook_ping.c_str());
  if(!hook_ping_ms.is_null()) hook_summary.Value("ping_ms", hook_ping_ms.c_str());
  hook_summary.Value("ports", (double)hook_nports);
  hook_summary.Value("open", (double)hook_open);
  hook_summary.Append(hook_ports);
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
