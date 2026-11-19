// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Source Code File Name: internet-test.cpp
// C++ Compiler Used: MSVC, GCC
// Produced By: DataReel Software Development Team
// File Creation Date: 2005
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

Test Internet connectivity: resolve a web server's name, connect to
its HTTP port and read an HTTP response header. Any HTTP answer
passes. By default the test repeats every 60 seconds until it
passes, so a startup script can wait for the network before it
starts a job that needs it.

Changes:
11/18/2026: dr_utils version, based on work from the 2005
internet-test program. -c sets the number of tries (the 2005 version
tried forever), -w the wait between tries, -t a connect timeout, -p
the port; -q prints only the result. Each failure says which step
failed (DNS, connect or HTTP).
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

#if defined (__UNIX__)
#include <unistd.h>
#include <signal.h>
#endif

#include "gxshttpc.h"
#include "gxsurl.h"
#include "drnet.h"
#include "drutil.h"
#include "drhook.h"

const char *ProgramName = "internet-test";
const char *VersionString = DR_UTILS_VERSION;

void HelpMessage(const char *pname)
{
  cout << ProgramName << " version " << VersionString
       << " (dr_utils, DataReel 6.4.1)" << "\n";
  cout << "\n";
  cout << "Usage: " << pname << " [switches] server" << "\n";
  cout << "server = web server to test connectivity to (a host name or URL)"
       << "\n";
  cout << "Switches: -c n    = try n times, 0 = until it passes (default 0)"
       << "\n";
  cout << "          -w sec  = seconds between tries (default 60)" << "\n";
  cout << "          -t ms   = connect timeout in milliseconds (default 5000)"
       << "\n";
  cout << "          -p port = HTTP port (default 80)" << "\n";
  cout << "          -q      = print only the final result" << "\n";
  cout << "\n";
  cout << "Example: " << pname << " www.example.com" << "\n";
  cout << "Example: " << pname << " -c 3 -w 10 www.example.com" << "\n";
  cout << "\n";
  cout << "Exit status: 0 when the test passes, 1 when every try failed, "
       << "2 on an error." << "\n";
  cout << "\n" << flush;
  drhook_usage(ProgramName);
}

void SleepSeconds(int seconds)
{
#if defined (__WIN32__)
  Sleep((DWORD)seconds * 1000);
#else
  sleep((unsigned)seconds);
#endif
}

// One try. Returns 1 if the server sent an HTTP response.
int TestOnce(const gxString &server, int port, int timeout_ms,
	     gxString &why)
{
  gxString ip, err;
  if(!drnet_resolve(server.c_str(), ip, err)) {
    why << clear << "DNS: " << err;
    return 0;
  }
  int rv = drnet_tcp_connect(ip.c_str(), port, timeout_ms, err);
  if(rv != 1) {
    why << clear << "connect to " << ip << " port " << port;
    if(rv == DRNET_ERROR) why << ": " << err;
    else why << " failed or timed out";
    return 0;
  }

  gxString web_url;
  web_url << clear << "http://" << server;
  if(port != 80) web_url << ":" << port;
  web_url << "/";
  gxsURL url;
  gxsURLInfo u;
  if(!url.ParseURL(web_url, u)) {
    why << clear << "cannot parse " << web_url;
    return 0;
  }
  gxsHTTPClient httpc;
  gxsHTTPHeader hdr;
  if(httpc.RequestHeader(u, hdr) != 0) {
    why << clear << "HTTP: " << httpc.SocketExceptionMessage();
    return 0;
  }
  why << clear << "HTTP status " << hdr.http_status;
  return 1;
}

// Dashboard output (--drsm, --submit, see common/drhook.h): set
// hook_ready once the arguments are good, so a usage error sends
// nothing
drhookOptions hook("internet-test");
drhookTee hook_tee;
drhookLines hook_summary;
drhookLines hook_table;
int hook_ready = 0;
gxString hook_server, hook_detail;
int hook_port = 0, hook_passed = 0, hook_tries = 0;
time_t hook_start = 0;

