// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Source Code File Name: drnet.cpp
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

Network helpers shared by the dr_utils network tools. See drnet.h.

Changes:
11/18/2026: First version for dr_utils, based on the network code of
Douglas Gaer's 1999-2010 scan programs.
*/
// ----------------------------------------------------------- //
#if defined (__WIN32__)
// Winsock 2 before any DataReel header (gxstypes.h includes the
// Winsock 1 header winsock.h, which winsock2.h replaces)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
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
#include <ctype.h>

#include "drnet.h"

#if defined (__WIN32__)
typedef SOCKET drnet_socket_t;
#define DRNET_BAD_SOCKET INVALID_SOCKET
#define drnet_close_socket closesocket
#else
typedef int drnet_socket_t;
#define DRNET_BAD_SOCKET (-1)
#define drnet_close_socket close
#endif

static double drnet_now_ms()
{
#if defined (__WIN32__)
  LARGE_INTEGER f, c;
  QueryPerformanceFrequency(&f);
  QueryPerformanceCounter(&c);
  return (double)c.QuadPart * 1000.0 / (double)f.QuadPart;
#else
  struct timeval tv;
  gettimeofday(&tv, 0);
  return (double)tv.tv_sec * 1000.0 + (double)tv.tv_usec / 1000.0;
#endif
}

int drnet_init()
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

static int drnet_to_addr(const char *ip, struct sockaddr_in &sa)
{
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  return inet_pton(AF_INET, ip, &sa.sin_addr) == 1;
}

int drnet_resolve(const char *host, gxString &ip, gxString &err)
{
  drnet_init();
  struct addrinfo hints, *res = 0;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  int rv = getaddrinfo(host, 0, &hints, &res);
  if(rv != 0 || !res) {
    err << clear << "Could not resolve " << host << ": "
	<< gai_strerror(rv);
    return 0;
  }
  char buf[INET_ADDRSTRLEN];
  struct sockaddr_in *sa = (struct sockaddr_in *)res->ai_addr;
  inet_ntop(AF_INET, &sa->sin_addr, buf, sizeof(buf));
  ip = buf;
  freeaddrinfo(res);
  return 1;
}

int drnet_reverse(const char *ip, gxString &name)
{
  drnet_init();
  struct sockaddr_in sa;
  if(!drnet_to_addr(ip, sa)) return 0;
  char host[NI_MAXHOST];
  if(getnameinfo((struct sockaddr *)&sa, sizeof(sa), host, sizeof(host),
		 0, 0, NI_NAMEREQD) != 0) {
    return 0;
  }
  name = host;
  return 1;
}

int drnet_host_info(const char *host, gxString &info)
{
  drnet_init();
  info.Clear();
  struct addrinfo hints, *res = 0, *p;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_CANONNAME;
  if(getaddrinfo(host, 0, &hints, &res) != 0 || !res) return 0;

  gxString first_ip;
  char buf[INET_ADDRSTRLEN];
  gxString addrs;
  for(p = res; p; p = p->ai_next) {
    struct sockaddr_in *sa = (struct sockaddr_in *)p->ai_addr;
    inet_ntop(AF_INET, &sa->sin_addr, buf, sizeof(buf));
    if(first_ip.is_null()) first_ip = buf;
    gxString line;
    line << clear << "Host IP Address: " << buf << "\n";
    if(addrs.Find(line) == -1) addrs << line;
  }

  // The name: the reverse DNS name of a dotted address, else the
  // canonical name
  gxString name;
  struct sockaddr_in tmp;
  if(drnet_to_addr(host, tmp)) {
    if(!drnet_reverse(host, name)) name = "(no DNS name)";
  }
  else {
    name = (res->ai_canonname) ? res->ai_canonname : host;
    if(name != host) info << "Host Alias: " << host << "\n";
  }
  freeaddrinfo(res);

  gxString out;
  out << clear << "Hostname: " << name << "\n" << info << addrs;
  info = out;
  return 1;
}

