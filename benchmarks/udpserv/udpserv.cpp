// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Source Code File Name: udpserv.cpp
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

UDP bandwidth benchmark server for udptest. It counts the datagrams
of each client run (by sender address and run number): how many
arrived, how many out of order, the bytes, and the time from the
first to the last. When the client sends its end of run datagram the
server answers with those counts, so the client can report the loss
and the rate that really arrived. One line per finished run.

Changes:
11/18/2026: dr_utils version, based on Douglas Gaer's 2002-2019 UDP
benchmark server (bmarks.udp, then benchmarks/src/udptest/udpserv),
on the same default port (53897). It now counts loss and order with
sequence numbers and reports back to the client. The clients can no
longer shut down or restart the server (the old -k and -r). New:
--count, -l, Windows builds.
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

const char *ProgramName = "udpserv";
const char *VersionString = DR_UTILS_VERSION;
const int MaxRuns = 1024;
const double RunExpireSecs = 120;

struct Run {
  int in_use;
  int reported;
  gxString ip;
  int port;
  unsigned long run;
  unsigned long received;
  unsigned long out_of_order;
  unsigned long next_seq;
  unsigned long long bytes;
  double first, last;
};

Run runs[MaxRuns];

Run *FindRun(const gxString &ip, int port, unsigned long run, int create)
{
  int free_slot = -1;
  double now = drbench_now();
  for(int i = 0; i < MaxRuns; i++) {
    if(runs[i].in_use) {
      if(runs[i].port == port && runs[i].run == run && runs[i].ip == ip) {
	return &runs[i];
      }
      if(now - runs[i].last > RunExpireSecs) runs[i].in_use = 0;
    }
    if(!runs[i].in_use && free_slot < 0) free_slot = i;
  }
  if(!create || free_slot < 0) return 0;
  Run *r = &runs[free_slot];
  r->in_use = 1;
  r->reported = 0;
  r->ip = ip;
  r->port = port;
  r->run = run;
  r->received = r->out_of_order = r->next_seq = 0;
  r->bytes = 0;
  r->first = r->last = now;
  return r;
}

void HelpMessage()
{
  cout << ProgramName << " version " << VersionString
       << " (dr_utils, DataReel 6.4.1)" << "\n";
  cout << "UDP bandwidth benchmark server (clients: udptest)" << "\n\n";
  cout << "Usage: " << ProgramName << " [switches]" << "\n";
  cout << "Switches: " << "\n";
  cout << "-?          = Display this help message." << "\n";
  cout << "-p[port]    = Port number, -p1024 to -p65535 (default "
       << DRSOCK_UDP_PORT << ")" << "\n";
  cout << "-l[address] = Listen on this IPv4 address only (default: all)"
       << "\n";
  cout << "--count=N   = Exit after N client runs (default: run until Ctrl-C)"
       << "\n";
  cout << "\n" << flush;
}

int main(int argc, char **argv)
{
  int port = DRSOCK_UDP_PORT;
  gxString bind_ip;
  unsigned long count = 0;
  for(int i = 1; i < argc; i++) {
    const char *a = argv[i];
    const char *v;
    if(strcmp(a, "-?") == 0 || drbench_switch(a, "help", v)) {
      HelpMessage();
      return 0;
    }
    else if(strncmp(a, "-p", 2) == 0) {
      port = atoi(a + 2);
      if(port < 1024 || port > 65535) {
	cout << "Bad port number specified: " << a + 2 << "\n" << flush;
	return 1;
      }
    }
    else if(strncmp(a, "-l", 2) == 0 && a[2]) bind_ip = a + 2;
    else if(drbench_switch(a, "count", v)) {
      if(!drbench_parse_count(v, 1, 1000000000UL, count)) {
	cout << "Bad --count" << "\n" << flush;
	return 1;
      }
    }
    else {
      cout << "Unknown switch " << a << "\n" << flush;
      HelpMessage();
      return 1;
    }
  }

  gxString err;
  drsock_t s = drsock_udp_open(bind_ip.c_str(), port, err);
  if(s == DRSOCK_BAD) {
    cout << "Error: " << err.c_str() << "\n" << flush;
    return 1;
  }
  drsock_set_buffers(s, 8 * 1024 * 1024);
  cout << "UDP bandwidth benchmark server " << VersionString << " on port "
       << port;
  if(!bind_ip.is_null()) cout << " (" << bind_ip.c_str() << ")";
  cout << "\n" << "Press Ctrl-C to stop the server" << "\n" << flush;

  unsigned char *buf = new unsigned char[DRSOCK_DGRAM_MAX + 1];
  unsigned long reported = 0;
  for(;;) {
    gxString from;
    int from_port = 0;
    int n = drsock_recvfrom(s, buf, DRSOCK_DGRAM_MAX + 1, 1000, from,
			    from_port);
    if(n < 0) {
      cout << "Error: recvfrom: " << drsock_error().c_str() << "\n" << flush;
      break;
    }
    if(n < DRSOCK_DGRAM_HDR) continue; // A timeout or a short datagram
    unsigned long run = drsock_get32(buf + 4);
    unsigned long seq = drsock_get32(buf + 8);
    if(memcmp(buf, "DRBU", 4) == 0) {
      Run *r = FindRun(from, from_port, run, 1);
      if(!r || r->reported) continue;
      double now = drbench_now();
      if(r->received == 0) r->first = now;
      r->last = now;
      r->received++;
      r->bytes += (unsigned long long)n;
      if(seq < r->next_seq) r->out_of_order++;
      else r->next_seq = seq + 1;
    }
    else if(memcmp(buf, "DRBE", 4) == 0) {
      Run *r = FindRun(from, from_port, run, 1);
      if(!r) continue;
      unsigned long sent = seq;
      unsigned char rep[DRSOCK_DGRAM_HDR + 16];
      memcpy(rep, "DRBR", 4);
      drsock_put32(rep + 4, run);
      drsock_put32(rep + 8, r->received);
      drsock_put32(rep + 12, r->out_of_order);
      drsock_put64(rep + 16, r->bytes);
      double span = (r->received > 1) ? r->last - r->first : 0;
      drsock_put64(rep + 24, (unsigned long long)(span * 1e6));
      drsock_sendto(s, rep, sizeof(rep), from.c_str(), from_port);
      if(!r->reported) {
	r->reported = 1;
	reported++;
	double loss = sent ? 100.0 * (double)(sent - (r->received < sent ?
						       r->received : sent))
	  / (double)sent : 0;
	char line[512];
	sprintf(line, "%s:%d run %lu: %lu of %lu datagrams (%.2f%% lost, %lu "
		"out of order), %llu bytes in %.3f seconds, %s",
		from.c_str(), from_port, run, r->received, sent, loss,
		r->out_of_order, r->bytes, span,
		drbench_rate((double)r->bytes, span).c_str());
	cout << line << "\n" << flush;
      }
      if(count && reported >= count) break;
    }
  }
  delete[] buf;
  drsock_close(s);
  cout << reported << " run(s), server exiting" << "\n" << flush;
  return 0;
}
// ----------------------------------------------------------- //
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
