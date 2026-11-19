// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Source Code File Name: tcpserv.cpp
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

TCP bandwidth benchmark server for tcptest. Each client connection
is one transfer in a thread of its own: the client uploads a number
of bytes and the server answers with the count it received, or the
server sends the bytes (download). One line per transfer: client,
direction, bytes, seconds and rate.

Changes:
11/18/2026: dr_utils version, based on work from the 2002-2019 TCP
benchmark server (bmarks.tcp, then benchmarks/src/tcptest/tcpserv),
on the same default port (53897). The clients can no longer shut
down or restart the server over the network (the old -k and -r);
stop it with Ctrl-C, or let --count end it. New: downloads, an idle
timeout, a limit on clients and transfer size, -l to listen on one
address, Windows builds.
*/
// ----------------------------------------------------------- //
#include "gxdlcode.h"

#include <iostream>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gxthread.h"
#include "gxmutex.h"
#include "drsock.h"
#include "drbench.h"
#include "drutil.h"

const char *ProgramName = "tcpserv";
const char *VersionString = DR_UTILS_VERSION;
const int MaxClients = 256;
const int BlockMax = 1024 * 1024;

drbenchConsole console;
int idle_timeout_ms = 30000;
unsigned long long max_bytes = 100000000000ULL; // 100g

struct ClientSlot {
  int in_use;
  int done;
  drsock_t s;
  gxString peer;
  gxThread_t *thread;
};

ClientSlot slots[MaxClients];
gxMutex slot_lock;
unsigned long transfers_done = 0;

class ClientThread : public gxThread
{
private:
  void *ThreadEntryRoutine(gxThread_t *thread);
};

void Report(const ClientSlot *c, char mode, unsigned long long bytes,
	    double secs, const char *status)
{
  char line[512];
  sprintf(line, "%s %s %llu bytes in %.3f seconds, %s%s%s\n",
	  c->peer.c_str(), mode == 'U' ? "upload" : "download", bytes, secs,
	  drbench_rate((double)bytes, secs).c_str(), status ? " - " : "",
	  status ? status : "");
  console.Write(gxString(line));
}

void *ClientThread::ThreadEntryRoutine(gxThread_t *thread)
{
  ClientSlot *c = (ClientSlot *)thread->GetThreadParm();
  unsigned char hdr[DRSOCK_TCP_HDR];
  char *buf = 0;
  if(!drsock_recv_all(c->s, hdr, DRSOCK_TCP_HDR, idle_timeout_ms) ||
     memcmp(hdr, "DRBT", 4) != 0 || (hdr[4] != 'U' && hdr[4] != 'D')) {
    { gxString m; m << c->peer << " bad or missing request, closed\n"; console.Write(m); }
  }
  else {
    char mode = (char)hdr[4];
    unsigned long long bytes = drsock_get64(hdr + 8);
    int block = (int)drsock_get32(hdr + 16);
    if(block < 1 || block > BlockMax) block = 65536;
    if(bytes == 0 || bytes > max_bytes) {
      gxString m;
      m << c->peer << " transfer size out of range, closed\n";
      console.Write(m);
    }
    else {
      buf = new char[block];
      memset(buf, 'D', block);
      unsigned long long moved = 0;
      double start = drbench_now();
      const char *status = 0;
      unsigned char ack[8];
      if(mode == 'U') {
	while(moved < bytes) {
	  unsigned long long left = bytes - moved;
	  int want = (left < (unsigned long long)block) ? (int)left : block;
	  int n = drsock_recv_some(c->s, buf, want, idle_timeout_ms);
	  if(n <= 0) { status = "client stopped or timed out"; break; }
	  moved += (unsigned long long)n;
	}
	drsock_put64(ack, moved);
	drsock_send_all(c->s, ack, 8);
      }
      else {
	while(moved < bytes) {
	  unsigned long long left = bytes - moved;
	  int n = (left < (unsigned long long)block) ? (int)left : block;
	  if(!drsock_send_all(c->s, buf, n)) { status = "send failed"; break; }
	  moved += (unsigned long long)n;
	}
	if(!status && (!drsock_recv_all(c->s, ack, 8, idle_timeout_ms) ||
		       drsock_get64(ack) != bytes)) {
	  status = "client did not confirm the count";
	}
      }
      Report(c, mode, moved, drbench_now() - start, status);
    }
  }
  delete[] buf;
  drsock_close(c->s);
  slot_lock.MutexLock();
  c->done = 1;
  transfers_done++;
  slot_lock.MutexUnlock();
  return 0;
}

