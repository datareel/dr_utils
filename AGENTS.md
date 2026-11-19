# AGENTS.md: notes for AI coding assistants

dr_utils is a set of small command line file and network tools on the DataReel C++ library 6.4.1, for Windows and Linux. Read README.md first.

## Rules

- License: GNU LGPL 3 or later. Every source file has the DataReel LGPL 3 header; new files get the same header. Credit the original program in the header's Changes.
- No credentials, real host names, internal addresses or email addresses in code, tests or docs. Examples use 192.168.1.x, 192.0.2.x and example.com.
- The tests touch only 127.0.0.1, a local HTTP server and (getmac/arpscan) one host already in the machine's ARP table. Do not add tests that scan other hosts or networks.
- The network tools scan one /24 at most. Keep that limit.
- No build output in commits (`*.o`, `*.obj`, `*.exe`, `bin/`). Stage explicit paths.
- Commit messages: no Co-Authored-By, model names or session lines.

## Build and test

```
DATAREEL_DIR=/path/to/datareel-6.4.1 ./build.sh && tests/run_tests.sh     # Linux
set DATAREEL_DIR=C:\path\to\datareel-6.4.1 & build.cmd                    # Windows, x64 tools prompt
powershell -ExecutionPolicy Bypass -File tests\run_tests.ps1
```

Build one tool folder at a time with `cd DIR && make`, never `cd DIR; make`.

## Code layout

- `common/drnet.cpp` includes winsock2.h before any DataReel header (DataReel's gxstypes.h includes winsock.h). `common/drnet.h` has no socket headers, so tools can include it with any DataReel header.
- Shared code goes in `common/`, and each tool stays a single .cpp file.
- Keep the old output lines (YES/NOT/ERR, LST, "Contacting host:") so existing scripts keep working.
