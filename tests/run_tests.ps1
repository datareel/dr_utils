# dr_utils Windows tests. Build first (build.cmd), then run:
#   powershell -ExecutionPolicy Bypass -File tests\run_tests.ps1 [-NoLan]
#
# Network tests use only 127.0.0.1 and a small local HTTP server on a
# free port (a PowerShell TcpListener job). -NoLan skips the getmac and
# arpscan check of a host already in this machine's ARP table.
param([switch]$NoLan)

$ErrorActionPreference = 'Continue'
$Top = Split-Path -Parent $PSScriptRoot
$Bin = Join-Path $Top 'bin'
$Work = Join-Path $Top 'tests\work'
$script:pass = 0; $script:fail = 0; $script:Out = ''; $script:Rc = 0

function Ok($n) { $script:pass++; Write-Output "ok   $n" }
function Bad($n, $why) { $script:fail++; Write-Output "FAIL $n"; if ($why) { Write-Output "     $why" } }
function Run($exe, [string[]]$a) {
  $script:Out = (& (Join-Path $Bin "$exe.exe") @a 2>&1 | Out-String)
  $script:Rc = $LASTEXITCODE
}
function CheckRc($n, $want, $exe, [string[]]$a) {
  Run $exe $a
  if ($script:Rc -eq $want) { Ok $n } else { Bad $n "exit $($script:Rc), wanted $want`: $(($script:Out -split "`n" | Select-Object -First 3) -join ' | ')" }
}
function Has($n, $pat) {
  if ($script:Out -match $pat) { Ok $n } else { Bad $n "no /$pat/ in: $(($script:Out -split "`n" | Select-Object -First 5) -join ' | ')" }
}
function SetAge($f, [TimeSpan]$age) {
  if (-not (Test-Path $f)) { New-Item -ItemType File -Path $f | Out-Null }
  (Get-Item $f).LastWriteTime = (Get-Date) - $age
}
function FreePort {
  $l = [System.Net.Sockets.TcpListener]::new([System.Net.IPAddress]::Loopback, 0)
  $l.Start(); $p = $l.LocalEndpoint.Port; $l.Stop(); return $p
}

foreach ($t in 'file_hours','file_purge','filetimes','internet-test','pingscan','dnsscan','portscan','netpscan','arpscan','getmac',
             'cputest','disktest','memtest','tcpserv','tcptest','udpserv','udptest','mcserv','mctest','flops','sar_cpu_stats') {
  if (-not (Test-Path (Join-Path $Bin "$t.exe"))) { Write-Output "$Bin\$t.exe is missing: run build.cmd first"; exit 2 }
}

if (Test-Path $Work) { Remove-Item -Recurse -Force $Work }
New-Item -ItemType Directory -Path "$Work\purge", "$Work\moved" | Out-Null
Set-Location $Work

Write-Output '--- file_hours'
SetAge 'old3h.txt' ([TimeSpan]::FromHours(3))
SetAge 'new40m.txt' ([TimeSpan]::FromMinutes(40))
CheckRc 'file_hours 3h file is 2 hours old' 0 'file_hours' @('old3h.txt','2'); Has 'YES' '(?m)^YES'
CheckRc 'file_hours 3h file is not 5 hours old' 0 'file_hours' @('old3h.txt','5'); Has 'NOT' '(?m)^NOT'
CheckRc 'file_hours 40m file is 0.5 hours old' 0 'file_hours' @('new40m.txt','0.5'); Has 'YES under one hour' '(?m)^YES'
CheckRc 'file_hours -s NOT exits 1' 1 'file_hours' @('-s','new40m.txt','1')
CheckRc 'file_hours missing file' 1 'file_hours' @('nofile.txt','1'); Has 'ERR missing' 'ERR - nofile.txt does not exist'
CheckRc 'file_hours -s missing exits 2' 2 'file_hours' @('-s','nofile.txt','1')
CheckRc 'file_hours directory' 1 'file_hours' @('purge','1'); Has 'ERR dir' 'is not a file name'

Write-Output '--- filetimes'
CheckRc 'filetimes one file' 0 'filetimes' @('old3h.txt'); Has 'mod time' 'File modification time:'; Has 'creation' 'File creation time:'; Has 'age hours' 'File is 3\.0\d hour'
CheckRc 'filetimes -n newest' 0 'filetimes' @('-n','old3h.txt','new40m.txt'); Has 'newest' '(?m)^new40m.txt'
CheckRc 'filetimes -o oldest' 0 'filetimes' @('-o','old3h.txt','new40m.txt'); Has 'oldest' '(?m)^old3h.txt'

