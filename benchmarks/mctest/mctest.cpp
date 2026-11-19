// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Source Code File Name: mctest.cpp
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

Multicast benchmark receiver for mcserv. One or more receivers (one
thread and socket each) join the multicast group and count the
datagrams of one mcserv run: received, lost (from the sequence
numbers), out of order, and the rate from the first to the last.
Each receiver stops at the end of the run, or when nothing arrives
for --timeout seconds.

Changes:
11/18/2026: dr_utils version, based on Douglas Gaer's 2002-2019
multicast benchmark client (bmarks.mc, then benchmarks/src/
multicasttest/mctest), with the same -b -g -n -p switches and port
(53898). Loss and order are counted with mcserv's sequence numbers,
and the run ends by itself. New: -i, --timeout, Windows builds.
*/
// ----------------------------------------------------------- //
#include "gxdlcode.h"

#include <iostream>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gxthread.h"
#include "drsock.h"
#include "drbench.h"
#include "drutil.h"

const char *ProgramName = "mctest";
const char *VersionString = DR_UTILS_VERSION;

drbenchConsole console;

struct RecvParm {
  int number;
  gxString group;
  gxString iface;
  int port;
  int dgram;
  int timeout_ms;
  unsigned long received, expected, out_of_order;
  unsigned long long bytes;
  double span;
  int ok;
};

class RecvThread : public gxThread
{
private:
  void *ThreadEntryRoutine(gxThread_t *thread);
};

void *RecvThread::ThreadEntryRoutine(gxThread_t *thread)
{
  RecvParm *p = (RecvParm *)thread->GetThreadParm();
  p->ok = 0;
  gxString err, m;
  drsock_t s = drsock_udp_open(0, p->port, err);
  if(s == DRSOCK_BAD ||
     !drsock_mc_join(s, p->group.c_str(), p->iface.c_str(), err)) {
    m << "Receiver[" << p->number << "]: " << err << "\n";
    console.Write(m);
    drsock_close(s);
    return 0;
  }
  drsock_set_buffers(s, 8 * 1024 * 1024);
  m << "Receiver[" << p->number << "]: joined multicast group " << p->group
    << " port " << p->port << "\n";
  console.Write(m);

  unsigned char *buf = new unsigned char[DRSOCK_DGRAM_MAX + 1];
  int have_run = 0, ended = 0;
  unsigned long run = 0, next_seq = 0, sent = 0;
  unsigned long long bytes = 0;
  double first = 0, last = 0;
  p->received = p->expected = p->out_of_order = 0;
  for(;;) {
    gxString from;
    int from_port;
    int n = drsock_recvfrom(s, buf, DRSOCK_DGRAM_MAX + 1, p->timeout_ms,
			    from, from_port);
    if(n < 0) {
      m << clear << "Receiver[" << p->number << "]: recvfrom: "
	<< drsock_error() << "\n";
      console.Write(m);
      break;
    }
    if(n == 0) break; // Nothing for the timeout
    if(n < DRSOCK_DGRAM_HDR) continue;
    unsigned long r = drsock_get32(buf + 4);
    unsigned long seq = drsock_get32(buf + 8);
    if(!have_run) { // The first run seen is the one counted
      if(memcmp(buf, "DRBU", 4) != 0) continue;
      have_run = 1;
      run = r;
      first = drbench_now();
    }
    if(r != run) continue;
    if(memcmp(buf, "DRBE", 4) == 0) {
      sent = seq;
      ended = 1;
      break;
    }
    if(memcmp(buf, "DRBU", 4) != 0) continue;
    last = drbench_now();
    p->received++;
    bytes += (unsigned long long)n;
    p->expected = drsock_get32(buf + 12);
    if(seq < next_seq) p->out_of_order++;
    else next_seq = seq + 1;
  }
  drsock_close(s);
  delete[] buf;

  char line[512];
  if(!have_run) {
    sprintf(line, "Receiver[%d]: no datagrams in %d seconds\n", p->number,
	    p->timeout_ms / 1000);
  }
  else {
    if(!ended) sent = next_seq; // No end of run: count up to the last seen
    p->expected = sent;
    double span = last - first;
    p->bytes = bytes;
    p->span = span;
    double loss = sent ? 100.0 * (double)(sent - (p->received < sent ?
						  p->received : sent))
      / (double)sent : 0;
    p->ok = ended;
    sprintf(line, "Receiver[%d]: run %lu: %lu of %lu datagrams (%.2f%% lost, "
	    "%lu out of order), %llu bytes in %.3f seconds, %s%s\n",
	    p->number, run, p->received, sent, loss, p->out_of_order, bytes,
	    span, drbench_rate((double)bytes, span).c_str(),
	    ended ? "" : " (no end of run, timed out)");
  }
  console.Write(gxString(line));
  return 0;
}

