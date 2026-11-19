// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Header File Name: drhook.h
// C++ Compiler Used: MSVC, GCC
// Produced By: DataReel Software Development Team
// File Creation Date: 11/19/2026
// Date Last Modified: 11/19/2026
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

Dashboard output for the dr_utils programs: DRSM lines for the
DataReel Reporting Agents' hook uploads (format drsm-lines).

  INFO - text, WARNING - text, ERROR - text   status lines
  DETAIL name=value                            a value for the manifest
                                               rules and trends (METRICS)
  UPLOAD columns|name|name...                  a table header
  UPLOAD row|cell|cell...                      a table row

Every program that uses it takes the same options:

  --drsm=fname      write the DRSM lines to a file
  --submit[=ID]     hand them to the reporting agent: drra submit
                    --hook ID file (Linux) or drra_svc.exe submit
                    --hook ID file (Windows). ID defaults to
                    dru_<program>, the hook files in dr_utils hooks/.
  --now             with --submit, the agent sends within seconds
  --drra=path       agent program (Linux: drra in the PATH, then
                    /usr/sbin/drra; Windows: %ProgramFiles%\DataReel\
                    ReportingAgent\drra_svc.exe)
  --drra-config=ini agent settings file, drra -c ini (Linux, for tests)

Numbers are written as plain decimals (never 1e+08), since DRSM
rules and trends only read those.

Changes:
11/19/2026: First version for dr_utils.
*/
// ----------------------------------------------------------- //
#ifndef __DR_UTILS_HOOK_HPP__
#define __DR_UTILS_HOOK_HPP__

#include "gxdlcode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#if defined (__WIN32__)
#include <windows.h>
#include <process.h>
#include <direct.h>
#include <io.h>
#else
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include <iostream>
#include <string>
#include <vector>

#include "gxstring.h"

// A number as a plain decimal: 100000000, 12.5, 0.000123
inline gxString drhook_num(double v)
{
  char buf[128];
  if(v != v) return gxString("0");  // NaN
  if(fabs(v) >= 1e15 || (v == floor(v) && fabs(v) < 1e15)) {
    sprintf(buf, "%.0f", v);
    return gxString(buf);
  }
  double a = fabs(v);
  int decimals = 6;
  if(a < 1) {
    // Keep 6 significant digits for small values
    decimals = 6 + (int)ceil(-log10(a > 1e-12 ? a : 1e-12));
    if(decimals > 15) decimals = 15;
  }
  else if(a >= 1000) decimals = 3;
  sprintf(buf, "%.*f", decimals, v);
  size_t len = strlen(buf);
  while(len > 1 && buf[len-1] == '0') buf[--len] = 0;
  if(len > 1 && buf[len-1] == '.') buf[--len] = 0;
  return gxString(buf);
}

// A value from a settings or data file: numbers in any form (1e+08)
// become plain decimals, other text is kept
inline gxString drhook_value(const char *s)
{
  if(!s) return gxString("");
  char *end = 0;
  double v = strtod(s, &end);
  if(*s && end && *end == 0 && strpbrk(s, "eE") && !strpbrk(s, "xX")) {
    return drhook_num(v);
  }
  return gxString(s);
}

// Text for one line or cell: no control characters, no "|" in cells
inline gxString drhook_clean(const char *s, int cell)
{
  gxString out;
  if(!s) return out;
  for(; *s; s++) {
    char c = *s;
    if((unsigned char)c < 0x20 || c == 0x7f) c = ' ';
    else if(cell && c == '|') c = '/';
    out << c;
  }
  return out;
}

