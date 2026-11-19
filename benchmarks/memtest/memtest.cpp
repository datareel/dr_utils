// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Source Code File Name: memtest.cpp
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

Memory benchmark with DataReel's unbalanced binary search tree
(gxBStree) of string keys "Item 0", "Item 1" and so on: small heap
allocations, string compares and pointer chasing. Each thread builds
its own tree and times four phases: insert, find, delete (each delete
checked) and insert again. The keys arrive in nearly sorted order, so
the tree is deep and the work grows faster than the number of keys,
as in the 2002-2019 test; compare runs with the same --num-inserts.

Changes:
11/18/2026: dr_utils version, based on work from the 2002-2019 memory
benchmark (bmarks.memory, then benchmarks/src/memtest): the same tree,
keys and default size (25k keys, one thread). Times are wall clock
(the old clock() times counted every thread's CPU time on Linux).
New: a find phase, the delete check on by default, totals for all
threads, Windows builds.
*/
// ----------------------------------------------------------- //
#include "gxdlcode.h"

#include <iostream>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gxthread.h"
#include "gxbstree.h"
#include "drbench.h"
#include "drutil.h"

const char *ProgramName = "memtest";
const char *VersionString = DR_UTILS_VERSION;

drbenchConsole console;

struct MemThreadParm {
  int thread_number;
  unsigned long num_inserts;
  double times[4];   // insert, find, delete, reinsert
  double run_time;
  int ok;
  gxString error;
};

class MemoryTestThread : public gxThread
{
private:
  void *ThreadEntryRoutine(gxThread_t *thread);
};

void MakeKey(gxString &key, unsigned long i)
{
  char sbuf[64];
  sprintf(sbuf, "Item %lu", i);
  key = sbuf;
}

// Returns 1 if every key worked
int RunPhase(gxBStree<gxString> &tree, int phase, MemThreadParm *p)
{
  const char *verb[] = { "Inserted", "Found", "Deleted", "Reinserted" };
  gxString key;
  double start = drbench_now();
  for(unsigned long i = 0; i < p->num_inserts; i++) {
    MakeKey(key, i);
    int rv = 0;
    switch(phase) {
      case 0: case 3: rv = tree.Insert(key); break;
      case 1: rv = tree.Find(key); break;
      case 2:
	rv = tree.Delete(key);
	if(rv && tree.Find(key)) {
	  p->error << clear << key << " still found after delete";
	  return 0;
	}
	break;
    }
    if(!rv) {
      p->error << clear << "Problem with key " << key << " ("
	       << verb[phase] << " phase)";
      return 0;
    }
  }
  p->times[phase] = drbench_now() - start;
  gxString m;
  char buf[64];
  sprintf(buf, "%.3f", p->times[phase]);
  m << "Thread[" << p->thread_number << "]: " << verb[phase] << " "
    << p->num_inserts << " values in " << buf << " seconds" << "\n";
  console.Write(m);
  return 1;
}

void *MemoryTestThread::ThreadEntryRoutine(gxThread_t *thread)
{
  MemThreadParm *p = (MemThreadParm *)thread->GetThreadParm();
  gxBStree<gxString> tree;
  double start = drbench_now();
  p->ok = 1;
  for(int phase = 0; phase < 4 && p->ok; phase++) {
    p->ok = RunPhase(tree, phase, p);
  }
  tree.Clear();
  p->run_time = drbench_now() - start;
  gxString m;
  char buf[64];
  sprintf(buf, "%.3f", p->run_time);
  if(p->ok) {
    m << "Thread[" << p->thread_number << "]: Memory test elapsed time "
      << buf << " seconds" << "\n";
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
  cout << "Memory benchmark: binary search tree insert, find, delete and "
       << "insert again" << "\n\n";
  cout << "Usage: " << ProgramName << " [args]" << "\n";
  cout << "args: " << "\n";
  cout << "     --help                Print help message and exit" << "\n";
  cout << "     --version             Print version number and exit" << "\n";
  cout << "     --num-threads=NUM     Number of threads to spawn (default 1,"
       << " max 1024)" << "\n";
  cout << "     --datfile=fname       Write the results to a plot data file "
       << "(dr_plot.py)" << "\n";
  cout << "     --num-inserts=NUM     Number of key insertions per thread: "
       << "1k 25k 100k" << "\n"
       << "                           (default 25k, max 1m)" << "\n";
  cout << "\n" << flush;
}

int main(int argc, char **argv)
{
  unsigned long num_threads = 1;
  unsigned long num_inserts = 25000;
  gxString datfile;

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
    else if(drbench_switch(argv[i], "num-inserts", v)) {
      if(!drbench_parse_count(v, 10, 1000000, num_inserts)) {
	cout << "Error number of inserts is not valid (10 to 1m)" << "\n"
	     << flush;
	return 1;
      }
    }
    else if(drbench_switch(argv[i], "datfile", v) && v && *v) datfile = v;
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

  cout << ProgramName << " version " << VersionString << "\n";
  cout << "Host: " << drbench_hostname().c_str() << "\n";
  cout << "Threads: " << num_threads << ", " << num_inserts
       << " key insertions per thread" << "\n" << flush;

  MemoryTestThread t;
  MemThreadParm *parms = new MemThreadParm[num_threads];
  gxThread_t **threads = new gxThread_t *[num_threads];
  double start = drbench_now();
  unsigned long i;
  for(i = 0; i < num_threads; i++) {
    parms[i].thread_number = (int)i;
    parms[i].num_inserts = num_inserts;
    parms[i].run_time = 0;
    parms[i].ok = 0;
    for(int k = 0; k < 4; k++) parms[i].times[k] = 0;
    // A large stack: the tree is freed recursively
    threads[i] = t.CreateThread((void *)&parms[i], gxTHREAD_TYPE_JOINABLE,
				(gxStackSizeType)(64 * 1024 * 1024));
    if(!threads[i] || threads[i]->GetThreadError() != gxTHREAD_NO_ERROR) {
      cout << "Error could not start thread " << i << "\n" << flush;
      return 1;
    }
  }
  int failed = 0;
  double sums[4] = { 0, 0, 0, 0 };
  for(i = 0; i < num_threads; i++) {
    t.JoinThread(threads[i]);
    if(!parms[i].ok) failed++;
    for(int k = 0; k < 4; k++) sums[k] += parms[i].times[k];
    delete threads[i];
  }
  double elapsed = drbench_now() - start;

  char line[256];
  sprintf(line, "Average thread times: insert %.3f, find %.3f, delete %.3f,"
	  " reinsert %.3f seconds", sums[0] / num_threads,
	  sums[1] / num_threads, sums[2] / num_threads, sums[3] / num_threads);
  cout << line << "\n";
  sprintf(line, "Total elapsed time %.3f seconds", elapsed);
  cout << line << "\n" << flush;

  if(!datfile.is_null()) {
    drbenchDat dat("memtest", VersionString, "Memory Benchmark");
    dat.Set("threads", (double)num_threads);
    dat.Set("num_inserts", (double)num_inserts);
    dat.Set("total", elapsed);
    dat.Set("total_unit", "seconds");
    dat.Set("total_label", "Total elapsed time");
    dat.Set("items", "Insert,Find,Delete,Reinsert");
    gxString vals;
    for(int k = 0; k < 4; k++) dat.Add(vals, sums[k] / num_threads);
    dat.Set("item_values", vals.c_str());
    dat.Set("item_unit", "seconds (thread average)");
    if(!dat.Write(datfile.c_str())) failed++;
  }
  delete[] threads;
  delete[] parms;
  if(failed) {
    cout << failed << " thread(s) failed" << "\n" << flush;
    return 1;
  }
  return 0;
}
// ----------------------------------------------------------- //
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