// ICMP checksum (RFC 1071)
static unsigned short drnet_cksum(const unsigned char *buf, int len)
{
  unsigned long sum = 0;
  while(len > 1) {
    sum += (unsigned short)((buf[0] << 8) | buf[1]);
    buf += 2; len -= 2;
  }
  if(len == 1) sum += (unsigned short)(buf[0] << 8);
  while(sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
  return (unsigned short)(~sum & 0xFFFF);
}

#if !defined (__WIN32__)
// One echo on a Linux ICMP socket. "raw" sockets get the IP header
// in front of the reply. Returns DRNET_REPLY, DRNET_NO_REPLY or
// DRNET_ERROR (with errno set).
static int drnet_ping_socket(int s, int raw, const struct sockaddr_in &to,
			     int timeout_ms, double &rtt_ms)
{
  static unsigned short seq = 0;
  unsigned short ident = (unsigned short)(getpid() & 0xFFFF);
  unsigned short my_seq = ++seq;

  unsigned char pkt[64];
  memset(pkt, 0, sizeof(pkt));
  pkt[0] = 8; // Echo request
  pkt[1] = 0;
  pkt[4] = (unsigned char)(ident >> 8); pkt[5] = (unsigned char)ident;
  pkt[6] = (unsigned char)(my_seq >> 8); pkt[7] = (unsigned char)my_seq;
  for(int i = 8; i < (int)sizeof(pkt); i++) pkt[i] = (unsigned char)i;
  unsigned short ck = drnet_cksum(pkt, sizeof(pkt));
  pkt[2] = (unsigned char)(ck >> 8); pkt[3] = (unsigned char)ck;

  double start = drnet_now_ms();
  if(sendto(s, pkt, sizeof(pkt), 0, (const struct sockaddr *)&to,
	    sizeof(to)) < 0) {
    return DRNET_ERROR;
  }

  unsigned char buf[1500];
  for(;;) {
    int left = timeout_ms - (int)(drnet_now_ms() - start);
    if(left <= 0) return DRNET_NO_REPLY;
    struct pollfd pfd;
    pfd.fd = s; pfd.events = POLLIN; pfd.revents = 0;
    int pr = poll(&pfd, 1, left);
    if(pr < 0) {
      if(errno == EINTR) continue;
      return DRNET_ERROR;
    }
    if(pr == 0) return DRNET_NO_REPLY;
    struct sockaddr_in from;
    socklen_t flen = sizeof(from);
    int n = (int)recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&from,
			  &flen);
    if(n < 0) {
      if(errno == EINTR || errno == EAGAIN) continue;
      return DRNET_ERROR;
    }
    if(from.sin_addr.s_addr != to.sin_addr.s_addr) continue;
    const unsigned char *icmp = buf;
    if(raw) {
      if(n < 20) continue;
      int hlen = (buf[0] & 0x0F) * 4;
      if(hlen < 20 || n < hlen + 8) continue;
      icmp = buf + hlen;
      n -= hlen;
    }
    if(n < 8) continue;
    if(icmp[0] != 0) continue; // Not an echo reply
    unsigned short r_seq = (unsigned short)((icmp[6] << 8) | icmp[7]);
    if(r_seq != my_seq) continue;
    if(raw) { // The kernel sets and checks the id on datagram sockets
      unsigned short r_id = (unsigned short)((icmp[4] << 8) | icmp[5]);
      if(r_id != ident) continue;
    }
    rtt_ms = drnet_now_ms() - start;
    return DRNET_REPLY;
  }
}
#endif

