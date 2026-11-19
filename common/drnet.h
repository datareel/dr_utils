// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Header File Name: drnet.h
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

Network helpers shared by the dr_utils network tools (pingscan,
dnsscan, portscan, netpscan, arpscan, getmac, internet-test).

This header has no socket headers in it, so a tool can include it
next to any DataReel header. On Windows drnet.cpp includes
winsock2.h before the DataReel headers.

- Ping: an ICMP echo with a timeout. Linux first tries an
  unprivileged ICMP datagram socket (net.ipv4.ping_group_range), then
  a raw socket (root or CAP_NET_RAW). Windows uses IcmpSendEcho, which
  needs no administrator rights.
- TCP connect with a timeout (non-blocking connect), so a filtered
  port costs the timeout, not the system's two minute connect wait.
- Forward and reverse DNS (getaddrinfo and getnameinfo).
- The MAC address of a host on the local network: /proc/net/arp on
  Linux, SendARP on Windows.
- Address ranges: a.b.c (a.b.c.1 to a.b.c.254), a.b.c.d, or
  a.b.c.x-y. The tools scan one /24 at most.
- A services file reader (name port/protocol aliases # comment).

Changes:
11/18/2026: First version for dr_utils, based on work from the network code of
the 1999-2010 scan programs (pingscan, dnsscan, portscan,
netpscan, arpscan, getmac).
*/
// ----------------------------------------------------------- //
#ifndef __DR_UTILS_NET_HPP__
#define __DR_UTILS_NET_HPP__

#include "gxdlcode.h"
#include "gxstring.h"

// Ping results
const int DRNET_REPLY = 1;     // Echo reply received
const int DRNET_NO_REPLY = 0;  // No reply in the timeout
const int DRNET_ERROR = -1;    // Could not send (see the error string)

// Start the socket library (Windows WSAStartup). Returns 1 if ready.
int drnet_init();

// Resolve a host name or dotted address to a dotted IPv4 address.
// Returns 1 and sets "ip", or 0 and sets "err".
int drnet_resolve(const char *host, gxString &ip, gxString &err);

// Reverse DNS name of a dotted IPv4 address. Returns 1 and sets
// "name", or 0 if the address has no name.
int drnet_reverse(const char *ip, gxString &name);

// All names and addresses of a host, as "Hostname:", "Host Alias:"
// and "Host IP Address:" lines. Returns 0 if the host is unknown.
int drnet_host_info(const char *host, gxString &info);

// Send one ICMP echo request to a dotted IPv4 address and wait up to
// "timeout_ms" for the reply. Sets "rtt_ms" on a reply and "err" on
// DRNET_ERROR.
int drnet_ping(const char *ip, int timeout_ms, double &rtt_ms,
	       gxString &err);

// Connect to a TCP port with a timeout. Returns 1 if the port took
// the connection, 0 if it is closed or did not answer in time, or
// DRNET_ERROR (sets "err") if no socket could be made.
int drnet_tcp_connect(const char *ip, int port, int timeout_ms,
		      gxString &err);

// MAC address of a host on a local network, as XX:XX:XX:XX:XX:XX.
// Ping the host first so the system has an ARP entry. "dev" is an
// optional interface name (Linux only, ignored on Windows). Returns 1
// if found.
int drnet_mac_address(const char *ip, const char *dev, gxString &mac);

// An address range: a.b.c, a.b.c.d or a.b.c.x-y. Sets "base"
// ("a.b.c"), "first" and "last". Returns 0 and sets "err" if the
// range is not valid.
int drnet_parse_range(const char *range, gxString &base, int &first,
		      int &last, gxString &err);

// One services file entry
struct drnetService {
  int port;
  char protocol[16];
  char name[64];
  char aliases[128];
  char comment[128];
};

// Parse one services file line. Returns 1 for an entry, 0 for a
// comment, blank or malformed line.
int drnet_parse_service(const char *line, drnetService &s);

// Read the TCP entries of a services file, one per port (the first
// entry for a port wins), in file order. Returns the number read into
// "out" (at most "max"), or -1 if the file cannot be opened.
int drnet_load_tcp_services(const char *fname, drnetService *out, int max);

// The services entry for a TCP port in a list from
// drnet_load_tcp_services(), or 0
const drnetService *drnet_find_service(const drnetService *list, int n,
				       int port);

// The default services file for this system
const char *drnet_default_services();

#endif // __DR_UTILS_NET_HPP__
// ----------------------------------------------------------- //
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
