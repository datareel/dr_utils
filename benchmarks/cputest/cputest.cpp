// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Source Code File Name: cputest.cpp
// C++ Compiler Used: MSVC, GCC
// Produced By: DataReel Software Development Team
// File Creation Date: 2002
// Date Last Modified: 11/19/2026
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

CPU speed benchmark. A memory buffer is filled with random bytes and
split between the threads. Each thread compresses its part with zlib
at the best compression level, uncompresses it and checks the result.
The time of each thread and of the whole run is printed, with the
rate in bytes of buffer a second. Run it with one thread for single
core speed and with one thread per core for the whole processor.

Changes:
11/18/2026: dr_utils version, based on work from the 2002-2019 CPU
benchmark (bmarks.cpu, then benchmarks/src/cputest). The same test
(zlib compress2 level 9 and uncompress of random data, --num-threads
and --buf-size as before). Times are wall clock: the old version used
clock(), which on Linux counts the CPU time of every thread, so its
multi-thread times were too long. New: --repeat, a check that each
uncompressed part matches, Windows builds.
11/19/2026: --drsm and --submit: results for the DRSM dashboard through
the DataReel Reporting Agents (hooks/README.md).
*/
// ----------------------------------------------------------- //
#include "gxdlcode.h"

#include <iostream>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "gxthread.h"
#include "drbench.h"
#include "drutil.h"

const char *ProgramName = "cputest";
const char *VersionString = DR_UTILS_VERSION;

drbenchConsole console;

struct CPUThreadParm {
  int thread_number;
  unsigned char *buf;  // This thread's part of the buffer
  size_t len;
  int repeat;
  double run_time;     // Wall clock seconds, set by the thread
  int ok;
  gxString error;
};

class CPUTestThread : public gxThread
{
private:
  void *ThreadEntryRoutine(gxThread_t *thread);
};

// One pass: compress, uncompress, compare. Returns 1 if it worked.
int RunPass(CPUThreadParm *p, unsigned char *dest, uLong dest_size,
	    unsigned char *dup)
{
  uLong dest_len = dest_size;
  int rv = compress2(dest, &dest_len, p->buf, (uLong)p->len,
		     Z_BEST_COMPRESSION);
  if(rv != Z_OK) {
    p->error << clear << "compress2 failed: " << zError(rv);
    return 0;
  }
  uLong dup_len = (uLong)p->len;
  rv = uncompress(dup, &dup_len, dest, dest_len);
  if(rv != Z_OK) {
    p->error << clear << "uncompress failed: " << zError(rv);
    return 0;
  }
  if(dup_len != (uLong)p->len || memcmp(dup, p->buf, p->len) != 0) {
    p->error << clear << "the uncompressed data does not match";
    return 0;
  }
  return 1;
}

void *CPUTestThread::ThreadEntryRoutine(gxThread_t *thread)
{
  CPUThreadParm *p = (CPUThreadParm *)thread->GetThreadParm();
  p->ok = 0;
  uLong dest_size = compressBound((uLong)p->len);
  unsigned char *dest = new unsigned char[dest_size];
  unsigned char *dup = new unsigned char[p->len];

  gxString m;
  m << clear << "Thread[" << p->thread_number << "]: Buffer size is set to "
    << (unsigned long)p->len << " bytes" << "\n";
  console.Write(m);

  double start = drbench_now();
  p->ok = 1;
  for(int r = 0; r < p->repeat && p->ok; r++) {
    p->ok = RunPass(p, dest, dest_size, dup);
  }
  p->run_time = drbench_now() - start;
  delete[] dest;
  delete[] dup;

  m.Clear();
  m.Precision(3);
  if(p->ok) {
    m << "Thread[" << p->thread_number << "]: CPU speed test elapsed time "
      << p->run_time << " seconds" << "\n";
  }
  else {
    m << "Thread[" << p->thread_number << "]: Error: " << p->error << "\n";
  }
  console.Write(m);
  return 0;
}

void HelpMessage()
{
  cout << ProgramName << " version " << VersionString
       << " (dr_utils, DataReel 6.4.1)" << "\n";
  cout << "CPU speed benchmark: zlib compress and uncompress of random data"
       << "\n\n";
  cout << "Usage: " << ProgramName << " [args]" << "\n";
  cout << "args: " << "\n";
  cout << "     --help                Print help message and exit" << "\n";
  cout << "     --version             Print version number and exit" << "\n";
  cout << "     --num-threads=NUM     Number of threads to spawn (default 1,"
       << " max 1024)" << "\n";
  cout << "     --buf-size=SIZE       Size of memory buffer: 24k 240m 2.4g "
       << "(default 100m)" << "\n";
  cout << "     --datfile=fname       Write the results to a plot data file "
       << "(dr_plot.py)" << "\n";
  cout << "     --repeat=NUM          Passes over the buffer (default 1)"
       << "\n";
  cout << "\n";
  cout << "The buffer is split between the threads. Sizes are decimal "
       << "(1k = 1000)." << "\n";
  cout << "\n" << flush;
  drhook_usage("cputest");
}

