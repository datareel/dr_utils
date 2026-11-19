// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Source Code File Name: disktest.cpp
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

Disk speed benchmark with a DataReel B-tree index file: the database
index work of a disk. Four timed phases on N keys: insert, find,
delete (each delete checked) and insert again. Each phase prints its
time every 10% of the keys. --datfile writes the times in the plot
data format of the 2016-2019 disk benchmark ([DISKBENCHMARK],
[PLOTINFO] and [HOSTINFO] sections).

Changes:
11/18/2026: dr_utils version, based on Douglas Gaer's 2002-2019 disk
benchmark (bmarks.disk, then benchmarks/src/disktest), with the same
phases, B-tree order (128) and plot data keys. Times are wall clock
(the old version summed clock() around each call). New: --file to
put the test file on the disk to test, --keep, the delete check on
by default, the B-tree status at the end, Windows builds.
*/
// ----------------------------------------------------------- //
#include "gxdlcode.h"

#include <iostream>
#include <fstream>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gxbtree.h"
#include "futils.h"
#include "drbench.h"
#include "drutil.h"

const char *ProgramName = "disktest";
const char *VersionString = DR_UTILS_VERSION;
const BtreeNodeOrder_t MyKeyClassOrder = 128;

class MyKeyClass : public DatabaseKeyB
{
public:
  MyKeyClass() : DatabaseKeyB((char *)&data) { data = 0; }
  MyKeyClass(gxUINT32 i) : DatabaseKeyB((char *)&data) { data = i; }

public: // Base class interface
  size_t KeySize() { return sizeof(data); }
  int operator==(const DatabaseKeyB& key) const {
    const MyKeyClass *kptr = (const MyKeyClass *)(&key);
    return data == *((gxUINT32 *)kptr->db_key);
  }
  int operator>(const DatabaseKeyB& key) const {
    const MyKeyClass *kptr = (const MyKeyClass *)(&key);
    return data > *((gxUINT32 *)kptr->db_key);
  }
  int CompareKey(const DatabaseKeyB& key) const {
    const MyKeyClass *kptr = (const MyKeyClass *)(&key);
    gxUINT32 other = *((gxUINT32 *)kptr->db_key);
    if(data == other) return 0;
    return (data > other) ? 1 : -1;
  }

public:
  gxUINT32 data;
};

// One timed phase over all the keys
struct PhaseTimes {
  gxString chunk_times;  // Seconds for each 10% chunk
  gxString chunk_avgs;   // Milliseconds per key in each chunk
  double total;
};

enum PhaseType { PHASE_INSERT, PHASE_FIND, PHASE_DELETE, PHASE_REINSERT };

gxString xticks, xticks_str;

// Returns 1 if every key worked
int RunPhase(gxBtree &btx, PhaseType type, unsigned long n, PhaseTimes &pt)
{
  const char *verb[] = { "Inserted", "Found", "Deleted", "Reinserted" };
  MyKeyClass key, compare_key;
  unsigned long chunk = n / 10;
  if(chunk == 0) chunk = 1;
  pt.total = 0;
  pt.chunk_times.Clear();
  pt.chunk_avgs.Clear();
  int make_ticks = (type == PHASE_INSERT);

  double start = drbench_now();
  double chunk_start = start;
  unsigned long in_chunk = 0;
  for(unsigned long i = 0; i < n; i++) {
    key.data = (gxUINT32)i;
    int rv = 0;
    switch(type) {
      case PHASE_INSERT:
      case PHASE_REINSERT:
	rv = btx.Insert(key, compare_key, 0);
	break;
      case PHASE_FIND:
	rv = btx.Find(key, compare_key, 0);
	break;
      case PHASE_DELETE:
	rv = btx.Delete(key, compare_key, 0);
	if(rv == 1 && btx.Find(key, compare_key, 0)) { // Delete check
	  cout << "\n" << "Error key " << i << " still found after delete"
	       << "\n" << flush;
	  return 0;
	}
	break;
    }
    if(rv != 1) {
      cout << "\n" << "Problem with key " << i << " (" << verb[type]
	   << " phase)" << "\n";
      cout << btx.DatabaseExceptionMessage() << "\n" << flush;
      return 0;
    }
    if(++in_chunk >= chunk || i + 1 == n) {
      double now = drbench_now();
      double t = now - chunk_start;
      char buf[64];
      if(!pt.chunk_times.is_null()) {
	pt.chunk_times << ",";
	pt.chunk_avgs << ",";
      }
      sprintf(buf, "%.6f", t);
      pt.chunk_times << buf;
      sprintf(buf, "%.6f", t / in_chunk * 1000.0);
      pt.chunk_avgs << buf;
      if(make_ticks) {
	if(!xticks.is_null()) { xticks << ","; xticks_str << ","; }
	xticks << (i + 1);
	xticks_str << drbench_shn((double)(i + 1));
      }
      sprintf(buf, "%.3f", now - start);
      cout << verb[type] << " " << (i + 1) << " keys in " << buf
	   << " seconds" << "\n" << flush;
      chunk_start = now;
      in_chunk = 0;
    }
  }
  pt.total = drbench_now() - start;
  char buf[128];
  sprintf(buf, "%.3f seconds, %.0f keys a second", pt.total,
	  (double)n / (pt.total > 0 ? pt.total : 1e-9));
  cout << verb[type] << " " << n << " keys: " << buf << "\n\n" << flush;
  return 1;
}

