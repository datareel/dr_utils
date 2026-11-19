// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Source Code File Name: internet-test.cpp
// C++ Compiler Used: MSVC, GCC
// Produced By: DataReel Software Development Team
// File Creation Date: 2005
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

int main(int argc, char **argv)
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

  for(int tries = 1; ; tries++) {
    gxString why;
    if(TestOnce(server, port, timeout_ms, why)) {
      cout << "Internet connectivity test passed (" << server.c_str()
	   << ", " << why.c_str() << ")" << "\n" << flush;
      return 0;
    }
    if(count && tries >= count) {
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
// ----------------------------------------------------------- //
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