int main(int argc, char **argv)
{
  unsigned long num_threads = 1;
  double buf_size = 100e6;
  unsigned long repeat = 1;
  gxString datfile;
  drhookOptions hook("cputest");

  for(int i = 1; i < argc; i++) {
    const char *v;
    if(drbench_switch(argv[i], "help", v) || strcmp(argv[i], "-?") == 0) {
      HelpMessage();
      return 0;
    }
    else if(drbench_switch(argv[i], "version", v)) {
      cout << ProgramName << " version " << VersionString << "\n" << flush;
      return 0;
    }
    else if(drbench_switch(argv[i], "num-threads", v)) {
      if(!drbench_parse_count(v, 1, 1024, num_threads)) {
	cout << "Error number of threads is not valid" << "\n" << flush;
	return 1;
      }
    }
    else if(drbench_switch(argv[i], "buf-size", v)) {
      if(!drbench_parse_size(v, 64e9, buf_size) || buf_size < 1000) {
	cout << "Error buffer size value is not valid" << "\n" << flush;
	return 1;
      }
    }
    else if(hook.Arg(argv[i])) ;
    else if(drbench_switch(argv[i], "datfile", v) && v && *v) datfile = v;
    else if(drbench_switch(argv[i], "repeat", v)) {
      if(!drbench_parse_count(v, 1, 1000000, repeat)) {
	cout << "Error repeat value is not valid" << "\n" << flush;
	return 1;
      }
    }
    else if(drbench_switch(argv[i], "verbose", v) ||
	    drbench_switch(argv[i], "debug", v)) {
      // Accepted for the old command lines
    }
    else {
      cout << "Error unknown command line switch " << argv[i] << "\n"
	   << flush;
      return 1;
    }
  }
  if(!hook.Check()) return 1;
  if(sizeof(size_t) < 8 && buf_size > 1.5e9) {
    cout << "Error a 32-bit build can use at most 1.5g" << "\n" << flush;
    return 1;
  }

  size_t total = (size_t)buf_size;
  size_t part = total / num_threads;
  if(part < 1000) {
    cout << "Error the buffer is too small for " << num_threads
	 << " threads" << "\n" << flush;
    return 1;
  }

  cout << ProgramName << " version " << VersionString << "\n";
  cout << "Host: " << drbench_hostname().c_str() << "\n";
  cout << "Threads: " << num_threads << ", buffer "
       << drbench_shn((double)total).c_str() << " bytes, " << repeat
       << " pass(es)" << "\n" << flush;

  unsigned char *buf = new (nothrow) unsigned char[total];
  if(!buf) {
    cout << "Error could not allocate " << (unsigned long)total << " bytes"
	 << "\n" << flush;
    return 1;
  }
  // Random bytes (xorshift), so zlib has real work to do
  unsigned long long x = 0x9E3779B97F4A7C15ULL;
  for(size_t i = 0; i < total; i++) {
    x ^= x << 13; x ^= x >> 7; x ^= x << 17;
    buf[i] = (unsigned char)(x >> 24);
  }

  CPUTestThread t;
  CPUThreadParm *parms = new CPUThreadParm[num_threads];
  gxThread_t **threads = new gxThread_t *[num_threads];
  double start = drbench_now();
  unsigned long i;
  for(i = 0; i < num_threads; i++) {
    parms[i].thread_number = (int)i;
    parms[i].buf = buf + i * part;
    parms[i].len = part;
    parms[i].repeat = (int)repeat;
    parms[i].run_time = 0;
    parms[i].ok = 0;
    threads[i] = t.CreateThread((void *)&parms[i]);
    if(!threads[i] || threads[i]->GetThreadError() != gxTHREAD_NO_ERROR) {
      cout << "Error could not start thread " << i << "\n" << flush;
      return 1;
    }
  }
  int failed = 0;
  int out_failed = 0;  // Plot file or dashboard output
  double sum = 0;
  for(i = 0; i < num_threads; i++) {
    t.JoinThread(threads[i]);
    if(!parms[i].ok) failed++;
    sum += parms[i].run_time;
    delete threads[i];
  }
  double elapsed = drbench_now() - start;
  double bytes = (double)part * num_threads * repeat;

  char line[256];
  sprintf(line, "Average thread time %.3f seconds", sum / num_threads);
  cout << line << "\n";
  sprintf(line, "Total elapsed time %.3f seconds, %s", elapsed,
	  drbench_rate(bytes, elapsed).c_str());
  cout << line << "\n" << flush;

  if(!datfile.is_null() || hook.Wanted()) {
    drbenchDat dat("cputest", VersionString, "CPU Speed Benchmark");
    dat.Set("threads", (double)num_threads);
    dat.Set("buf_size", (double)total);
    dat.Set("repeat", (double)repeat);
    dat.Set("total", bytes / (elapsed > 0 ? elapsed : 1e-9) / 1e6);
    dat.Set("total_unit", "MB/s");
    dat.Set("total_label", "zlib compress + uncompress rate");
    dat.Set("elapsed", elapsed);
    gxString items, vals;
    for(i = 0; i < num_threads; i++) {
      char name[32];
      sprintf(name, "Thread %lu", i);
      dat.Add(items, name);
      dat.Add(vals, parms[i].run_time);
    }
    dat.Set("items", items.c_str());
    dat.Set("item_values", vals.c_str());
    dat.Set("item_unit", "seconds");
    if(!datfile.is_null() && !dat.Write(datfile.c_str())) out_failed++;
    if(hook.Wanted()) {
      drhookLines lines;
      dat.Hook(lines);
      if(failed > 0) lines.Error("cputest: the run did not complete, see the table");
      if(!hook.Finish(lines)) out_failed++;
    }
  }
  delete[] threads;
  delete[] parms;
  delete[] buf;
  if(failed) {
    cout << failed << " thread(s) failed" << "\n" << flush;
    return 1;
  }
  return out_failed ? 1 : 0;
}
// ----------------------------------------------------------- //
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