Write-Output '--- file_purge'
SetAge 'purge\a.log' ([TimeSpan]::FromDays(10)); SetAge 'purge\b.log' ([TimeSpan]::FromHours(2)); SetAge 'purge\c.log' ([TimeSpan]::Zero)
CheckRc 'file_purge -l (wildcard)' 0 'file_purge' @('-l','purge\*.log'); Has 'LST' 'LST - purge\\a.log \(240\.0\d\) hours \(10\.00\) days'
CheckRc 'file_purge -n dry run' 0 'file_purge' @('-n','-h1','purge\*.log'); Has 'DRY' 'DRY - purge\\b.log would be purged'
if ((Test-Path 'purge\a.log') -and (Test-Path 'purge\b.log')) { Ok 'file_purge -n kept the files' } else { Bad 'file_purge -n deleted files' }
CheckRc 'file_purge -h1.5' 0 'file_purge' @('-h1.5','purge\*.log'); Has 'b purged' 'YES - purge\\b.log purged'; Has 'c kept' 'NOT - purge\\c.log not purged'
if (-not (Test-Path 'purge\a.log') -and -not (Test-Path 'purge\b.log') -and (Test-Path 'purge\c.log')) { Ok 'file_purge -h deleted only old files' } else { Bad 'file_purge -h files' }
SetAge 'purge\e.log' ([TimeSpan]::FromDays(3))
CheckRc 'file_purge -m move' 0 'file_purge' @("-m$Work\moved",'-d2','purge\e.log'); Has 'moved' 'YES - purge\\e.log moved'
if ((Test-Path 'moved\e.log') -and -not (Test-Path 'purge\e.log')) { Ok 'file_purge -m moved the file' } else { Bad 'file_purge -m file' }
CheckRc 'file_purge -e' 0 'file_purge' @('-e','purge\c.log'); Has 'existing' 'existing file purged'
CheckRc 'file_purge -h and -d' 1 'file_purge' @('-h1','-d1','old3h.txt')
CheckRc 'file_purge no operation' 1 'file_purge' @('old3h.txt')
if (Test-Path 'old3h.txt') { Ok 'file_purge error cases changed nothing' } else { Bad 'old3h.txt deleted' }

Write-Output '--- range and argument errors'
CheckRc 'pingscan bad range' 2 'pingscan' @('300.1.1')
CheckRc 'pingscan /16 refused' 2 'pingscan' @('10.0')
CheckRc 'portscan too many ports' 2 'portscan' @('127.0.0.1','1-2000')
CheckRc 'netpscan missing list' 2 'netpscan' @("$Work\none.txt")

Write-Output '--- ping (IcmpSendEcho, no administrator rights)'
CheckRc 'pingscan 127.0.0.1' 0 'pingscan' @('-t','500','127.0.0.1'); Has 'reply' 'Host is up, reply in'; Has 'summary' '1 of 1 hosts answered'
CheckRc 'dnsscan 127.0.0.1 -a -p' 0 'dnsscan' @('-a','-p','-t','300','127.0.0.1')
Has 'dnsscan up' 'up \('

