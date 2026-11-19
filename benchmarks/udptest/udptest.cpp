// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Source Code File Name: udptest.cpp
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

UDP bandwidth benchmark client for udpserv. One or more clients (one
thread and socket each) send numbered datagrams to the server, as
fast as they can or at a set rate (-r). At the end each client asks
the server what arrived and prints the send rate, the receive rate
and the loss.

Changes:
11/18/2026: dr_utils version, based on Douglas Gaer's 2002-2019 UDP
benchmark client (bmarks.udp, then benchmarks/src/udptest), with the
same switches and defaults (-b1500 -n1 -p53897 -s150M). The server
reports what really arrived, so loss is measured; -r sets a send
rate. Removed: -k and -r as remote server shutdown and restart (-r
is now the rate). New: Windows builds.
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

const char *ProgramName = "udptest";
const char *VersionString = DR_UTILS_VERSION;

drbenchConsole console;

struct ClientParm {
  int number;
  gxString server_ip;
  int port;
  int dgram;
  unsigned long count;  // Datagrams to send
  double rate_bps;      // 0 = no limit
  unsigned long run;
  double secs;
  unsigned long received;
  unsigned long long recv_bytes;
  double span;          // First to last datagram at the server
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
  gxString err;
  drsock_t s = drsock_udp_open(0, 0, err);
  if(s == DRSOCK_BAD) {
    gxString m;
    m << "Client[" << p->number << "]: " << err << "\n";
    console.Write(m);
    return 0;
  }
  drsock_set_buffers(s, 4 * 1024 * 1024);
  unsigned char *buf = new unsigned char[p->dgram];
  memset(buf, 'D', p->dgram);
  memcpy(buf, "DRBU", 4);
  drsock_put32(buf + 4, p->run);
  drsock_put32(buf + 12, p->count);

  drbenchPacer pacer(p->rate_bps);
  double start = drbench_now();
  unsigned long sent = 0;
  for(unsigned long i = 0; i < p->count; i++) {
    drsock_put32(buf + 8, i);
    if(drsock_sendto(s, buf, p->dgram, p->server_ip.c_str(), p->port) < 0) {
      break;
    }
    sent++;
    pacer.Sent((double)p->dgram);
  }
  p->secs = drbench_now() - start;

  // End of run, until the server reports (up to 10 tries)
  unsigned char end[DRSOCK_DGRAM_HDR];
  memcpy(end, "DRBE", 4);
  drsock_put32(end + 4, p->run);
  drsock_put32(end + 8, sent);
  drsock_put32(end + 12, p->count);
  unsigned char rep[64];
  int got_report = 0;
  unsigned long out_of_order = 0;
  double span = 0;
  for(int tries = 0; tries < 10 && !got_report; tries++) {
    drsock_sendto(s, end, sizeof(end), p->server_ip.c_str(), p->port);
    double until = drbench_now() + 0.5;
    while(drbench_now() < until) {
      gxString from;
      int from_port;
      int n = drsock_recvfrom(s, rep, sizeof(rep), 100, from, from_port);
      if(n >= DRSOCK_DGRAM_HDR + 16 && memcmp(rep, "DRBR", 4) == 0 &&
	 drsock_get32(rep + 4) == p->run) {
	p->received = drsock_get32(rep + 8);
	out_of_order = drsock_get32(rep + 12);
	p->recv_bytes = drsock_get64(rep + 16);
	span = (double)drsock_get64(rep + 24) / 1e6;
	p->span = span;
	got_report = 1;
	break;
      }
    }
  }
  drsock_close(s);
  delete[] buf;

  char line[512];
  double bytes_sent = (double)sent * p->dgram;
  if(!got_report) {
    sprintf(line, "Client[%d]: sent %lu datagrams (%s) in %.3f seconds, %s;"
	    " no report from the server\n", p->number, sent,
	    drbench_shn(bytes_sent).c_str(), p->secs,
	    drbench_rate(bytes_sent, p->secs).c_str());
  }
  else {
    p->ok = (sent == p->count);
    double loss = sent ? 100.0 * (double)(sent - (p->received < sent ?
						  p->received : sent))
      / (double)sent : 0;
    sprintf(line, "Client[%d]: sent %lu datagrams in %.3f seconds, %s; "
	    "server received %lu (%.2f%% lost, %lu out of order), %s\n",
	    p->number, sent, p->secs, drbench_rate(bytes_sent, p->secs).c_str(),
	    p->received, loss, out_of_order,
	    drbench_rate((double)p->recv_bytes, span).c_str());
  }
  console.Write(gxString(line));
  return 0;
}

