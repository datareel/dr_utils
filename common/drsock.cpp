// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Source Code File Name: drsock.cpp
// C++ Compiler Used: MSVC, GCC
// Produced By: DataReel Software Development Team
// File Creation Date: 11/18/2026
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

IPv4 sockets for the dr_utils network benchmarks. See drsock.h.

Changes:
11/18/2026: First version for dr_utils.
*/
// ----------------------------------------------------------- //
#if defined (__WIN32__)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drsock.h"

#if defined (__WIN32__)
typedef SOCKET native_t;
#define NATIVE_BAD INVALID_SOCKET
typedef int socklen_t;
#else
typedef int native_t;
#define NATIVE_BAD (-1)
#endif

static native_t N(drsock_t s) { return (native_t)s; }

int drsock_init()
{
#if defined (__WIN32__)
  static int ready = 0;
  if(ready) return 1;
  WSADATA wsa;
  if(WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 0;
  ready = 1;
#endif
  return 1;
}

gxString drsock_error()
{
  gxString e;
#if defined (__WIN32__)
  e << "socket error " << (int)WSAGetLastError();
#else
  e << strerror(errno);
#endif
  return e;
}

void drsock_close(drsock_t s)
{
  if(s == DRSOCK_BAD) return;
#if defined (__WIN32__)
  closesocket(N(s));
#else
  close(N(s));
#endif
}

static drsock_t drsock_new(int type, int proto)
{
  native_t s = socket(AF_INET, type, proto);
  if(s == NATIVE_BAD) return DRSOCK_BAD;
#if !defined (__WIN32__)
  fcntl(s, F_SETFD, FD_CLOEXEC);
#else
  SetHandleInformation((HANDLE)s, HANDLE_FLAG_INHERIT, 0);
#endif
  return (drsock_t)s;
}

static int drsock_addr(const char *ip, int port, struct sockaddr_in &sa)
{
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_port = htons((unsigned short)port);
  if(!ip || !*ip) {
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    return 1;
  }
  return inet_pton(AF_INET, ip, &sa.sin_addr) == 1;
}

int drsock_resolve(const char *host, gxString &ip, gxString &err)
{
  drsock_init();
  struct addrinfo hints, *res = 0;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  int rv = getaddrinfo(host, 0, &hints, &res);
  if(rv != 0 || !res) {
    err << clear << "Could not resolve " << host << ": " << gai_strerror(rv);
    return 0;
  }
  char buf[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &((struct sockaddr_in *)res->ai_addr)->sin_addr, buf,
	    sizeof(buf));
  ip = buf;
  freeaddrinfo(res);
  return 1;
}

// Wait for a socket to be readable (POLLIN) or writable. Returns 1
// ready, 0 timeout, -1 error.
static int drsock_wait(drsock_t s, int for_write, int timeout_ms)
{
#if defined (__WIN32__)
  WSAPOLLFD p;
  p.fd = N(s);
  p.events = for_write ? POLLWRNORM : POLLRDNORM;
  p.revents = 0;
  int rv = WSAPoll(&p, 1, timeout_ms);
  if(rv < 0) return -1;
  return rv > 0;
#else
  struct pollfd p;
  p.fd = N(s);
  p.events = for_write ? POLLOUT : POLLIN;
  p.revents = 0;
  int rv;
  do {
    rv = poll(&p, 1, timeout_ms);
  } while(rv < 0 && errno == EINTR);
  if(rv < 0) return -1;
  return rv > 0;
#endif
}

drsock_t drsock_tcp_listen(const char *bind_ip, int port, gxString &err)
{
  drsock_init();
  struct sockaddr_in sa;
  if(!drsock_addr(bind_ip, port, sa)) {
    err << clear << bind_ip << " is not an IPv4 address";
    return DRSOCK_BAD;
  }
  drsock_t s = drsock_new(SOCK_STREAM, IPPROTO_TCP);
  if(s == DRSOCK_BAD) {
    err << clear << "socket: " << drsock_error();
    return DRSOCK_BAD;
  }
#if !defined (__WIN32__)
  int on = 1;
  setsockopt(N(s), SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
#endif
  if(bind(N(s), (struct sockaddr *)&sa, sizeof(sa)) != 0) {
    err << clear << "bind port " << port << ": " << drsock_error();
    drsock_close(s);
    return DRSOCK_BAD;
  }
  if(listen(N(s), SOMAXCONN) != 0) {
    err << clear << "listen: " << drsock_error();
    drsock_close(s);
    return DRSOCK_BAD;
  }
  return s;
}

drsock_t drsock_accept(drsock_t ls, int timeout_ms, gxString &peer,
		       gxString &err)
{
  err.Clear();
  int w = drsock_wait(ls, 0, timeout_ms);
  if(w == 0) return DRSOCK_BAD;
  if(w < 0) {
    err << clear << "poll: " << drsock_error();
    return DRSOCK_BAD;
  }
  struct sockaddr_in sa;
  socklen_t len = sizeof(sa);
  native_t c = accept(N(ls), (struct sockaddr *)&sa, &len);
  if(c == NATIVE_BAD) {
    err << clear << "accept: " << drsock_error();
    return DRSOCK_BAD;
  }
#if !defined (__WIN32__)
  fcntl(c, F_SETFD, FD_CLOEXEC);
#endif
  char buf[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &sa.sin_addr, buf, sizeof(buf));
  peer << clear << buf << ":" << (int)ntohs(sa.sin_port);
  return (drsock_t)c;
}

drsock_t drsock_tcp_connect(const char *host, int port, int timeout_ms,
			    gxString &err)
{
  drsock_init();
  gxString ip;
  if(!drsock_resolve(host, ip, err)) return DRSOCK_BAD;
  struct sockaddr_in sa;
  drsock_addr(ip.c_str(), port, sa);
  drsock_t s = drsock_new(SOCK_STREAM, IPPROTO_TCP);
  if(s == DRSOCK_BAD) {
    err << clear << "socket: " << drsock_error();
    return DRSOCK_BAD;
  }
#if defined (__WIN32__)
  u_long nb = 1;
  ioctlsocket(N(s), FIONBIO, &nb);
  int rv = connect(N(s), (struct sockaddr *)&sa, sizeof(sa));
  int in_progress = (rv != 0 && WSAGetLastError() == WSAEWOULDBLOCK);
#else
  int flags = fcntl(N(s), F_GETFL, 0);
  fcntl(N(s), F_SETFL, flags | O_NONBLOCK);
  int rv = connect(N(s), (struct sockaddr *)&sa, sizeof(sa));
  int in_progress = (rv != 0 && errno == EINPROGRESS);
#endif
  if(rv != 0) {
    if(!in_progress || drsock_wait(s, 1, timeout_ms) <= 0) {
      err << clear << "connect to " << ip << " port " << port
	  << " failed or timed out";
      drsock_close(s);
      return DRSOCK_BAD;
    }
    int so_err = 0;
    socklen_t len = sizeof(so_err);
    getsockopt(N(s), SOL_SOCKET, SO_ERROR, (char *)&so_err, &len);
    if(so_err != 0) {
      err << clear << "connect to " << ip << " port " << port
	  << " refused or failed";
      drsock_close(s);
      return DRSOCK_BAD;
    }
  }
  // Back to blocking for the transfer
#if defined (__WIN32__)
  nb = 0;
  ioctlsocket(N(s), FIONBIO, &nb);
#else
  fcntl(N(s), F_SETFL, flags);
#endif
  return s;
}

int drsock_send_all(drsock_t s, const void *buf, int len)
{
  const char *p = (const char *)buf;
  while(len > 0) {
#if defined (__WIN32__)
    int n = send(N(s), p, len, 0);
#else
    int n = (int)send(N(s), p, len, MSG_NOSIGNAL);
    if(n < 0 && errno == EINTR) continue;
#endif
    if(n <= 0) return 0;
    p += n;
    len -= n;
  }
  return 1;
}

int drsock_recv_some(drsock_t s, void *buf, int len, int timeout_ms)
{
  if(timeout_ms >= 0 && drsock_wait(s, 0, timeout_ms) <= 0) return -1;
  for(;;) {
    int n = (int)recv(N(s), (char *)buf, len, 0);
#if !defined (__WIN32__)
    if(n < 0 && errno == EINTR) continue;
#endif
    return n < 0 ? -1 : n;
  }
}

int drsock_recv_all(drsock_t s, void *buf, int len, int timeout_ms)
{
  char *p = (char *)buf;
  while(len > 0) {
    int n = drsock_recv_some(s, p, len, timeout_ms);
    if(n <= 0) return 0;
    p += n;
    len -= n;
  }
  return 1;
}

void drsock_set_buffers(drsock_t s, int bytes)
{
  setsockopt(N(s), SOL_SOCKET, SO_SNDBUF, (char *)&bytes, sizeof(bytes));
  setsockopt(N(s), SOL_SOCKET, SO_RCVBUF, (char *)&bytes, sizeof(bytes));
}

drsock_t drsock_udp_open(const char *bind_ip, int port, gxString &err)
{
  drsock_init();
  struct sockaddr_in sa;
  if(!drsock_addr(bind_ip, port, sa)) {
    err << clear << bind_ip << " is not an IPv4 address";
    return DRSOCK_BAD;
  }
  drsock_t s = drsock_new(SOCK_DGRAM, IPPROTO_UDP);
  if(s == DRSOCK_BAD) {
    err << clear << "socket: " << drsock_error();
    return DRSOCK_BAD;
  }
  int on = 1;
  if(port != 0) { // Several receivers of one multicast group
    setsockopt(N(s), SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
  }
  if(bind(N(s), (struct sockaddr *)&sa, sizeof(sa)) != 0) {
    err << clear << "bind UDP port " << port << ": " << drsock_error();
    drsock_close(s);
    return DRSOCK_BAD;
  }
  return s;
}

int drsock_sendto(drsock_t s, const void *buf, int len, const char *ip,
		  int port)
{
  struct sockaddr_in sa;
  if(!drsock_addr(ip, port, sa)) return -1;
  for(;;) {
    int n = (int)sendto(N(s), (const char *)buf, len, 0,
			(struct sockaddr *)&sa, sizeof(sa));
#if !defined (__WIN32__)
    if(n < 0 && errno == EINTR) continue;
    // A full socket buffer: wait for room, do not count a loss here
    if(n < 0 && (errno == ENOBUFS || errno == EAGAIN)) {
      drsock_wait(s, 1, 10);
      continue;
    }
#else
    if(n < 0 && WSAGetLastError() == WSAENOBUFS) {
      Sleep(1);
      continue;
    }
#endif
    return n;
  }
}

int drsock_recvfrom(drsock_t s, void *buf, int len, int timeout_ms,
		    gxString &from_ip, int &from_port)
{
  if(timeout_ms >= 0) {
    int w = drsock_wait(s, 0, timeout_ms);
    if(w == 0) return 0;
    if(w < 0) return -1;
  }
  struct sockaddr_in sa;
  socklen_t slen = sizeof(sa);
  int n;
  for(;;) {
    n = (int)recvfrom(N(s), (char *)buf, len, 0, (struct sockaddr *)&sa,
		      &slen);
#if !defined (__WIN32__)
    if(n < 0 && errno == EINTR) continue;
#else
    if(n < 0) {
      int e = WSAGetLastError();
      if(e == WSAEMSGSIZE) n = len; // Larger than the buffer: truncated
      else if(e == WSAECONNRESET) return 0; // ICMP port unreachable
    }
#endif
    break;
  }
  if(n < 0) return -1;
  char ipbuf[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &sa.sin_addr, ipbuf, sizeof(ipbuf));
  from_ip = ipbuf;
  from_port = (int)ntohs(sa.sin_port);
  return n;
}

int drsock_is_multicast(const char *ip)
{
  struct sockaddr_in sa;
  if(!drsock_addr(ip, 0, sa) || !ip || !*ip) return 0;
  unsigned long a = ntohl(sa.sin_addr.s_addr);
  return (a >> 28) == 0xE;
}

int drsock_mc_join(drsock_t s, const char *group, const char *iface_ip,
		   gxString &err)
{
  struct ip_mreq mreq;
  memset(&mreq, 0, sizeof(mreq));
  if(inet_pton(AF_INET, group, &mreq.imr_multiaddr) != 1) {
    err << clear << group << " is not an IPv4 address";
    return 0;
  }
  if(iface_ip && *iface_ip) {
    if(inet_pton(AF_INET, iface_ip, &mreq.imr_interface) != 1) {
      err << clear << iface_ip << " is not an IPv4 address";
      return 0;
    }
  }
  else {
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
  }
  if(setsockopt(N(s), IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq,
		sizeof(mreq)) != 0) {
    err << clear << "join " << group << ": " << drsock_error();
    return 0;
  }
  return 1;
}

int drsock_mc_sender(drsock_t s, int ttl, int loopback,
		     const char *iface_ip, gxString &err)
{
#if defined (__WIN32__)
  DWORD t = (DWORD)ttl, l = (DWORD)(loopback ? 1 : 0);
#else
  unsigned char t = (unsigned char)ttl, l = (unsigned char)(loopback ? 1:0);
#endif
  if(setsockopt(N(s), IPPROTO_IP, IP_MULTICAST_TTL, (char *)&t,
		sizeof(t)) != 0) {
    err << clear << "IP_MULTICAST_TTL: " << drsock_error();
    return 0;
  }
  setsockopt(N(s), IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&l, sizeof(l));
  if(iface_ip && *iface_ip) {
    struct in_addr a;
    if(inet_pton(AF_INET, iface_ip, &a) != 1 ||
       setsockopt(N(s), IPPROTO_IP, IP_MULTICAST_IF, (char *)&a,
		  sizeof(a)) != 0) {
      err << clear << "IP_MULTICAST_IF " << iface_ip << ": "
	  << drsock_error();
      return 0;
    }
  }
  return 1;
}

void drsock_put32(unsigned char *p, unsigned long v)
{
  p[0] = (unsigned char)(v >> 24); p[1] = (unsigned char)(v >> 16);
  p[2] = (unsigned char)(v >> 8); p[3] = (unsigned char)v;
}

unsigned long drsock_get32(const unsigned char *p)
{
  return ((unsigned long)p[0] << 24) | ((unsigned long)p[1] << 16) |
    ((unsigned long)p[2] << 8) | (unsigned long)p[3];
}

void drsock_put64(unsigned char *p, unsigned long long v)
{
  drsock_put32(p, (unsigned long)(v >> 32));
  drsock_put32(p + 4, (unsigned long)(v & 0xFFFFFFFFUL));
}

unsigned long long drsock_get64(const unsigned char *p)
{
  return ((unsigned long long)drsock_get32(p) << 32) | drsock_get32(p + 4);
}
// ----------------------------------------------------------- //
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