Write-Output '--- TCP (local HTTP server)'
$Port = FreePort; $Closed = FreePort
$job = Start-Job -ArgumentList $Port -ScriptBlock {
  param($p)
  $l = [System.Net.Sockets.TcpListener]::new([System.Net.IPAddress]::Loopback, $p)
  $l.Start()
  $end = (Get-Date).AddMinutes(3)
  while ((Get-Date) -lt $end) {
    if (-not $l.Pending()) { Start-Sleep -Milliseconds 50; continue }
    $c = $l.AcceptTcpClient(); $s = $c.GetStream(); $s.ReadTimeout = 2000
    $buf = New-Object byte[] 4096
    try { $null = $s.Read($buf, 0, $buf.Length) } catch {}
    $body = "hello`n"
    $r = [Text.Encoding]::ASCII.GetBytes("HTTP/1.0 200 OK`r`nContent-Type: text/plain`r`nContent-Length: $($body.Length)`r`nConnection: close`r`n`r`n$body")
    try { $s.Write($r, 0, $r.Length); $s.Flush() } catch {}
    $c.Close()
  }
  $l.Stop()
}
for ($i = 0; $i -lt 50; $i++) {
  try { $t = [System.Net.Sockets.TcpClient]::new('127.0.0.1', $Port); $t.Close(); break } catch { Start-Sleep -Milliseconds 100 }
}
"drtest`t$Port/tcp`tdrtestalias`t# dr_utils test port`nclosed`t$Closed/tcp`n" | Set-Content -Encoding ASCII -NoNewline services.txt
CheckRc 'portscan open port' 0 'portscan' @('-t','1000','127.0.0.1',"$Port",'services.txt'); Has 'open' "TCP port $Port open"; Has 'service name' 'Name: +drtest'
CheckRc 'portscan closed port' 1 'portscan' @('-t','1000','127.0.0.1',"$Closed",'services.txt'); Has 'closed' "TCP port $Closed closed"
"# test list`n127.0.0.1   # loopback`n" | Set-Content -Encoding ASCII iplist.txt
CheckRc 'netpscan services file' 0 'netpscan' @('-A','iplist.txt','services.txt'); Has 'netpscan open' "$Port/tcp drtest drtestalias # dr_utils test port"
CheckRc 'internet-test pass' 0 'internet-test' @('-c','1',"127.0.0.1:$Port"); Has 'passed' 'test passed .*HTTP status 200'
CheckRc 'internet-test fail' 1 'internet-test' @('-c','2','-w','0','-t','500',"127.0.0.1:$Closed"); Has 'failed after 2' 'failed after 2 tries'
Stop-Job $job -ErrorAction SilentlyContinue; Remove-Job $job -Force -ErrorAction SilentlyContinue

Write-Output '--- MAC addresses (SendARP)'
CheckRc 'getmac loopback has no ARP entry' 1 'getmac' @('127.0.0.1')
if (-not $NoLan) {
  $n = Get-NetNeighbor -AddressFamily IPv4 -State Reachable,Stale,Permanent -ErrorAction SilentlyContinue |
       Where-Object { $_.LinkLayerAddress -match '^[0-9A-F]{2}(-[0-9A-F]{2}){5}$' -and $_.LinkLayerAddress -ne '00-00-00-00-00-00' -and
                      $_.LinkLayerAddress -ne 'FF-FF-FF-FF-FF-FF' -and $_.IPAddress -notmatch '^(22[4-9]|23\d|255)\.' -and $_.IPAddress -notmatch '\.255$' } |
       Select-Object -First 1
  if ($n) {
    $mac = $n.LinkLayerAddress -replace '-', ':'
    CheckRc "getmac $($n.IPAddress)" 0 'getmac' @($n.IPAddress); Has 'getmac MAC' "MAC Address: $mac"
    CheckRc "arpscan $($n.IPAddress)" 0 'arpscan' @($n.IPAddress); Has 'arpscan MAC' "$mac"
  } else {
    Write-Output 'skip getmac/arpscan LAN check: no neighbor entry'
  }
}

Write-Output '--- benchmarks (small sizes, loopback only)'
CheckRc 'cputest 2 threads' 0 'cputest' @('--num-threads=2','--buf-size=4m'); Has 'cputest threads' 'Thread\[1\]: CPU speed test elapsed time'; Has 'cputest total' 'Total elapsed time .* (KB|MB|GB)/s'
CheckRc 'cputest bad size' 1 'cputest' @('--buf-size=abc')
CheckRc 'disktest 20k' 0 'disktest' @('--num-inserts=20k',"--file=$Work\disk.btx","--datfile=$Work\disk.dat")
Has 'disktest phases' 'Reinserted 20000 keys:'; Has 'disktest entries' 'Number of entries = 20000'
if (-not (Test-Path "$Work\disk.btx")) { Ok 'disktest removed its file' } else { Bad 'disktest left its file' }
$dat = Get-Content "$Work\disk.dat" -Raw
if ($dat -match '(?m)^xticks_str=2K,4K' -and $dat -match '\[HOSTINFO\]') { Ok 'disktest plot data' } else { Bad 'disktest plot data' }
CheckRc 'memtest 2 threads' 0 'memtest' @('--num-threads=2','--num-inserts=5k'); Has 'memtest phases' 'Thread\[1\]: Reinserted 5000 values'