void HelpMessage()
{
  cout << ProgramName << " version " << VersionString
       << " (dr_utils, DataReel 6.4.1)" << "\n";
  cout << "Disk speed benchmark: B-tree index insert, find, delete and "
       << "insert again" << "\n\n";
  cout << "Usage: " << ProgramName << " [args]" << "\n";
  cout << "args: " << "\n";
  cout << "     --help                Print help message and exit" << "\n";
  cout << "     --version             Print version number and exit" << "\n";
  cout << "     --num-inserts=NUM     Number of key insertions: 1k 1m 1g "
       << "(default 1m)" << "\n";
  cout << "     --file=fname          B-tree test file (default "
       << "testfile.btx in the" << "\n"
       << "                           current folder); put it on the disk "
       << "to test" << "\n";
  cout << "     --keep                Keep the test file (default: remove "
       << "it)" << "\n";
  cout << "     --datfile=fname       Name of plot data file for plotting"
       << "\n";
  cout << "\n" << flush;
}

int main(int argc, char **argv)
{
  unsigned long num_inserts = 1000000;
  gxString fname = "testfile.btx";
  gxString datfile;
  int keep = 0;

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
    else if(drbench_switch(argv[i], "num-inserts", v)) {
      if(!drbench_parse_count(v, 10, 4000000000UL, num_inserts)) {
	cout << "Error number of inserts is not valid (10 to 4g)" << "\n"
	     << flush;
	return 1;
      }
    }
    else if(drbench_switch(argv[i], "file", v) && v && *v) fname = v;
    else if(drbench_switch(argv[i], "datfile", v) && v && *v) datfile = v;
    else if(drbench_switch(argv[i], "keep", v)) keep = 1;
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

  if(futils_exists(fname.c_str())) {
    cout << "Error " << fname.c_str() << " exists; remove it or use "
	 << "--file" << "\n" << flush;
    return 1;
  }

  cout << ProgramName << " version " << VersionString << "\n";
  cout << "Host: " << drbench_hostname().c_str() << "\n";
  cout << "Benchmark will be performing " << num_inserts
       << " key insertions." << "\n";
  cout << "Benchmark output file is " << fname.c_str() << "\n\n" << flush;

  MyKeyClass key;
  gxBtree btx(key, MyKeyClassOrder);
  char rev_letter = gxDatabaseRevisionLetter;
  if(btx.Create(fname.c_str(), rev_letter) != gxDBASE_NO_ERROR) {
    cout << "Error creating " << fname.c_str() << ": "
	 << btx.DatabaseExceptionMessage() << "\n" << flush;
    return 1;
  }

  double start = drbench_now();
  PhaseTimes ins, fnd, del, rei;
  int ok = RunPhase(btx, PHASE_INSERT, num_inserts, ins) &&
    RunPhase(btx, PHASE_FIND, num_inserts, fnd) &&
    RunPhase(btx, PHASE_DELETE, num_inserts, del) &&
    RunPhase(btx, PHASE_REINSERT, num_inserts, rei);
  btx.Flush();
  double elapsed = drbench_now() - start;

  if(ok) {
    cout << "Root address =      " << (long long)(FAU_t)btx.Root() << "\n";
    cout << "Number of trees =   " << btx.NumTrees() << "\n";
    cout << "Number of entries = " << (unsigned long)btx.NumKeys() << "\n";
    cout << "Number of nodes =   " << (unsigned long)btx.NumNodes() << "\n";
    cout << "B-Tree order =      " << btx.NodeOrder() << "\n";
    cout << "B-Tree height =     " << btx.BtreeHeight() << "\n";
    if((unsigned long)btx.NumKeys() != num_inserts) {
      cout << "Error the tree has " << (unsigned long)btx.NumKeys()
	   << " keys, expected " << num_inserts << "\n";
      ok = 0;
    }
    char buf[64];
    sprintf(buf, "%.3f", elapsed);
    cout << "Disk speed test elapsed time " << buf << " seconds" << "\n"
	 << flush;
  }
  btx.Close();
  if(!keep) futils_remove(fname.c_str());

  if(ok && !datfile.is_null()) {
    ofstream df(datfile.c_str(), ios::out|ios::trunc);
    if(!df) {
      cout << "Error could not write " << datfile.c_str() << "\n" << flush;
      return 1;
    }
    char t[64];
    df << "[DISKBENCHMARK]" << "\n";
    df << "num_inserts=" << num_inserts << "\n";
    df << "num_inserts_str=" << drbench_shn((double)num_inserts).c_str()
       << "\n";
    df << "write_time=" << ins.chunk_times.c_str() << "\n";
    df << "write_time_avg=" << ins.chunk_avgs.c_str() << "\n";
    df << "xticks=" << xticks.c_str() << "\n";
    df << "xticks_str=" << xticks_str.c_str() << "\n";
    sprintf(t, "%.6f", ins.total); df << "insert_time=" << t << "\n";
    sprintf(t, "%.6f", ins.total / num_inserts * 1000.0);
    df << "insert_time_avg=" << t << "\n";
    df << "read_time=" << fnd.chunk_times.c_str() << "\n";
    df << "read_time_avg=" << fnd.chunk_avgs.c_str() << "\n";
    sprintf(t, "%.6f", fnd.total); df << "search_time=" << t << "\n";
    sprintf(t, "%.6f", fnd.total / num_inserts * 1000.0);
    df << "search_time_avg=" << t << "\n";
    df << "remove_time=" << del.chunk_times.c_str() << "\n";
    df << "remove_time_avg=" << del.chunk_avgs.c_str() << "\n";
    sprintf(t, "%.6f", del.total); df << "delete_time=" << t << "\n";
    sprintf(t, "%.6f", del.total / num_inserts * 1000.0);
    df << "delete_time_avg=" << t << "\n";
    df << "rewrite_time=" << rei.chunk_times.c_str() << "\n";
    df << "rewrite_time_avg=" << rei.chunk_avgs.c_str() << "\n";
    sprintf(t, "%.6f", rei.total); df << "reinsert_time=" << t << "\n";
    sprintf(t, "%.6f", rei.total / num_inserts * 1000.0);
    df << "reinsert_time_avg=" << t << "\n";
    df << "\n" << "[PLOTINFO]" << "\n" << "DPI=75" << "\n"
       << "plot_type=times" << "\n";
    df << "\n" << "[HOSTINFO]" << "\n";
    df << "host=" << drbench_hostname().c_str() << "\n";
    df << "date=" << drbench_date().c_str() << "\n";
    df << "build=" << drbench_build() << "\n";
    df << "title=Disk Speed Benchmark" << "\n";
    df.close();
    cout << "Plot data written to " << datfile.c_str() << "\n" << flush;
  }
  return ok ? 0 : 1;
}
// ----------------------------------------------------------- //
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