// Join the threads that finished. With "all", wait for every one.
void ReapClients(ClientThread &t, int all)
{
  for(int i = 0; i < MaxClients; i++) {
    slot_lock.MutexLock();
    int ready = slots[i].in_use && (slots[i].done || all);
    slot_lock.MutexUnlock();
    if(!ready) continue;
    t.JoinThread(slots[i].thread);
    delete slots[i].thread;
    slots[i].thread = 0;
    slots[i].in_use = 0;
  }
}

void HelpMessage()
{
  cout << ProgramName << " version " << VersionString
       << " (dr_utils, DataReel 6.4.1)" << "\n";
  cout << "TCP bandwidth benchmark server (clients: tcptest)" << "\n\n";
  cout << "Usage: " << ProgramName << " [switches]" << "\n";
  cout << "Switches: " << "\n";
  cout << "-?          = Display this help message." << "\n";
  cout << "-p[port]    = Port number, -p1024 to -p65535 (default "
       << DRSOCK_TCP_PORT << ")" << "\n";
  cout << "-l[address] = Listen on this IPv4 address only (default: all)"
       << "\n";
  cout << "--count=N   = Exit after N transfers (default: run until Ctrl-C)"
       << "\n";
  cout << "--timeout=S = Close a client idle for S seconds (default 30)"
       << "\n";
  cout << "--max-bytes=SIZE = Largest transfer accepted (default 100g)"
       << "\n";
  cout << "\n";
  cout << "Allow the port in the host firewall for tests from other hosts."
       << "\n";
  cout << "\n" << flush;
}

int main(int argc, char **argv)
{
  int port = DRSOCK_TCP_PORT;
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
    else if(drbench_switch(a, "timeout", v)) {
      unsigned long sec;
      if(!drbench_parse_count(v, 1, 86400, sec)) {
	cout << "Bad --timeout" << "\n" << flush;
	return 1;
      }
      idle_timeout_ms = (int)sec * 1000;
    }
    else if(drbench_switch(a, "max-bytes", v)) {
      double d;
      if(!drbench_parse_size(v, 1e15, d)) {
	cout << "Bad --max-bytes" << "\n" << flush;
	return 1;
      }
      max_bytes = (unsigned long long)d;
    }
    else {
      cout << "Unknown switch " << a << "\n" << flush;
      HelpMessage();
      return 1;
    }
  }

  gxString err;
  drsock_t ls = drsock_tcp_listen(bind_ip.c_str(), port, err);
  if(ls == DRSOCK_BAD) {
    cout << "Error: " << err.c_str() << "\n" << flush;
    return 1;
  }
  cout << "TCP bandwidth benchmark server " << VersionString << " on port "
       << port;
  if(!bind_ip.is_null()) cout << " (" << bind_ip.c_str() << ")";
  cout << "\n" << "Press Ctrl-C to stop the server" << "\n" << flush;

  ClientThread t;
  unsigned long accepted = 0;
  for(;;) {
    ReapClients(t, 0);
    if(count) {
      slot_lock.MutexLock();
      unsigned long done = transfers_done;
      slot_lock.MutexUnlock();
      if(done >= count) break;
      if(accepted >= count) {
	ReapClients(t, 1);
	continue;
      }
    }
    gxString peer;
    drsock_t c = drsock_accept(ls, 500, peer, err);
    if(c == DRSOCK_BAD) {
      if(!err.is_null()) {
	cout << "Error: " << err.c_str() << "\n" << flush;
	break;
      }
      continue;
    }
    int slot = -1;
    slot_lock.MutexLock();
    for(int i = 0; i < MaxClients; i++) {
      if(!slots[i].in_use) { slot = i; break; }
    }
    if(slot >= 0) {
      slots[slot].in_use = 1;
      slots[slot].done = 0;
      slots[slot].s = c;
      slots[slot].peer = peer;
    }
    slot_lock.MutexUnlock();
    if(slot < 0) {
      gxString m;
      m << peer << " refused: " << MaxClients << " clients already\n";
      console.Write(m);
      drsock_close(c);
      continue;
    }
    accepted++;
    slots[slot].thread = t.CreateThread((void *)&slots[slot]);
    if(!slots[slot].thread ||
       slots[slot].thread->GetThreadError() != gxTHREAD_NO_ERROR) {
      cout << "Error could not start a client thread" << "\n" << flush;
      drsock_close(c);
      slots[slot].in_use = 0;
    }
  }
  ReapClients(t, 1);
  drsock_close(ls);
  cout << transfers_done << " transfer(s), server exiting" << "\n" << flush;
  return 0;
}
// ----------------------------------------------------------- //
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