function StartBg($exe, [string[]]$a, $log) {
  return Start-Process -FilePath (Join-Path $Bin "$exe.exe") -ArgumentList $a -NoNewWindow -PassThru -RedirectStandardOutput $log -RedirectStandardError "$log.err"
}
$TP = FreePort
$srv = StartBg 'tcpserv' @("-p$TP",'-l127.0.0.1','--count=2') "$Work\tcpserv.log"
Start-Sleep -Milliseconds 700
CheckRc 'tcptest upload 2 clients' 0 'tcptest' @("-p$TP",'-n2','-b65536','-s20M','127.0.0.1'); Has 'tcptest total' 'All tests complete: 2 of 2 clients'
$srv.WaitForExit(20000) | Out-Null
if ((Get-Content "$Work\tcpserv.log" -Raw) -match 'upload 20000000 bytes') { Ok 'tcpserv logged the uploads' } else { Bad 'tcpserv log' }
$srv = StartBg 'tcpserv' @("-p$TP",'-l127.0.0.1','--count=1') "$Work\tcpserv2.log"
Start-Sleep -Milliseconds 700
CheckRc 'tcptest download' 0 'tcptest' @('-d',"-p$TP",'-b65536','-s10M','127.0.0.1'); Has 'tcptest received' 'received 10000000 bytes'
$srv.WaitForExit(20000) | Out-Null
CheckRc 'tcptest no server' 1 'tcptest' @("-p$TP",'-s1M','127.0.0.1')
$UP = FreePort
$srv = StartBg 'udpserv' @("-p$UP",'-l127.0.0.1','--count=2') "$Work\udpserv.log"
Start-Sleep -Milliseconds 700
CheckRc 'udptest 2 clients at 200M' 0 'udptest' @("-p$UP",'-n2','-s5M','-r200M','127.0.0.1'); Has 'udptest report' 'server received \d+ \([0-9.]+% lost'
$srv.WaitForExit(20000) | Out-Null
if ((Get-Content "$Work\udpserv.log" -Raw) -match 'run .*datagrams') { Ok 'udpserv logged the runs' } else { Bad 'udpserv log' }
$rcv = StartBg 'mctest' @('-n2','-i127.0.0.1',"-p$UP",'--timeout=10') "$Work\mctest.log"
Start-Sleep -Milliseconds 700
CheckRc 'mcserv on loopback' 0 'mcserv' @('-i127.0.0.1',"-p$UP",'-s2M','-r100M'); Has 'mcserv sent' 'Sent 1333 datagrams'
$rcv.WaitForExit(20000) | Out-Null
if ((Get-Content "$Work\mctest.log" -Raw) -match 'Receiver\[1\]: run .*1333 datagrams') { Ok 'mctest received the run' } else { Bad 'mctest' (Get-Content "$Work\mctest.log" -Raw) }
CheckRc 'mcserv refuses a unicast group' 1 'mcserv' @('-g192.0.2.1')

Write-Output '--- flops and sar_cpu_stats'
CheckRc 'flops 2 threads' 0 'flops' @('--num-threads=2','--iterations=100m'); Has 'flops rate' 'Total elapsed time .* (MFLOPS|GFLOPS)'
CheckRc 'flops --dp' 0 'flops' @('--dp','--iterations=50m')
CheckRc 'flops --div' 0 'flops' @('--div','--num-threads=2','--iterations=100m'); Has 'flops div' 'a\*b\+a/b loops'
$Sar = Join-Path $Top 'tests\data\sar'
CheckRc 'sar RHEL 5' 0 'sar_cpu_stats' @("$Sar\sar_rhel5_partial.txt"); Has 'rhel5 samples' '52 samples of all CPUs, every 10 minutes'
CheckRc 'sar RHEL 10' 0 'sar_cpu_stats' @("$Sar\sar_rhel10.txt"); Has 'rhel10 iowait spikes' 'iowait >= 20.0%: 4'
CheckRc 'sar gap' 0 'sar_cpu_stats' @("$Sar\sar_gap.txt"); Has 'gap listed' 'gap: no samples for 70 minutes before 2018-09-30 01:40:01'
CheckRc 'sar steal spike' 0 'sar_cpu_stats' @("$Sar\sar_steal_spike.txt"); Has '241 flagged' 'steal 241.45'; Has 'day stops' 'gap: samples stop at 2018-09-24 00:10:01'
CheckRc 'sar --alarm' 3 'sar_cpu_stats' @('--alarm',"$Sar\sar_steal_spike.txt")
CheckRc 'sar csv' 0 'sar_cpu_stats' @('--csv','--all-cpus',"$Sar\sar_rhel7_ampm.txt"); Has 'csv row' 'rhel7-2.example.com,2018-09-30 00:10:01,all,0.33,0.00,0.24,0.25,0.00,0.00,0.00,0.00,0.00,99.19,ok'

Write-Output ''
Write-Output "$($script:pass) passed, $($script:fail) failed"
if ($script:fail -eq 0) { exit 0 } else { exit 1 }
