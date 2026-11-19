// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Source Code File Name: flops.cpp
// C++ Compiler Used: MSVC, GCC
// Produced By: DataReel Software Development Team
// File Creation Date: 2016
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

Floating point operations per second (FLOPS) benchmark, single or
double precision, in one or more threads. Each thread runs
multiply-add chains over a small array that stays in the CPU cache
(two floating point operations per element per pass), so the result
is the compute rate of the cores, not of memory. The iterations are
split between the threads.

The rate depends on the compiler and its flags (vector units): use
the same build to compare machines.

Changes:
11/18/2026: dr_utils version, based on Douglas Gaer's 2016-2019 FLOPS
benchmark (work_programs benchmarks/src/flops), with the same
switches (--num-threads, --dp, --iterations). The old loop had a
result that was never used, so an optimizing compiler removed it and
it timed nothing; this one keeps every result, checks the sums are
finite, and prints the time and MFLOPS or GFLOPS.
*/
// ----------------------------------------------------------- //
#include "gxdlcode.h"

#include <iostream>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "gxthread.h"
#include "drbench.h"
#include "drutil.h"

const char *ProgramName = "flops";
const char *VersionString = DR_UTILS_VERSION;
const int ArraySize = 1024; // 4 or 8 KB: in the L1 cache

drbenchConsole console;

struct FLOPSThreadParm {
  int thread_number;
  int dp;
  unsigned long long iterations; // Multiply-adds for this thread
  double secs;
  double result;
};

class FLOPSTestThread : public gxThread
{
private:
  void *ThreadEntryRoutine(gxThread_t *thread);
};

template<class T> double RunTest(unsigned long long iterations)
{
  T a[ArraySize];
  T b = (T)0.999999, c = (T)0.000001;
  for(int i = 0; i < ArraySize; i++) a[i] = (T)1.0 + (T)i / (T)ArraySize;
  unsigned long long passes = iterations / ArraySize;
  if(passes == 0) passes = 1;
  for(unsigned long long p = 0; p < passes; p++) {
    for(int i = 0; i < ArraySize; i++) {
      a[i] = a[i] * b + c; // One multiply and one add
    }
  }
  double sum = 0;
  for(int i = 0; i < ArraySize; i++) sum += (double)a[i];
  return sum; // Used by the caller, so the loop cannot be removed
}

void *FLOPSTestThread::ThreadEntryRoutine(gxThread_t *thread)
{
  FLOPSThreadParm *p = (FLOPSThreadParm *)thread->GetThreadParm();
  double start = drbench_now();
  p->result = p->dp ? RunTest<double>(p->iterations) :
    RunTest<float>(p->iterations);
  p->secs = drbench_now() - start;
  char line[256];
  sprintf(line, "Thread[%d]: %s precision, %llu multiply-adds in %.3f "
	  "seconds\n", p->thread_number, p->dp ? "Double" : "Single",
	  p->iterations, p->secs);
  console.Write(gxString(line));
  return 0;
}

gxString FlopsString(double flops)
{
  char buf[64];
  if(flops >= 1e9) sprintf(buf, "%.2f GFLOPS", flops / 1e9);
  else sprintf(buf, "%.2f MFLOPS", flops / 1e6);
  return gxString(buf);
}

void HelpMessage()
{
  cout << ProgramName << " version " << VersionString
       << " (dr_utils, DataReel 6.4.1)" << "\n";
  cout << "Floating point operations per second benchmark" << "\n\n";
  cout << "Usage: " << ProgramName << " [args]" << "\n";
  cout << "args: " << "\n";
  cout << "     --help                Print help message and exit" << "\n";
  cout << "     --version             Print version number and exit" << "\n";
  cout << "     --num-threads=NUM     Number of threads to run (default 1, "
       << "max 1024)" << "\n";
  cout << "     --dp                  Double precision test (defaults to "
       << "single precision)" << "\n";
  cout << "     --iterations=NUM      Multiply-adds in all: 1m 1g 10g "
       << "(default 1g)" << "\n";
  cout << "\n" << flush;
}

int main(int argc, char **argv)
{
  unsigned long num_threads = 1;
  double iterations = 1e9;
  int dp = 0;
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
    else if(drbench_switch(argv[i], "iterations", v)) {
      if(!drbench_parse_size(v, 1e15, iterations) || iterations < 1e4) {
	cout << "Error number of iterations is not valid (10k or more)"
	     << "\n" << flush;
	return 1;
      }
    }
    else if(drbench_switch(argv[i], "dp", v)) dp = 1;
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

  unsigned long long per_thread =
    (unsigned long long)(iterations / num_threads);
  per_thread -= per_thread % ArraySize; // Whole passes
  if(per_thread < (unsigned long long)ArraySize) per_thread = ArraySize;

  cout << ProgramName << " version " << VersionString << "\n";
  cout << "Host: " << drbench_hostname().c_str() << "\n";
  cout << "Iterations for " << num_threads << " threads = " << per_thread
       << " each, " << (dp ? "double" : "single") << " precision" << "\n"
       << flush;

  FLOPSTestThread t;
  FLOPSThreadParm *parms = new FLOPSThreadParm[num_threads];
  gxThread_t **threads = new gxThread_t *[num_threads];
  double start = drbench_now();
  unsigned long i;
  for(i = 0; i < num_threads; i++) {
    parms[i].thread_number = (int)i;
    parms[i].dp = dp;
    parms[i].iterations = per_thread;
    parms[i].secs = 0;
    parms[i].result = 0;
    threads[i] = t.CreateThread((void *)&parms[i]);
    if(!threads[i] || threads[i]->GetThreadError() != gxTHREAD_NO_ERROR) {
      cout << "Error could not start thread " << i << "\n" << flush;
      return 1;
    }
  }
  int bad = 0;
  for(i = 0; i < num_threads; i++) {
    t.JoinThread(threads[i]);
    double r = parms[i].result;
    if(!(r == r) || r > 1e300 || r < -1e300) bad++; // NaN or infinite
    delete threads[i];
  }
  double elapsed = drbench_now() - start;
  double flops = 2.0 * (double)per_thread * num_threads;
  char line[256];
  sprintf(line, "Total elapsed time %.3f seconds, %s", elapsed,
	  FlopsString(flops / (elapsed > 0 ? elapsed : 1e-9)).c_str());
  cout << line << "\n" << flush;
  delete[] threads;
  delete[] parms;
  if(bad) {
    cout << "Error " << bad << " thread(s) did not get a finite result"
	 << "\n" << flush;
    return 1;
  }
  return 0;
}
// ----------------------------------------------------------- //
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
