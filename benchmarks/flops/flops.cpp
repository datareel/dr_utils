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
11/18/2026: dr_utils version, based on work from the 2016-2019 FLOPS
benchmark (work_programs benchmarks/src/flops), with the same
switches (--num-threads, --dp, --iterations). The old loop had a
result that was never used, so an optimizing compiler removed it and
it timed nothing; this one keeps every result, checks the sums are
finite, and prints the time and MFLOPS or GFLOPS. --div runs the
formula of the c/flops program (a*b + a/b), counted as the 3
operations it is (c/flops counted 5).
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
  int div;
  unsigned long long iterations; // Multiply-adds for this thread
  double secs;
  double result;
};

class FLOPSTestThread : public gxThread
{
private:
  void *ThreadEntryRoutine(gxThread_t *thread);
};

// The c/flops formula: a*c + a/b, a multiply, a divide and an add
// (3 floating point operations; the old program counted 5). The
// divide makes this much slower than multiply-add: it measures what
// code with divisions gets.
template<class T> double RunDivTest(unsigned long long iterations)
{
  T a[ArraySize];
  // Not a power of two, and read at run time, so the compiler cannot
  // turn the divide into a multiply
  volatile T vb = (T)3.0, vc = (T)(2.0 / 3.0);
  T b = vb, c = vc; // c + 1/b = 1, so the values stay near 1.0
  for(int i = 0; i < ArraySize; i++) a[i] = (T)1.0 + (T)i / (T)ArraySize;
  unsigned long long passes = iterations / ArraySize;
  if(passes == 0) passes = 1;
  for(unsigned long long p = 0; p < passes; p++) {
    for(int i = 0; i < ArraySize; i++) {
      a[i] = a[i] * c + a[i] / b; // Multiply, divide, add
    }
  }
  double sum = 0;
  for(int i = 0; i < ArraySize; i++) sum += (double)a[i];
  return sum;
}

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
  sprintf(line, "Thread[%d]: %s precision, %llu %s in %.3f seconds\n",
	  p->thread_number, p->dp ? "Double" : "Single", p->iterations,
	  p->div ? "a*b+a/b loops" : "multiply-adds", p->secs);
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
  cout << "     --div                 The c/flops test: a*b + a/b (3 FLOPs, "
       << "with a divide)" << "\n";
  cout << "     --datfile=fname       Write the results to a plot data file "
       << "(dr_plot.py)" << "\n";
  cout << "     --iterations=NUM      Loop iterations in all: 1m 1g 10g "
       << "(default 1g)" << "\n";
  cout << "\n" << flush;
}

int main(int argc, char **argv)
{
  unsigned long num_threads = 1;
  double iterations = 1e9;
  int dp = 0, div = 0;
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
    else if(drbench_switch(argv[i], "iterations", v)) {
      if(!drbench_parse_size(v, 1e15, iterations) || iterations < 1e4) {
	cout << "Error number of iterations is not valid (10k or more)"
	     << "\n" << flush;
	return 1;
      }
    }
    else if(drbench_switch(argv[i], "dp", v)) dp = 1;
    else if(drbench_switch(argv[i], "div", v)) div = 1;
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

  unsigned long long per_thread =
    (unsigned long long)(iterations / num_threads);
  per_thread -= per_thread % ArraySize; // Whole passes
  if(per_thread < (unsigned long long)ArraySize) per_thread = ArraySize;

  cout << ProgramName << " version " << VersionString << "\n";
  cout << "Host: " << drbench_hostname().c_str() << "\n";
  cout << "Iterations for " << num_threads << " threads = " << per_thread
       << " each, " << (dp ? "double" : "single") << " precision, "
       << (div ? "a*b + a/b (3 FLOPs a loop)" : "multiply-add (2 FLOPs a loop)")
       << "\n"
       << flush;

  FLOPSTestThread t;
  FLOPSThreadParm *parms = new FLOPSThreadParm[num_threads];
  gxThread_t **threads = new gxThread_t *[num_threads];
  double start = drbench_now();
  unsigned long i;
  for(i = 0; i < num_threads; i++) {
    parms[i].thread_number = (int)i;
    parms[i].dp = dp;
    parms[i].div = div;
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
  double flops = (div ? 3.0 : 2.0) * (double)per_thread * num_threads;
  char line[256];
  sprintf(line, "Total elapsed time %.3f seconds, %s", elapsed,
	  FlopsString(flops / (elapsed > 0 ? elapsed : 1e-9)).c_str());
  cout << line << "\n" << flush;
  if(!datfile.is_null()) {
    drbenchDat dat("flops", VersionString, "FLOPS Benchmark");
    dat.Set("threads", (double)num_threads);
    dat.Set("precision", dp ? "double" : "single");
    dat.Set("mode", div ? "a*c+a/b" : "multiply-add");
    dat.Set("total", flops / (elapsed > 0 ? elapsed : 1e-9) / 1e9);
    dat.Set("total_unit", "GFLOPS");
    dat.Set("total_label", div ? "a*c + a/b rate" : "Multiply-add rate");
    dat.Set("elapsed", elapsed);
    gxString items, vals;
    for(i = 0; i < num_threads; i++) {
      char name[32];
      sprintf(name, "Thread %lu", i);
      dat.Add(items, name);
      dat.Add(vals, parms[i].secs);
    }
    dat.Set("items", items.c_str());
    dat.Set("item_values", vals.c_str());
    dat.Set("item_unit", "seconds");
    if(!dat.Write(datfile.c_str())) bad++;
  }
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