void HelpMessage()
{
  cout << ProgramName << " version " << VersionString
       << " (dr_utils, DataReel 6.4.1)" << "\n";
  cout << "Multicast benchmark receiver (sender: mcserv)" << "\n\n";
  cout << "Usage: " << ProgramName << " [switches] [group]" << "\n";
  cout << "Switches: " << "\n";
  cout << "-?           = Display this help message." << "\n";
  cout << "-g[group]    = Multicast group (default " << DRSOCK_MC_GROUP << ")"
       << "\n";
  cout << "-i[address]  = Join on the interface with this IPv4 address"
       << "\n";
  cout << "-n[number]   = Number of receivers, -n1 to -n100" << "\n";
  cout << "-p[port]     = Port number, -p1024 to -p65535 (default "
       << DRSOCK_MC_PORT << ")" << "\n";
  cout << "--datfile=fname = Write the results to a plot data file "
       << "(dr_plot.py)" << "\n";
  cout << "--timeout=S  = Stop when nothing arrives for S seconds (default "
       << "60)" << "\n";
  cout << "\n";
  cout << "Start mctest, then mcserv. Allow UDP port " << DRSOCK_MC_PORT
       << " in the host firewall." << "\n";
  cout << "Exit status: 0 if every receiver saw the end of a run, 1 "
       << "otherwise." << "\n";
  cout << "\n" << flush;
}

int main(int argc, char **argv)
{
  int port = DRSOCK_MC_PORT;
  unsigned long receivers = 1, timeout = 60;
  gxString group = DRSOCK_MC_GROUP, iface, datfile;
  for(int i = 1; i < argc; i++) {
    const char *a = argv[i];
    const char *v;
    if(strcmp(a, "-?") == 0 || drbench_switch(a, "help", v)) {
      HelpMessage();
      return 0;
    }
    else if(strncmp(a, "-b", 2) == 0) {
      // Accepted for the old command lines: datagrams of any size are read
    }
    else if(strncmp(a, "-g", 2) == 0) group = a + 2;
    else if(strncmp(a, "-i", 2) == 0 && a[2]) iface = a + 2;
    else if(strncmp(a, "-n", 2) == 0) {
      if(!drbench_parse_count(a + 2, 1, 100, receivers)) {
	cout << "Bad number of receivers specified: " << a + 2 << "\n"
	     << flush;
	return 1;
      }
    }
    else if(strncmp(a, "-p", 2) == 0) {
      port = atoi(a + 2);
      if(port < 1024 || port > 65535) {
	cout << "Bad port number specified: " << a + 2 << "\n" << flush;
	return 1;
      }
    }
    else if(drbench_switch(a, "timeout", v)) {
      if(!drbench_parse_count(v, 1, 86400, timeout)) {
	cout << "Bad --timeout" << "\n" << flush;
	return 1;
      }
    }
    else if(drbench_switch(a, "datfile", v) && v && *v) datfile = v;
    else if(a[0] != '-') group = a;
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
  if(!drsock_init()) {
    cout << "Could not start the socket library" << "\n" << flush;
    return 1;
  }
  cout << "Multicast benchmark receiver " << VersionString << "\n" << flush;

  RecvThread t;
  RecvParm *parms = new RecvParm[receivers];
  gxThread_t **threads = new gxThread_t *[receivers];
  unsigned long i;
  for(i = 0; i < receivers; i++) {
    parms[i].number = (int)i;
    parms[i].group = group;
    parms[i].iface = iface;
    parms[i].port = port;
    parms[i].timeout_ms = (int)timeout * 1000;
    parms[i].ok = 0;
    parms[i].bytes = 0;
    parms[i].span = 0;
    parms[i].received = parms[i].expected = parms[i].out_of_order = 0;
    threads[i] = t.CreateThread((void *)&parms[i]);
    if(!threads[i] || threads[i]->GetThreadError() != gxTHREAD_NO_ERROR) {
      cout << "Error could not start receiver " << i << "\n" << flush;
      return 1;
    }
  }
  unsigned long ok = 0;
  for(i = 0; i < receivers; i++) {
    t.JoinThread(threads[i]);
    if(parms[i].ok) ok++;
    delete threads[i];
  }
  cout << "All tests complete: " << ok << " of " << receivers
       << " receivers saw the end of a run" << "\n" << flush;
  if(!datfile.is_null()) {
    drbenchDat dat("mctest", VersionString, "Multicast Benchmark");
    dat.Set("group", group.c_str());
    dat.Set("port", (double)port);
    dat.Set("receivers", (double)receivers);
    gxString items, vals, loss;
    double sum = 0;
    for(i = 0; i < receivers; i++) {
      char name[32];
      sprintf(name, "Receiver %lu", i);
      dat.Add(items, name);
      double r = parms[i].span > 0 ? (double)parms[i].bytes / parms[i].span / 1e6 : 0;
      sum += r;
      dat.Add(vals, r);
      unsigned long e = parms[i].expected, g = parms[i].received;
      dat.Add(loss, e ? 100.0 * (double)(e - (g < e ? g : e)) / (double)e : 0.0);
    }
    dat.Set("total", receivers ? sum / receivers : 0.0);
    dat.Set("total_unit", "MB/s");
    dat.Set("total_label", "Average per receiver");
    dat.Set("items", items.c_str());
    dat.Set("item_values", vals.c_str());
    dat.Set("item_unit", "MB/s");
    dat.Set("item2_values", loss.c_str());
    dat.Set("item2_unit", "% lost");
    if(!dat.Write(datfile.c_str())) ok = 0;
  }
  delete[] threads;
  delete[] parms;
  return ok == receivers ? 0 : 1;
}
// ----------------------------------------------------------- //
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
