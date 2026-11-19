// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Source Code File Name: tcptest.cpp
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

TCP bandwidth benchmark client for tcpserv. One or more clients (one
thread each) connect to the server at the same time and send it a
number of bytes in writes of the buffer size, or download them with
-d. Each client prints its time and rate when the server has
confirmed the byte count, then the total rate of all clients.

Changes:
11/18/2026: dr_utils version, based on work from the 2002-2019 TCP
benchmark client (bmarks.tcp, then benchmarks/src/tcptest), with the
same switches and defaults (-b1500 -n1 -p53897 -s150M). The server
confirms every byte count, times are wall clock, and a total line
covers all clients. Removed: -k and -r (remote server shutdown and
restart), -c and -f. New: -d (download), a connect timeout, Windows
builds.
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

const char *ProgramName = "tcptest";
const char *VersionString = DR_UTILS_VERSION;

drbenchConsole console;

struct ClientParm {
  int number;
  gxString server;
  int port;
  int block;
  unsigned long long bytes;
  char mode;
  double secs;
  int ok;
};

class ClientThread : public gxThread
{
private:
  void *ThreadEntryRoutine(gxThread_t *thread);
};

void *ClientThread::ThreadEntryRoutine(gxThread_t *thread)
{
  ClientParm *p = (ClientParm *)thread->GetThreadParm();
  p->ok = 0;
  gxString err, m;
  drsock_t s = drsock_tcp_connect(p->server.c_str(), p->port, 10000, err);
  if(s == DRSOCK_BAD) {
    m << "Client[" << p->number << "]: " << err << "\n";
    console.Write(m);
    return 0;
  }
  unsigned char hdr[DRSOCK_TCP_HDR];
  memset(hdr, 0, sizeof(hdr));
  memcpy(hdr, "DRBT", 4);
  hdr[4] = (unsigned char)p->mode;
  drsock_put64(hdr + 8, p->bytes);
  drsock_put32(hdr + 16, (unsigned long)p->block);

  char *buf = new char[p->block];
  memset(buf, 'D', p->block);
  unsigned char ack[8];
  unsigned long long moved = 0;
  const char *status = 0;
  double start = drbench_now();
  if(!drsock_send_all(s, hdr, DRSOCK_TCP_HDR)) {
    status = "could not send the request";
  }
  else if(p->mode == 'U') {
    while(moved < p->bytes) {
      unsigned long long left = p->bytes - moved;
      int n = (left < (unsigned long long)p->block) ? (int)left : p->block;
      if(!drsock_send_all(s, buf, n)) { status = "send failed"; break; }
      moved += (unsigned long long)n;
    }
    if(!status) {
      if(!drsock_recv_all(s, ack, 8, 60000)) {
	status = "no count from the server";
      }
      else if(drsock_get64(ack) != p->bytes) {
	status = "the server counted a different number of bytes";
      }
    }
  }
  else {
    while(moved < p->bytes) {
      unsigned long long left = p->bytes - moved;
      int want = (left < (unsigned long long)p->block) ? (int)left : p->block;
      int n = drsock_recv_some(s, buf, want, 60000);
      if(n <= 0) { status = "the server stopped or timed out"; break; }
      moved += (unsigned long long)n;
    }
    drsock_put64(ack, moved);
    drsock_send_all(s, ack, 8);
  }
  p->secs = drbench_now() - start;
  drsock_close(s);
  delete[] buf;

  char line[512];
  if(status) {
    sprintf(line, "Client[%d]: Error: %s\n", p->number, status);
  }
  else {
    p->ok = 1;
    sprintf(line, "Client[%d]: %s %llu bytes in %.3f seconds, %s\n",
	    p->number, p->mode == 'U' ? "sent" : "received", moved, p->secs,
	    drbench_rate((double)moved, p->secs).c_str());
  }
  console.Write(gxString(line));
  return 0;
}

void HelpMessage()
{
  cout << ProgramName << " version " << VersionString
       << " (dr_utils, DataReel 6.4.1)" << "\n";
  cout << "TCP bandwidth benchmark client (server: tcpserv)" << "\n\n";
  cout << "Usage: " << ProgramName << " [switches] server" << "\n";
  cout << "Switches: " << "\n";
  cout << "-? = Display this help message." << "\n";
  cout << "-b = Set buffer size, ranges = -b64 to -b1048576 (default 1500)"
       << "\n";
  cout << "-d = Download from the server (default: upload to it)" << "\n";
  cout << "-n = Specify a number of clients, ranges -n1 to -n1000" << "\n";
  cout << "-p = Specify a port number, ranges -p1024 to -p65535 (default "
       << DRSOCK_TCP_PORT << ")" << "\n";
  cout << "--datfile=fname = Write the results to a plot data file "
       << "(dr_plot.py)" << "\n";
  cout << "-s = Specify the number of bytes per client, ranges -s1K to -s100G"
       << " (default 150M)" << "\n";
  cout << "\n";
  cout << "Example: " << ProgramName << " -n4 -b65536 -s1G 192.168.1.10"
       << "\n";
  cout << "Exit status: 0 if every client finished, 1 otherwise." << "\n";
  cout << "\n" << flush;
}