int drnet_ping(const char *ip, int timeout_ms, double &rtt_ms,
	       gxString &err)
{
  drnet_init();
  rtt_ms = 0;
  struct sockaddr_in to;
  if(!drnet_to_addr(ip, to)) {
    err << clear << ip << " is not an IPv4 address";
    return DRNET_ERROR;
  }
  if(timeout_ms < 1) timeout_ms = 1;

#if defined (__WIN32__)
  HANDLE h = IcmpCreateFile();
  if(h == INVALID_HANDLE_VALUE) {
    err << clear << "IcmpCreateFile failed, error " << (int)GetLastError();
    return DRNET_ERROR;
  }
  char data[32];
  memset(data, 'D', sizeof(data));
  DWORD rsize = sizeof(ICMP_ECHO_REPLY) + sizeof(data) + 64;
  unsigned char *reply = new unsigned char[rsize];
  double start = drnet_now_ms();
  DWORD n = IcmpSendEcho(h, to.sin_addr.s_addr, data, sizeof(data), 0,
			 reply, rsize, (DWORD)timeout_ms);
  int rv = DRNET_NO_REPLY;
  if(n > 0) {
    PICMP_ECHO_REPLY r = (PICMP_ECHO_REPLY)reply;
    if(r->Status == IP_SUCCESS && r->Address == to.sin_addr.s_addr) {
      rtt_ms = drnet_now_ms() - start;
      rv = DRNET_REPLY;
    }
  }
  else {
    DWORD e = GetLastError();
    if(e != IP_REQ_TIMED_OUT && e != IP_DEST_HOST_UNREACHABLE &&
       e != IP_DEST_NET_UNREACHABLE && e != 11010) {
      err << clear << "IcmpSendEcho failed, error " << (int)e;
      rv = DRNET_ERROR;
    }
  }
  delete[] reply;
  IcmpCloseHandle(h);
  return rv;
#else
  // Unprivileged ICMP datagram socket first, then a raw socket
  int raw = 0;
  int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
  if(s < 0) {
    raw = 1;
    s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  }
  if(s < 0) {
    err << clear << "No ICMP socket (" << strerror(errno)
	<< "): run as root, set net.ipv4.ping_group_range, or give the "
	<< "program CAP_NET_RAW";
    return DRNET_ERROR;
  }
  fcntl(s, F_SETFD, FD_CLOEXEC);
  int rv = drnet_ping_socket(s, raw, to, timeout_ms, rtt_ms);
  if(rv == DRNET_ERROR) {
    if(errno == ENETUNREACH || errno == EHOSTUNREACH) {
      rv = DRNET_NO_REPLY;
    }
    else {
      err << clear << "Ping " << ip << " failed: " << strerror(errno);
    }
  }
  close(s);
  return rv;
#endif
}

int drnet_tcp_connect(const char *ip, int port, int timeout_ms,
		      gxString &err)
{
  drnet_init();
  struct sockaddr_in sa;
  if(!drnet_to_addr(ip, sa)) {
    err << clear << ip << " is not an IPv4 address";
    return DRNET_ERROR;
  }
  if(port < 1 || port > 65535) {
    err << clear << "Port " << port << " is out of range";
    return DRNET_ERROR;
  }
  sa.sin_port = htons((unsigned short)port);
  if(timeout_ms < 1) timeout_ms = 1;

  drnet_socket_t s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(s == DRNET_BAD_SOCKET) {
    err << clear << "Could not make a TCP socket";
    return DRNET_ERROR;
  }

#if defined (__WIN32__)
  u_long nb = 1;
  ioctlsocket(s, FIONBIO, &nb);
  int rv = connect(s, (struct sockaddr *)&sa, sizeof(sa));
  int open_port = 0;
  if(rv == 0) {
    open_port = 1;
  }
  else if(WSAGetLastError() == WSAEWOULDBLOCK) {
    fd_set wfds, efds;
    FD_ZERO(&wfds); FD_ZERO(&efds);
    FD_SET(s, &wfds); FD_SET(s, &efds);
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    if(select(0, 0, &wfds, &efds, &tv) > 0 && FD_ISSET(s, &wfds)) {
      open_port = 1;
    }
  }
  closesocket(s);
  return open_port;
#else
  fcntl(s, F_SETFD, FD_CLOEXEC);
  int flags = fcntl(s, F_GETFL, 0);
  fcntl(s, F_SETFL, flags | O_NONBLOCK);
  int rv = connect(s, (struct sockaddr *)&sa, sizeof(sa));
  int open_port = 0;
  if(rv == 0) {
    open_port = 1;
  }
  else if(errno == EINPROGRESS) {
    struct pollfd pfd;
    pfd.fd = s; pfd.events = POLLOUT; pfd.revents = 0;
    int pr;
    do {
      pr = poll(&pfd, 1, timeout_ms);
    } while(pr < 0 && errno == EINTR);
    if(pr > 0) {
      int so_err = 0;
      socklen_t len = sizeof(so_err);
      if(getsockopt(s, SOL_SOCKET, SO_ERROR, &so_err, &len) == 0 &&
	 so_err == 0) {
	open_port = 1;
      }
    }
  }
  close(s);
  return open_port;
#endif
}