int RunTool(int argc, char **argv)
{
#if defined (__UNIX__)
  // A server that closes the connection must not kill the program
  signal(SIGPIPE, SIG_IGN);
#endif

  int count = 0, wait_sec = 60, timeout_ms = 5000, port = 80, quiet = 0;
  const char *server_arg = 0;
  for(int i = 1; i < argc; i++) {
    if(strcmp(argv[i], "-c") == 0 && i + 1 < argc) count = atoi(argv[++i]);
    else if(strcmp(argv[i], "-w") == 0 && i + 1 < argc) {
      wait_sec = atoi(argv[++i]);
    }
    else if(strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
      timeout_ms = atoi(argv[++i]);
    }
    else if(strcmp(argv[i], "-p") == 0 && i + 1 < argc) port = atoi(argv[++i]);
    else if(strcmp(argv[i], "-q") == 0) quiet = 1;
    else if(argv[i][0] == '-' || server_arg) {
      HelpMessage(argv[0]);
      return 2;
    }
    else server_arg = argv[i];
  }
  if(!server_arg || count < 0 || wait_sec < 0 || wait_sec > 86400 ||
     timeout_ms < 1 || timeout_ms > 120000 || port < 1 || port > 65535) {
    HelpMessage(argv[0]);
    return 2;
  }

  // Keep only the host name of a URL
  gxString server = server_arg;
  if(server.Find("://") != -1) server.DeleteBeforeIncluding("://");
  server.DeleteAfterIncluding("/");
  if(server.Find(":") != -1) {
    gxString p = server;
    p.DeleteBeforeIncluding(":");
    port = atoi(p.c_str());
    server.DeleteAfterIncluding(":");
    if(port < 1 || port > 65535) {
      cout << "Bad port in " << server_arg << "\n" << flush;
      return 2;
    }
  }
  if(server.is_null()) {
    HelpMessage(argv[0]);
    return 2;
  }
  if(!drnet_init()) {
    cout << "Could not start the socket library" << "\n" << flush;
    return 2;
  }

  hook_ready = 1;
  hook_server = server;
  hook_port = port;
  hook_start = time(0);
  for(int tries = 1; ; tries++) {
    gxString why;
    if(TestOnce(server, port, timeout_ms, why)) {
      hook_passed = 1;
      hook_tries = tries;
      hook_detail = why;
      cout << "Internet connectivity test passed (" << server.c_str()
	   << ", " << why.c_str() << ")" << "\n" << flush;
      return 0;
    }
    if(count && tries >= count) {
      hook_tries = tries;
      hook_detail = why;
      cout << "Internet connectivity test failed after " << tries
	   << " tries (" << why.c_str() << ")" << "\n" << flush;
      return 1;
    }
    if(!quiet) {
      cout << "Internet connectivity test failed (" << why.c_str()
	   << "), retry in " << wait_sec << " seconds" << "\n" << flush;
    }
    SleepSeconds(wait_sec);
  }
  return 1;
}
// The test as DRSM lines: passed (1 or 0), tries, the last result
int HookFinish(int rc)
{
  gxString info;
  info << "internet-test " << hook_server << " port " << hook_port << ": "
       << (hook_passed ? "passed" : "failed") << " after " << hook_tries
       << " tr" << (hook_tries == 1 ? "y" : "ies") << " (" << hook_detail
       << ")";
  hook_summary.Info(info.c_str());
  drhook_head(hook_summary, ProgramName, VersionString);
  hook_summary.Value("server", hook_server.c_str());
  hook_summary.Value("port", (double)hook_port);
  hook_summary.Value("passed", hook_passed ? "1" : "0");
  hook_summary.Value("tries", (double)hook_tries);
  hook_summary.Value("seconds", difftime(time(0), hook_start));
  hook_summary.Value("result", hook_detail.c_str());
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