int main(int argc, char **argv)
{
  int block = 1500, port = DRSOCK_TCP_PORT;
  unsigned long clients = 1;
  double bytes = 150e6;
  char mode = 'U';
  gxString datfile;
  const char *server = 0;
  for(int i = 1; i < argc; i++) {
    const char *a = argv[i];
    if(strcmp(a, "-?") == 0 || strcmp(a, "--help") == 0) {
      HelpMessage();
      return 0;
    }
    else if(strncmp(a, "-b", 2) == 0) {
      block = atoi(a + 2);
      if(block < 64 || block > 1048576) {
	cout << "Bad buffer size specified: " << a + 2 << "\n" << flush;
	return 1;
      }
    }
    else if(strcmp(a, "-d") == 0) mode = 'D';
    else if(strncmp(a, "--datfile=", 10) == 0 && a[10]) datfile = a + 10;
    else if(strncmp(a, "-n", 2) == 0) {
      if(!drbench_parse_count(a + 2, 1, 1000, clients)) {
	cout << "Bad number of clients specified: " << a + 2 << "\n" << flush;
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
    else if(strncmp(a, "-s", 2) == 0) {
      if(!drbench_parse_size(a + 2, 100e9, bytes) || bytes < 1000) {
	cout << "Bad number of bytes to send specified: " << a + 2 << "\n"
	     << flush;
	return 1;
      }
    }
    else if(a[0] == '-' || server) {
      cout << "Unknown switch " << a << "\n" << flush;
      HelpMessage();
      return 1;
    }
    else server = a;
  }
  if(!server) {
    HelpMessage();
    return 1;
  }
  if(!drsock_init()) {
    cout << "Could not start the socket library" << "\n" << flush;
    return 1;
  }

  cout << "TCP bandwidth benchmark client " << VersionString << "\n";
  cout << "Connecting " << clients << (clients == 1 ? " client" : " clients")
       << " to " << server << " port " << port << ", "
       << drbench_shn(bytes).c_str() << " bytes each, " << block
       << " byte buffers, " << (mode == 'U' ? "upload" : "download")
       << "\n" << flush;

  ClientThread t;
  ClientParm *parms = new ClientParm[clients];
  gxThread_t **threads = new gxThread_t *[clients];
  double start = drbench_now();
  unsigned long i;
  for(i = 0; i < clients; i++) {
    parms[i].number = (int)i;
    parms[i].server = server;
    parms[i].port = port;
    parms[i].block = block;
    parms[i].bytes = (unsigned long long)bytes;
    parms[i].mode = mode;
    parms[i].secs = 0;
    parms[i].ok = 0;
    threads[i] = t.CreateThread((void *)&parms[i]);
    if(!threads[i] || threads[i]->GetThreadError() != gxTHREAD_NO_ERROR) {
      cout << "Error could not start client " << i << "\n" << flush;
      return 1;
    }
  }
  unsigned long ok = 0;
  for(i = 0; i < clients; i++) {
    t.JoinThread(threads[i]);
    if(parms[i].ok) ok++;
    delete threads[i];
  }
  double elapsed = drbench_now() - start;
  double total = (double)(unsigned long long)bytes * ok;
  char line[256];
  sprintf(line, "All tests complete: %lu of %lu clients, %.0f bytes in %.3f"
	  " seconds, %s", ok, clients, total, elapsed,
	  drbench_rate(total, elapsed).c_str());
  cout << line << "\n" << flush;
  if(!datfile.is_null()) {
    drbenchDat dat("tcptest", VersionString, "TCP Bandwidth Benchmark");
    dat.Set("server", server);
    dat.Set("clients", (double)clients);
    dat.Set("bytes", (double)(unsigned long long)bytes);
    dat.Set("block", (double)block);
    dat.Set("direction", mode == 'U' ? "upload" : "download");
    dat.Set("total", total / (elapsed > 0 ? elapsed : 1e-9) / 1e6);
    dat.Set("total_unit", "MB/s");
    dat.Set("total_label", "All clients");
    gxString items, vals;
    for(i = 0; i < clients; i++) {
      char name[32];
      sprintf(name, "Client %lu", i);
      dat.Add(items, name);
      dat.Add(vals, parms[i].ok && parms[i].secs > 0 ?
	      (double)(unsigned long long)bytes / parms[i].secs / 1e6 : 0.0);
    }
    dat.Set("items", items.c_str());
    dat.Set("item_values", vals.c_str());
    dat.Set("item_unit", "MB/s");
    if(!dat.Write(datfile.c_str())) ok = 0;
  }
  delete[] threads;
  delete[] parms;
  return ok == clients ? 0 : 1;
}
// ----------------------------------------------------------- //
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
