// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Header File Name: drsock.h
// C++ Compiler Used: MSVC, GCC
// Produced By: DataReel Software Development Team
// File Creation Date: 11/18/2026
// Date Last Modified: 11/18/2026
// Copyright (c) 2001-2026 DataReel Software Development
// ----------------------------------------------------------- //
// ---------- Include File Description and Details  ---------- //
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

IPv4 TCP, UDP and multicast sockets for the dr_utils network
benchmarks (tcpserv/tcptest, udpserv/udptest, mcserv/mctest), and
the wire formats they share.

Like drnet.h this header has no socket headers in it. drsock.cpp
uses Winsock 2 on Windows, so multicast options use the Winsock 2
values (DataReel 6.x links the Winsock 1 library).

Changes:
11/18/2026: First version for dr_utils.
*/
// ----------------------------------------------------------- //
#ifndef __DR_UTILS_SOCK_HPP__
#define __DR_UTILS_SOCK_HPP__

#include "gxdlcode.h"
#include "gxstring.h"

typedef long long drsock_t;            // A socket handle
const drsock_t DRSOCK_BAD = -1;

// Default ports and multicast group of the 2002-2019 benchmarks
const int DRSOCK_TCP_PORT = 53897;
const int DRSOCK_UDP_PORT = 53897;
const int DRSOCK_MC_PORT = 53898;
const char * const DRSOCK_MC_GROUP = "239.255.10.10";

// Start the socket library (Windows WSAStartup). Returns 1 if ready.
int drsock_init();

// The last socket error as text
gxString drsock_error();

void drsock_close(drsock_t s);

// TCP server socket on "bind_ip" (0 or "0.0.0.0" for every address)
drsock_t drsock_tcp_listen(const char *bind_ip, int port, gxString &err);

// Wait up to "timeout_ms" for a connection (-1 = forever). Returns the
// connected socket, or DRSOCK_BAD on a timeout (err empty) or error.
// Sets "peer" to the client address.
drsock_t drsock_accept(drsock_t ls, int timeout_ms, gxString &peer,
		       gxString &err);

// TCP connection to a host name or address, with a connect timeout
drsock_t drsock_tcp_connect(const char *host, int port, int timeout_ms,
			    gxString &err);

// Send all bytes. Returns 1, or 0 on an error.
int drsock_send_all(drsock_t s, const void *buf, int len);

// Receive exactly "len" bytes, waiting up to "timeout_ms" between
// reads (-1 = forever). Returns 1, or 0 on an error, close or timeout.
int drsock_recv_all(drsock_t s, void *buf, int len, int timeout_ms);

// Receive up to "len" bytes. Returns the count, 0 at the end of the
// stream, or -1 on an error or timeout.
int drsock_recv_some(drsock_t s, void *buf, int len, int timeout_ms);

// Set the socket send and receive buffer sizes (best effort)
void drsock_set_buffers(drsock_t s, int bytes);

// UDP socket bound to "bind_ip":"port" (port 0 = any port)
drsock_t drsock_udp_open(const char *bind_ip, int port, gxString &err);

// A dotted IPv4 address for a host name or address
int drsock_resolve(const char *host, gxString &ip, gxString &err);

// Send one datagram to "ip":"port". Returns the bytes sent or -1.
int drsock_sendto(drsock_t s, const void *buf, int len, const char *ip,
		  int port);

// Receive one datagram, waiting up to "timeout_ms" (-1 = forever).
// Returns the length, 0 on a timeout or -1 on an error. Sets the
// sender's address and port.
int drsock_recvfrom(drsock_t s, void *buf, int len, int timeout_ms,
		    gxString &from_ip, int &from_port);

// Multicast: join "group" on the interface with address "iface_ip"
// (0 = the system's choice). Returns 1 or sets "err".
int drsock_mc_join(drsock_t s, const char *group, const char *iface_ip,
		   gxString &err);

// Multicast sending: TTL, loopback to this host, and the interface
int drsock_mc_sender(drsock_t s, int ttl, int loopback,
		     const char *iface_ip, gxString &err);

// True for a 224.0.0.0 to 239.255.255.255 address
int drsock_is_multicast(const char *ip);

// Network byte order helpers for the wire formats
void drsock_put32(unsigned char *p, unsigned long v);
unsigned long drsock_get32(const unsigned char *p);
void drsock_put64(unsigned char *p, unsigned long long v);
unsigned long long drsock_get64(const unsigned char *p);

// ----------------------------------------------------------- //
// Wire formats
// ----------------------------------------------------------- //
//
// TCP (tcptest -> tcpserv), one transfer per connection:
//   request, 24 bytes: "DRBT" mode(1: 'U' upload or 'D' download)
//                      0 0 0  bytes(8)  block(4)  0 0 0 0
//   'U': the client sends "bytes", the server answers with the
//        8-byte count it received.
//   'D': the server sends "bytes" in "block" writes, the client
//        answers with the 8-byte count it received.
const int DRSOCK_TCP_HDR = 24;

// UDP (udptest -> udpserv) and multicast (mcserv -> mctest)
// datagrams start with a 16-byte header:
//   magic(4) run(4) seq(4) total(4)
//   magic "DRBU" data, "DRBE" end of run (seq = datagrams sent),
//         "DRBR" report from udpserv: seq = datagrams received,
//         total = out of order, then bytes(8) and microseconds(8)
//         from the first to the last datagram.
const int DRSOCK_DGRAM_HDR = 16;
const int DRSOCK_DGRAM_MIN = 32;      // Smallest datagram size
const int DRSOCK_DGRAM_MAX = 65507;   // Largest IPv4 UDP payload

#endif // __DR_UTILS_SOCK_HPP__
// ----------------------------------------------------------- //
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