int drnet_mac_address(const char *ip, const char *dev, gxString &mac)
{
  drnet_init();
  struct sockaddr_in sa;
  if(!drnet_to_addr(ip, sa)) return 0;
  char buf[32];

#if defined (__WIN32__)
  (void)dev;
  ULONG macbuf[2];
  ULONG len = 6;
  memset(macbuf, 0, sizeof(macbuf));
  if(SendARP((IPAddr)sa.sin_addr.s_addr, 0, macbuf, &len) != NO_ERROR ||
     len < 6) {
    return 0;
  }
  unsigned char *p = (unsigned char *)macbuf;
  sprintf(buf, "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X", p[0], p[1], p[2], p[3],
	  p[4], p[5]);
  mac = buf;
  return 1;
#else
  // /proc/net/arp: IP address, HW type, Flags, HW address, Mask, Device
  FILE *fp = fopen("/proc/net/arp", "r");
  if(!fp) return 0;
  char line[512];
  int found = 0;
  if(!fgets(line, sizeof(line), fp)) { fclose(fp); return 0; } // Header
  while(fgets(line, sizeof(line), fp)) {
    char a_ip[64], a_type[32], a_flags[32], a_mac[64], a_mask[64], a_dev[64];
    if(sscanf(line, "%63s %31s %31s %63s %63s %63s", a_ip, a_type, a_flags,
	      a_mac, a_mask, a_dev) != 6) continue;
    if(strcmp(a_ip, ip) != 0) continue;
    if(dev && *dev && strcmp(a_dev, dev) != 0) continue;
    // Flags 0x0 is an incomplete entry
    if(strtol(a_flags, 0, 16) == 0) continue;
    if(strcmp(a_mac, "00:00:00:00:00:00") == 0) continue;
    for(char *c = a_mac; *c; c++) *c = (char)toupper(*c);
    mac = a_mac;
    found = 1;
    break;
  }
  fclose(fp);
  (void)buf;
  return found;
#endif
}

static int drnet_octet(const char *s, int &v)
{
  if(!s || !*s) return 0;
  v = 0;
  int digits = 0;
  for(; *s; s++, digits++) {
    if(!isdigit((unsigned char)*s) || digits >= 3) return 0;
    v = v * 10 + (*s - '0');
  }
  return v <= 255;
}

int drnet_parse_range(const char *range, gxString &base, int &first,
		      int &last, gxString &err)
{
  err.Clear();
  if(!range || !*range || strlen(range) > 40) {
    err << clear << "No address range given";
    return 0;
  }
  char buf[64];
  strncpy(buf, range, sizeof(buf)-1);
  buf[sizeof(buf)-1] = 0;

  char *part[5];
  int n = 0;
  part[n++] = buf;
  for(char *c = buf; *c; c++) {
    if(*c == '.') {
      if(n >= 4) { n = 5; break; }
      *c = 0;
      part[n++] = c + 1;
    }
  }
  if(n != 3 && n != 4) {
    err << clear << range << " is not a.b.c, a.b.c.d or a.b.c.x-y";
    return 0;
  }
  int o[3];
  for(int i = 0; i < 3; i++) {
    if(!drnet_octet(part[i], o[i])) {
      err << clear << range << " has a bad address byte: " << part[i];
      return 0;
    }
  }
  if(o[0] == 0) {
    err << clear << range << " does not start with a network number";
    return 0;
  }
  base << clear << o[0] << "." << o[1] << "." << o[2];
  if(n == 3) {
    first = 1; last = 254;
    return 1;
  }
  char *dash = strchr(part[3], '-');
  if(dash) {
    *dash = 0;
    if(!drnet_octet(part[3], first) || !drnet_octet(dash+1, last)) {
      err << clear << range << " has a bad last byte range";
      return 0;
    }
  }
  else {
    if(!drnet_octet(part[3], first)) {
      err << clear << range << " has a bad last address byte";
      return 0;
    }
    if(first == 0 || first == 255) { // a.b.c.0 or a.b.c.255: all hosts
      first = 1; last = 254;
      return 1;
    }
    last = first;
  }
  if(first > last || first < 1 || last > 254) {
    err << clear << range << ": the last byte range must be 1-254, low-high";
    return 0;
  }
  return 1;
}

