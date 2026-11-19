// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Source Code File Name: getmac.cpp
// C++ Compiler Used: MSVC, GCC
// Produced By: DataReel Software Development Team
// File Creation Date: 1999
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
}

int main(int argc, char **argv)
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

  cout << "\n" << "Contacting host: " << hostname << "\n";
  gxString info;
  if(drnet_host_info(hostname, info)) cout << info.c_str();
  else cout << "Could not resolve hostname" << "\n";

  double rtt = 0;
  int rv = drnet_ping(ip.c_str(), 1000, rtt, err);
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
  return 0;
}
// ----------------------------------------------------------- //
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
