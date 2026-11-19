// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Source Code File Name: mcserv.cpp
// C++ Compiler Used: MSVC, GCC
// Produced By: DataReel Software Development Team
// File Creation Date: 2002
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

Multicast benchmark sender for mctest. It sends numbered datagrams to
a multicast group at a set rate, then end of run datagrams, so every
mctest receiver on the network can measure the rate, loss and order.
Start the receivers first.

Multicast reaches every host on the network that joins the group, so
the defaults are cautious: TTL 1 (the local network only), an
administratively scoped group (239.255.10.10), 100 Mbit/s and 150 MB.
Use it only on networks you run.

Changes:
11/18/2026: dr_utils version, based on Douglas Gaer's 2002-2019
multicast benchmark server (bmarks.mc, then benchmarks/src/
multicasttest/mcserv), on the same default port (53898). The old
server flooded the network until it was stopped; this one sends a
set number of bytes at a set rate (-s, -r, --seconds), with sequence
numbers and end of run datagrams. New: --ttl, -i, Windows builds.
*/
// ----------------------------------------------------------- //
#include "gxdlcode.h"

#include <iostream>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drsock.h"
#include "drbench.h"
#include "drutil.h"

const char *ProgramName = "mcserv";
const char *VersionString = DR_UTILS_VERSION;

void HelpMessage()
{
  cout << ProgramName << " version " << VersionString
       << " (dr_utils, DataReel 6.4.1)" << "\n";
  cout << "Multicast benchmark sender (receivers: mctest)" << "\n\n";
  cout << "Usage: " << ProgramName << " [switches]" << "\n";
  cout << "Switches: " << "\n";
  cout << "-?           = Display this help message." << "\n";
  cout << "-b[size]     = Datagram size, -b" << DRSOCK_DGRAM_MIN << " to -b"
       << DRSOCK_DGRAM_MAX << " (default 1500)" << "\n";
  cout << "-g[group]    = Multicast group, 224.0.0.0 to 239.255.255.255 "
       << "(default " << DRSOCK_MC_GROUP << ")" << "\n";
  cout << "-i[address]  = Send from the interface with this IPv4 address"
       << "\n";
  cout << "-p[port]     = Port number, -p1024 to -p65535 (default "
       << DRSOCK_MC_PORT << ")" << "\n";
  cout << "-r[rate]     = Send rate in bits a second: -r10M -r1G (default "
       << "100M)" << "\n";
  cout << "-s[bytes]    = Bytes to send: -s1K to -s100G (default 150M)" << "\n";
  cout << "--seconds=N  = Stop after N seconds even if bytes are left" << "\n";
  cout << "--ttl=N      = Multicast TTL, 1 to 32 (default 1: the local "
       << "network)" << "\n";
  cout << "--no-loop    = Do not deliver to receivers on this host" << "\n";
  cout << "\n" << flush;
}