int drnet_parse_service(const char *line, drnetService &s)
{
  memset(&s, 0, sizeof(s));
  if(!line) return 0;
  while(*line == ' ' || *line == '\t') line++;
  if(*line == '#' || *line == 0 || *line == '\r' || *line == '\n') return 0;

  char buf[512];
  strncpy(buf, line, sizeof(buf)-1);
  buf[sizeof(buf)-1] = 0;
  char *nl = strpbrk(buf, "\r\n");
  if(nl) *nl = 0;

  char *hash = strchr(buf, '#');
  if(hash) {
    *hash = 0;
    char *c = hash + 1;
    while(*c == ' ' || *c == '\t') c++;
    strncpy(s.comment, c, sizeof(s.comment)-1);
  }

  char *save = 0;
  const char *ws = " \t";
#if defined (__WIN32__)
  char *tok = strtok_s(buf, ws, &save);
#else
  char *tok = strtok_r(buf, ws, &save);
#endif
  if(!tok) return 0;
  strncpy(s.name, tok, sizeof(s.name)-1);
#if defined (__WIN32__)
  tok = strtok_s(0, ws, &save);
#else
  tok = strtok_r(0, ws, &save);
#endif
  if(!tok) return 0;
  char *slash = strchr(tok, '/');
  if(!slash) return 0;
  *slash = 0;
  s.port = atoi(tok);
  if(s.port < 1 || s.port > 65535) return 0;
  strncpy(s.protocol, slash+1, sizeof(s.protocol)-1);
  for(char *c = s.protocol; *c; c++) *c = (char)tolower(*c);

  // Everything left is aliases
  for(;;) {
#if defined (__WIN32__)
    tok = strtok_s(0, ws, &save);
#else
    tok = strtok_r(0, ws, &save);
#endif
    if(!tok) break;
    size_t used = strlen(s.aliases);
    if(used + strlen(tok) + 2 >= sizeof(s.aliases)) break;
    if(used) strcat(s.aliases, " ");
    strcat(s.aliases, tok);
  }
  return 1;
}

int drnet_load_tcp_services(const char *fname, drnetService *out, int max)
{
  FILE *fp = fopen(fname, "r");
  if(!fp) return -1;
  int n = 0;
  char line[1024];
  while(n < max && fgets(line, sizeof(line), fp)) {
    size_t len = strlen(line);
    if(len && line[len-1] != '\n' && !feof(fp)) { // Skip the rest
      int ch;
      while((ch = fgetc(fp)) != EOF && ch != '\n') ;
    }
    drnetService s;
    if(!drnet_parse_service(line, s)) continue;
    if(strcmp(s.protocol, "tcp") != 0) continue;
    if(drnet_find_service(out, n, s.port)) continue;
    out[n++] = s;
  }
  fclose(fp);
  return n;
}

const drnetService *drnet_find_service(const drnetService *list, int n,
				       int port)
{
  for(int i = 0; i < n; i++) {
    if(list[i].port == port) return &list[i];
  }
  return 0;
}

const char *drnet_default_services()
{
#if defined (__WIN32__)
  static char path[MAX_PATH+64];
  const char *root = getenv("SystemRoot");
  if(!root || !*root) root = "C:\\Windows";
  _snprintf(path, sizeof(path)-1, "%s\\System32\\drivers\\etc\\services",
	    root);
  path[sizeof(path)-1] = 0;
  return path;
#else
  return "/etc/services";
#endif
}
// ----------------------------------------------------------- //
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