// A value name: a-z, 0-9, "_" and "."
inline gxString drhook_name(const char *s)
{
  gxString out;
  if(!s) return out;
  for(; *s; s++) {
    char c = *s;
    if(c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
    if(!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.')) {
      c = '_';
    }
    out << c;
  }
  return out;
}

// The DRSM lines of one result
class drhookLines
{
public:
  drhookLines() { row_open = 0; }
  void Info(const char *s) { Line("INFO - ", s); }
  void Warn(const char *s) { Line("WARNING - ", s); }
  void Error(const char *s) { Line("ERROR - ", s); }
  void Value(const char *name, const char *value) {
    EndRow();
    text << "DETAIL " << drhook_name(name) << "="
	 << drhook_clean(drhook_value(value).c_str(), 0) << "\n";
  }
  void Value(const char *name, double value) {
    Value(name, drhook_num(value).c_str());
  }
  // A table: Columns() or Row(), then one Cell() per cell
  void Columns() { StartRow("UPLOAD columns"); }
  void Row() { StartRow("UPLOAD row"); }
  void Cell(const char *s) {
    text << "|" << drhook_clean(s, 1);
  }
  void Cell(double v) { Cell(drhook_num(v).c_str()); }
  const gxString &Text() { EndRow(); return text; }
  // Add the lines of another set (for example a table made apart)
  void Append(drhookLines &other) { EndRow(); text << other.Text(); }
  int IsEmpty() const { return text.is_null(); }

  // Write the lines to a file. Returns 0 and prints an error if it
  // cannot.
  int Write(const char *fname) {
    EndRow();
    FILE *fp = fopen(fname, "wb");
    if(!fp) {
      fprintf(stdout, "Error could not write %s\n", fname);
      fflush(stdout);
      return 0;
    }
    int ok = fputs(text.c_str(), fp) >= 0;
    if(fclose(fp) != 0) ok = 0;
    if(!ok) {
      fprintf(stdout, "Error could not write %s\n", fname);
      fflush(stdout);
    }
    return ok;
  }

private:
  void Line(const char *kind, const char *s) {
    EndRow();
    text << kind << drhook_clean(s, 0) << "\n";
  }
  void StartRow(const char *kind) {
    EndRow();
    text << kind;
    row_open = 1;
  }
  void EndRow() {
    if(row_open) text << "\n";
    row_open = 0;
  }
  gxString text;
  int row_open;
};

// Collected time for a DETAIL line: 2026-11-19T13:05:00-0500
inline gxString drhook_collected()
{
  char buf[64];
  time_t now = time(0);
  struct tm *t = localtime(&now);
#if defined (__WIN32__)
  // MSVC's %z is a zone name, so make the offset
  struct tm g = *gmtime(&now);
  g.tm_isdst = t->tm_isdst;
  long off = (long)difftime(mktime(t), mktime(&g)) / 60;
  char zone[16];
  sprintf(zone, "%c%02ld%02ld", off < 0 ? '-' : '+', labs(off) / 60,
	  labs(off) % 60);
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", t);
  strcat(buf, zone);
#else
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S%z", t);
#endif
  return gxString(buf);
}

// The hook options and the hand-off to the agent
class drhookOptions
{
public:
  drhookOptions(const char *program) {
    submit = now = 0;
    hook_id << "dru_" << drhook_name(program);
    hook_id.ReplaceString(".", "_");
  }

  // Use --drsm, --submit, --now, --drra or --drra-config. Returns 1
  // if "arg" is one of them (an error in its value sets "error").
  int Arg(const char *arg) {
    if(strncmp(arg, "--drsm=", 7) == 0) {
      drsm_file = arg + 7;
      if(drsm_file.is_null()) error << "--drsm needs a file name";
      return 1;
    }
    if(strcmp(arg, "--submit") == 0) { submit = 1; return 1; }
    if(strncmp(arg, "--submit=", 9) == 0) {
      submit = 1;
      hook_id = arg + 9;
      if(!ValidId(hook_id.c_str())) {
	error << "--submit=ID: the hook ID is a-z, 0-9 and _, "
	      << "up to 32 characters";
      }
      return 1;
    }
    if(strcmp(arg, "--now") == 0) { now = 1; return 1; }
    if(strncmp(arg, "--drra=", 7) == 0) {
      drra = arg + 7;
      if(drra.is_null()) error << "--drra needs a program";
      return 1;
    }
    if(strncmp(arg, "--drra-config=", 14) == 0) {
      drra_config = arg + 14;
#if defined (__WIN32__)
      error << "--drra-config is for Linux, a Windows test copy of the "
	    << "agent reads the drra.ini next to it";
#else
      if(drra_config.is_null()) error << "--drra-config needs a file";
#endif
      return 1;
    }
    return 0;
  }

  // Check the options after the command line is read: prints the
  // error and returns 0 if there is one
  int Check() {
    if(error.is_null() && now && !submit) error << "--now needs --submit";
    if(error.is_null()) return 1;
    fprintf(stdout, "Error %s\n", error.c_str());
    fflush(stdout);
    return 0;
  }

  // Dashboard output asked for
  int Wanted() const { return submit || !drsm_file.is_null(); }

  // Write the --drsm file and/or submit the lines. Returns 0 on an
  // error (printed).
  int Finish(drhookLines &lines) {
    int ok = 1;
    if(!drsm_file.is_null()) {
      if(lines.Write(drsm_file.c_str())) {
	fprintf(stdout, "DRSM lines written to %s\n", drsm_file.c_str());
	fflush(stdout);
      }
      else {
	return 0;
      }
    }
    if(!submit) return ok;
    if(!drsm_file.is_null()) return Submit(drsm_file.c_str());

    // A private temporary folder for the file the agent copies
    gxString dir, fname;
    if(!TempDir(dir)) {
      fprintf(stdout, "Error could not make a temporary folder\n");
      fflush(stdout);
      return 0;
    }
#if defined (__WIN32__)
    fname << dir << "\\" << hook_id << ".txt";
#else
    fname << dir << "/" << hook_id << ".txt";
#endif
    ok = lines.Write(fname.c_str());
    if(ok) ok = Submit(fname.c_str());
    remove(fname.c_str());
#if defined (__WIN32__)
    _rmdir(dir.c_str());
#else
    rmdir(dir.c_str());
#endif
    return ok;
  }

  static int ValidId(const char *s) {
    size_t n = strlen(s);
    if(n < 1 || n > 32) return 0;
    for(; *s; s++) {
      if(!((*s >= 'a' && *s <= 'z') || (*s >= '0' && *s <= '9') ||
	   *s == '_')) return 0;
    }
    return 1;
  }

public:
  gxString drsm_file;
  gxString hook_id;
  gxString drra;
  gxString drra_config;
  gxString error;
  int submit;
  int now;

private:
  int TempDir(gxString &dir) {
#if defined (__WIN32__)
    char base[MAX_PATH+1];
    DWORD n = GetTempPathA(sizeof(base), base);
    if(n == 0 || n > MAX_PATH) return 0;
    for(int i = 0; i < 100; i++) {
      char name[64];
      sprintf(name, "dru_%lu_%lu_%d", (unsigned long)GetCurrentProcessId(),
	      (unsigned long)GetTickCount(), i);
      dir.Clear();
      dir << base << name;
      if(CreateDirectoryA(dir.c_str(), 0)) return 1;
    }
    return 0;
#else
    const char *base = getenv("TMPDIR");
    if(!base || !*base) base = "/tmp";
    gxString tmpl;
    tmpl << base << "/dru_XXXXXX";
    char *buf = new char[tmpl.length() + 1];
    strcpy(buf, tmpl.c_str());
    char *d = mkdtemp(buf);
    if(d) dir = d;
    delete[] buf;
    return d != 0;
#endif
  }

  // Run the agent: <drra> [-c ini] submit --hook ID [--now] file.
  // Returns 1 if it exited 0.
  int Submit(const char *fname) {
    const char *args[10];
    int n = 0;
    gxString prog = drra;
#if defined (__WIN32__)
    if(prog.is_null()) {
      const char *pf = getenv("ProgramFiles");
      prog << (pf && *pf ? pf : "C:\\Program Files")
	   << "\\DataReel\\ReportingAgent\\drra_svc.exe";
    }
    // _spawnv passes the arguments as one command line: quote them
    gxString qprog, qfile;
    qprog << "\"" << prog << "\"";
    qfile << "\"" << fname << "\"";
    args[n++] = qprog.c_str();
#else
    if(prog.is_null()) prog = "drra";
    args[n++] = prog.c_str();
    if(!drra_config.is_null()) {
      args[n++] = "-c";
      args[n++] = drra_config.c_str();
    }
#endif
    args[n++] = "submit";
    args[n++] = "--hook";
    args[n++] = hook_id.c_str();
    if(now) args[n++] = "--now";
#if defined (__WIN32__)
    args[n++] = qfile.c_str();
#else
    args[n++] = fname;
#endif
    args[n] = 0;
    fflush(stdout);
    fflush(stderr);

    int status = -1;
#if defined (__WIN32__)
    intptr_t rv = _spawnv(_P_WAIT, prog.c_str(), args);
    if(rv == -1) {
      fprintf(stdout, "Error could not run %s\n", prog.c_str());
      fflush(stdout);
      return 0;
    }
    status = (int)rv;
#else
    pid_t pid = fork();
    if(pid < 0) {
      fprintf(stdout, "Error could not run %s\n", prog.c_str());
      fflush(stdout);
      return 0;
    }
    if(pid == 0) {
      execvp(args[0], (char * const *)args);
      if(drra.is_null() && errno == ENOENT) {
	args[0] = "/usr/sbin/drra";
	execv(args[0], (char * const *)args);
      }
      fprintf(stdout, "Error could not run %s: %s\n", args[0],
	      strerror(errno));
      fflush(stdout);
      _exit(127);
    }
    int ws = 0;
    while(waitpid(pid, &ws, 0) < 0) {
      if(errno != EINTR) {
	fprintf(stdout, "Error waiting for %s\n", prog.c_str());
	return 0;
      }
    }
    status = WIFEXITED(ws) ? WEXITSTATUS(ws) : -1;
#endif
    if(status != 0) {
      fprintf(stdout, "Error agent submit failed for hook %s (exit %d)\n",
	      hook_id.c_str(), status);
      fflush(stdout);
      return 0;
    }
    fprintf(stdout, "Submitted to the reporting agent, hook %s%s\n",
	    hook_id.c_str(), now ? " (send now)" : "");
    fflush(stdout);
    return 1;
  }
};

// Take the hook options (--drsm, --submit, --now, --drra,
// --drra-config) out of the command line, so a tool's own argument
// parsing never sees them. Returns the new argc.
inline int drhook_args(drhookOptions &hook, int argc, char **argv)
{
  int n = 1;
  for(int i = 1; i < argc; i++) {
    if(!hook.Arg(argv[i])) argv[n++] = argv[i];
  }
  argv[n] = 0;
  return n;
}

// Keeps a copy of the lines a tool writes to cout, while they still
// go to the screen: the tools that report lines (YES/NOT/ERR) are read
// back for the dashboard, and the last line explains an error.
class drhookTee : public std::streambuf
{
public:
  drhookTee() { orig = 0; max_lines = 50000; }
  ~drhookTee() { Stop(); }
  void Start() {
    if(orig) return;
    orig = std::cout.rdbuf(this);
  }
  void Stop() {
    if(!orig) return;
    std::cout.flush();
    std::cout.rdbuf(orig);
    orig = 0;
    if(!cur.empty()) { Keep(cur); cur.clear(); }
  }
  const std::vector<std::string> &Lines() const { return lines; }
  // The last line that is not empty, or ""
  std::string LastLine() const {
    if(!cur.empty()) return cur;
    for(size_t i = lines.size(); i > 0; i--) {
      if(!lines[i-1].empty()) return lines[i-1];
    }
    return std::string();
  }
protected:
  virtual int overflow(int c) {
    if(c == EOF) return 0;
    if(orig) orig->sputc((char)c);
    if(c == '\n') { Keep(cur); cur.clear(); }
    else if(c != '\r' && cur.size() < 4000) cur += (char)c;
    return c;
  }
  virtual std::streamsize xsputn(const char *s, std::streamsize n) {
    for(std::streamsize i = 0; i < n; i++) overflow((unsigned char)s[i]);
    return n;
  }
  virtual int sync() { return orig ? orig->pubsync() : 0; }
private:
  void Keep(const std::string &l) {
    if(lines.size() < max_lines) lines.push_back(l);
  }
  std::streambuf *orig;
  std::string cur;
  std::vector<std::string> lines;
  size_t max_lines;
};

// The first lines of every tool's results: the program and version
inline void drhook_head(drhookLines &lines, const char *program,
			const char *version)
{
  lines.Value("program", program);
  lines.Value("version", version);
}

// Finish a tool's dashboard output: "summary" (INFO line and values)
// then "table". With "error" set, an ERROR line with the tool's last
// output line comes first. Returns 0 if the lines could not be
// written or submitted (printed).
inline int drhook_tool_finish(drhookOptions &hook, drhookTee &tee,
			      const char *program, int error,
			      drhookLines &summary, drhookLines &table)
{
  drhookLines lines;
  if(error) {
    gxString e;
    std::string last = tee.LastLine();
    e << program << ": " << (last.empty() ? "failed" : last.c_str());
    lines.Error(e.c_str());
  }
  lines.Append(summary);
  lines.Value("collected", drhook_collected().c_str());
  lines.Append(table);
  return hook.Finish(lines);
}

// Usage lines for the hook options
inline void drhook_usage(const char *program)
{
  printf("Dashboard (DataReel Reporting Agent hook upload, DRSM):\n");
  printf("     --drsm=fname          Write the results as DRSM lines\n");
  printf("     --submit[=ID]         Hand them to the reporting agent, "
	 "hook ID (default dru_%s)\n", drhook_name(program).c_str());
  printf("     --now                 With --submit, the agent sends "
	 "within seconds\n");
#if defined (__WIN32__)
  printf("     --drra=path           Agent program (default "
	 "%%ProgramFiles%%\\DataReel\\ReportingAgent\\drra_svc.exe)\n");
#else
  printf("     --drra=path           Agent program (default drra, then "
	 "/usr/sbin/drra)\n");
  printf("     --drra-config=ini     Agent settings file (drra -c ini)\n");
#endif
  fflush(stdout);
}

#endif // __DR_UTILS_HOOK_HPP__
// ----------------------------------------------------------- //
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