int main(int argc, char **argv)
{
  int dgram = 1500, port = DRSOCK_MC_PORT, loop = 1;
  unsigned long ttl = 1, max_secs = 0;
  double rate = 100e6, bytes = 150e6;
  gxString group = DRSOCK_MC_GROUP, iface;
  for(int i = 1; i < argc; i++) {
    const char *a = argv[i];
    const char *v;
    if(strcmp(a, "-?") == 0 || drbench_switch(a, "help", v)) {
      HelpMessage();
      return 0;
    }
    else if(strncmp(a, "-b", 2) == 0) {
      dgram = atoi(a + 2);
      if(dgram < DRSOCK_DGRAM_MIN || dgram > DRSOCK_DGRAM_MAX) {
	cout << "Bad datagram size specified: " << a + 2 << "\n" << flush;
	return 1;
      }
    }
    else if(strncmp(a, "-g", 2) == 0) group = a + 2;
    else if(strncmp(a, "-i", 2) == 0 && a[2]) iface = a + 2;
    else if(strncmp(a, "-p", 2) == 0) {
      port = atoi(a + 2);
      if(port < 1024 || port > 65535) {
	cout << "Bad port number specified: " << a + 2 << "\n" << flush;
	return 1;
      }
    }
    else if(strncmp(a, "-r", 2) == 0) {
      if(!drbench_parse_size(a + 2, 100e9, rate)) {
	cout << "Bad rate specified: " << a + 2 << "\n" << flush;
	return 1;
      }
    }
    else if(strncmp(a, "-s", 2) == 0) {
      if(!drbench_parse_size(a + 2, 100e9, bytes) || bytes < 1000) {
	cout << "Bad number of bytes specified: " << a + 2 << "\n" << flush;
	return 1;
      }
    }
    else if(drbench_switch(a, "seconds", v)) {
      if(!drbench_parse_count(v, 1, 86400, max_secs)) {
	cout << "Bad --seconds" << "\n" << flush;
	return 1;
      }
    }
    else if(drbench_switch(a, "ttl", v)) {
      if(!drbench_parse_count(v, 1, 32, ttl)) {
	cout << "Bad --ttl (1 to 32)" << "\n" << flush;
	return 1;
      }
    }
    else if(drbench_switch(a, "no-loop", v)) loop = 0;
    else {
      cout << "Unknown switch " << a << "\n" << flush;
      HelpMessage();
      return 1;
    }
  }
  if(!drsock_is_multicast(group.c_str())) {
    cout << group.c_str() << " is not a multicast group address (224.0.0.0 "
	 << "to 239.255.255.255)" << "\n" << flush;
    return 1;
  }

  gxString err;
  drsock_t s = drsock_udp_open(iface.c_str(), 0, err);
  if(s == DRSOCK_BAD ||
     !drsock_mc_sender(s, (int)ttl, loop, iface.c_str(), err)) {
    cout << "Error: " << err.c_str() << "\n" << flush;
    return 1;
  }
  drsock_set_buffers(s, 4 * 1024 * 1024);

  unsigned long count = (unsigned long)(bytes / dgram);
  if(count < 1) count = 1;
  unsigned long run = (unsigned long)(drbench_now() * 1000.0) & 0x7FFFFFFFUL;
  cout << "Multicast benchmark sender " << VersionString << "\n";
  cout << "Sending " << count << " datagrams of " << dgram << " bytes to "
       << group.c_str() << " port " << port << " at "
       << drbench_shn(rate).c_str() << "bit/s, TTL " << ttl << ", run " << run
       << "\n" << flush;

  unsigned char *buf = new unsigned char[dgram];
  memset(buf, 'D', dgram);
  memcpy(buf, "DRBU", 4);
  drsock_put32(buf + 4, run);
  drsock_put32(buf + 12, count);
  drbenchPacer pacer(rate);
  double start = drbench_now();
  unsigned long sent = 0;
  for(unsigned long i = 0; i < count; i++) {
    drsock_put32(buf + 8, i);
    if(drsock_sendto(s, buf, dgram, group.c_str(), port) < 0) {
      cout << "Error: sendto: " << drsock_error().c_str() << "\n" << flush;
      break;
    }
    sent++;
    pacer.Sent((double)dgram);
    if(max_secs && (i & 255) == 0 && drbench_now() - start > max_secs) break;
  }
  double secs = drbench_now() - start;

  // End of run, five times in case one is lost
  unsigned char end[DRSOCK_DGRAM_HDR];
  memcpy(end, "DRBE", 4);
  drsock_put32(end + 4, run);
  drsock_put32(end + 8, sent);
  drsock_put32(end + 12, count);
  for(int k = 0; k < 5; k++) {
    drbench_sleep_ms(100);
    drsock_sendto(s, end, sizeof(end), group.c_str(), port);
  }
  drsock_close(s);
  delete[] buf;

  char line[256];
  double sent_bytes = (double)sent * dgram;
  sprintf(line, "Sent %lu datagrams (%.0f bytes) in %.3f seconds, %s", sent,
	  sent_bytes, secs, drbench_rate(sent_bytes, secs).c_str());
  cout << line << "\n" << flush;
  return sent ? 0 : 1;
}
// ----------------------------------------------------------- //
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
