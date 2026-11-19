// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Source Code File Name: arpscan.cpp
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

List the MAC (Media Access Control) address of every host on a
local class C (/24) network, or of one host: each address is pinged
so the system learns its MAC address, then the MAC address is read
from the ARP table. One tab-separated line per host that answers:
address, DNS name, MAC address and, with a vendor file, the vendor.
A LAN inventory of your own networks.

Changes:
11/18/2026: dr_utils version, based on work from the 1999-2008
arpscan program. The MAC address comes from /proc/net/arp on Linux
(no SIOCGARP ioctl, so the interface name is optional) and from
SendARP on Windows (the 1999 version ran "arp -a" into a file in
the current folder). The switches replace the positional vendor and
output files; the 2008 version ignored the Linux output file name.
*/
// ----------------------------------------------------------- //
#include "gxdlcode.h"

#include <iostream>
#include <fstream>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gxmac48.h"
#include "drnet.h"
#include "drutil.h"

const char *ProgramName = "arpscan";
const char *VersionString = DR_UTILS_VERSION;

void HelpMessage(const char *pname)
{
  cout << ProgramName << " version " << VersionString
       << " (dr_utils, DataReel 6.4.1)" << "\n";
  cout << "\n";
  cout << "Usage: " << pname << " [switches] range" << "\n";
  cout << "range = a.b.c (hosts .1 to .254), a.b.c.d or a.b.c.x-y" << "\n";
  cout << "Switches: -i dev  = only ARP entries on this interface (Linux)"
       << "\n";
  cout << "          -v file = vendor code file (MAC prefix and name per line,"
       << "\n" << "                    as read by DataReel FindVendor)" << "\n";
  cout << "          -o file = also write the list to a file" << "\n";
  cout << "          -t ms   = ping timeout in milliseconds (default 1000)"
       << "\n";
  cout << "          -a      = list hosts with no MAC address too" << "\n";
  cout << "\n";
  cout << "Example 1: " << pname << " 192.0.2" << "\n";
  cout << "Example 2: " << pname << " -v maccodes.txt -o lan.txt 192.0.2"
       << "\n";
  cout << "Example 3: " << pname << " 192.0.2.7" << "\n";
  cout << "\n";
  cout << "Output, tab separated: address name MAC [vendor]" << "\n";
  cout << "Only hosts on a directly attached network have a MAC address."
       << "\n";
  cout << "Exit status: 0 if any MAC address was found, 1 if none, 2 on an "
       << "error." << "\n";
  cout << "\n" << flush;
}

int main(int argc, char **argv)
{
  const char *dev = 0, *vendors = 0, *ofname = 0, *range = 0;
  int timeout_ms = 1000;
  int all = 0;
  for(int i = 1; i < argc; i++) {
    if(strcmp(argv[i], "-i") == 0 && i + 1 < argc) dev = argv[++i];
    else if(strcmp(argv[i], "-v") == 0 && i + 1 < argc) vendors = argv[++i];
    else if(strcmp(argv[i], "-o") == 0 && i + 1 < argc) ofname = argv[++i];
    else if(strcmp(argv[i], "-a") == 0) all = 1;
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
  if(vendors) {
    ifstream vf(vendors, ios::in);
    if(!vf) {
      cout << "Cannot open the " << vendors << " vendor file" << "\n"
	   << flush;
      return 2;
    }
  }
  fstream outfile;
  if(ofname) {
    outfile.open(ofname, ios::out|ios::trunc);
    if(!outfile) {
      cout << "Could not open the " << ofname << " output file" << "\n"
	   << flush;
      return 2;
    }
  }
  if(!drnet_init()) {
    cout << "Could not start the socket library" << "\n" << flush;
    return 2;
  }

  int found = 0;
  for(int h = first; h <= last; h++) {
    gxString ip;
    ip << clear << base << "." << h;
    double rtt = 0;
    int rv = drnet_ping(ip.c_str(), timeout_ms, rtt, err);
    if(rv == DRNET_ERROR) {
      cout << ip.c_str() << ": " << err.c_str() << "\n" << flush;
      return 2;
    }
    // A host that drops ICMP can still answer ARP, so look it up even
    // without a ping reply
    gxString mac;
    int has_mac = drnet_mac_address(ip.c_str(), dev, mac);
    if(!has_mac && !all) continue;
    if(!has_mac && rv != DRNET_REPLY) continue; // -a: hosts that answer

    gxString name;
    if(!drnet_reverse(ip.c_str(), name)) name = "no_DNS_entry";

    gxString out;
    out << clear << ip << "\t" << name << "\t";
    if(has_mac) {
      found++;
      out << mac;
      if(vendors) {
	gxMAC48 m(mac.c_str(), ':', 2);
	char vname[255], comment[255];
	vname[0] = comment[0] = 0;
	if(FindVendor(vendors, m, vname, comment) && vname[0]) {
	  out << "\t" << vname;
	}
      }
    }
    else {
      out << "no_ARP_entry";
    }
    cout << out.c_str() << "\n" << flush;
    if(ofname) outfile << out.c_str() << "\n" << flush;
  }
  if(ofname) outfile.close();
  return found ? 0 : 1;
}
// ----------------------------------------------------------- //
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