void HelpMessage()
{
  cout << ProgramName << " version " << VersionString
       << " (dr_utils, DataReel 6.4.1)" << "\n";
  cout << "UDP bandwidth benchmark client (server: udpserv)" << "\n\n";
  cout << "Usage: " << ProgramName << " [switches] server" << "\n";
  cout << "Switches: " << "\n";
  cout << "-? = Display this help message." << "\n";
  cout << "-b = Set datagram size, ranges = -b" << DRSOCK_DGRAM_MIN << " to -b"
       << DRSOCK_DGRAM_MAX << " (default 1500)" << "\n";
  cout << "-n = Specify a number of clients, ranges -n1 to -n1000" << "\n";
  cout << "-p = Specify a port number, ranges -p1024 to -p65535 (default "
       << DRSOCK_UDP_PORT << ")" << "\n";
  cout << "-r = Send rate per client in bits a second: -r100M -r1G "
       << "(default: no limit)" << "\n";
  cout << "-s = Specify the number of bytes per client, ranges -s1K to -s100G"
       << " (default 150M)" << "\n";
  cout << "\n";
  cout << "With no rate limit the clients send as fast as the host can, and"
       << "\n" << "the loss shows what the network and the server dropped."
       << "\n";
  cout << "Exit status: 0 if every client got a report, 1 otherwise." << "\n";
  cout << "\n" << flush;
}

int main(int argc, char **argv)
{
  int dgram = 1500, port = DRSOCK_UDP_PORT;
  unsigned long clients = 1;
  double bytes = 150e6, rate = 0;
  const char *server = 0;
  for(int i = 1; i < argc; i++) {
    const char *a = argv[i];
    if(strcmp(a, "-?") == 0 || strcmp(a, "--help") == 0) {
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
    else if(strncmp(a, "-r", 2) == 0) {
      if(!drbench_parse_size(a + 2, 1e12, rate)) {
	cout << "Bad rate specified: " << a + 2 << "\n" << flush;
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
  gxString ip, err;
  if(!drsock_resolve(server, ip, err)) {
    cout << err.c_str() << "\n" << flush;
    return 1;
  }
  unsigned long count = (unsigned long)(bytes / dgram);
  if(count < 1) count = 1;

  cout << "UDP bandwidth benchmark client " << VersionString << "\n";
  cout << clients << (clients == 1 ? " client" : " clients") << " to "
       << ip.c_str() << " port " << port << ", " << count << " datagrams of "
       << dgram << " bytes each";
  if(rate > 0) cout << ", " << drbench_shn(rate).c_str() << "bit/s per client";
  cout << "\n" << flush;

  ClientThread t;
  ClientParm *parms = new ClientParm[clients];
  gxThread_t **threads = new gxThread_t *[clients];
  unsigned long run_base = (unsigned long)(drbench_now() * 1000.0) & 0x7FFFFFFFUL;
  unsigned long i;
  for(i = 0; i < clients; i++) {
    parms[i].number = (int)i;
    parms[i].server_ip = ip;
    parms[i].port = port;
    parms[i].dgram = dgram;
    parms[i].count = count;
    parms[i].rate_bps = rate;
    parms[i].run = run_base + i;
    parms[i].secs = 0;
    parms[i].received = 0;
    parms[i].recv_bytes = 0;
    parms[i].span = 0;
    parms[i].ok = 0;
    threads[i] = t.CreateThread((void *)&parms[i]);
    if(!threads[i] || threads[i]->GetThreadError() != gxTHREAD_NO_ERROR) {
      cout << "Error could not start client " << i << "\n" << flush;
      return 1;
    }
  }
  unsigned long ok = 0, received = 0;
  unsigned long long recv_bytes = 0;
  double span = 0;
  for(i = 0; i < clients; i++) {
    t.JoinThread(threads[i]);
    if(parms[i].ok) ok++;
    received += parms[i].received;
    recv_bytes += parms[i].recv_bytes;
    if(parms[i].span > span) span = parms[i].span;
    delete threads[i];
  }
  double sent_total = (double)count * clients;
  char line[256];
  sprintf(line, "All tests complete: %lu of %lu clients reported, %lu of %.0f"
	  " datagrams received (%.2f%% lost), %s received", ok, clients,
	  received, sent_total,
	  sent_total ? 100.0 * (sent_total - (double)received) / sent_total : 0,
	  drbench_rate((double)recv_bytes, span).c_str());
  cout << line << "\n" << flush;
  delete[] threads;
  delete[] parms;
  return ok == clients ? 0 : 1;
}
// ----------------------------------------------------------- //
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
