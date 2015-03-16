# Performance #
Similar output should be displayed within the QEMU Monitor.
```
Injection Statistics:
         | File: '/tmp/win/tasklist.inject'
         | Injections: 1
         | Temp Removals/Resumes: 0
         | Hypercalls: 4
         | Average Load Time: 0 s 0 ms 105768 ns
         | Average Exec Time: 0 s 0 ms 217410 ns
         | Average Unload Time: 0 s 0 ms 17943 ns
         |
         | Average Hypercall Time: 0 s 0 ms 41623 ns
         | Average Hypercall Time per Injection: 0 s 0 ms 166492 ns
         | Total Hypercall Time: 0 s 0 ms 166492 ns
         |
         | Average Total Time: 0 s 0 ms 341121 ns
         | Average Exec Time w/o Hypercalls: 0 s 0 ms 50918 ns
         | Total Time: 0 s 0 ms 341121 ns
         ___________________________________

Injection Finished (Return value 0)!
        Switching to 'XTIER' Mode...
        The VM will be stopped...
(X-TIER >> 
```

# Processes #
Similar output should be printed on the console where the virtual machine was started from.

```
 EPROCESS         PID           NAME
  --------         ---   ----
0xe272c960        1976   sppsvc.exe
0xe298cb18        2012   svchost.exe
0xe273cd40         248   SearchIndexer.
0xe3ff4b98           4   System
0xc9497020         212   smss.exe
0xe2e007e8         296   csrss.exe
0xe28e5d40         332   wininit.exe
0xe28e4030         344   csrss.exe
0xe28fbd40         392   winlogon.exe
0xe29012e0         428   services.exe
0xe290ac30         436   lsass.exe
0xe290d030         444   lsm.exe
0xe294d560         552   svchost.exe
0xcbed6578         616   svchost.exe
0xe2975408         668   svchost.exe
0xe29a9530         740   LogonUI.exe
0xe2618530         788   svchost.exe
0xc9caa4b8         820   svchost.exe
0xe2648b18         944   svchost.exe
0xe26572b0        1020   svchost.exe
0xe28dfd40        1136   spoolsv.exe
0xe2941338        1172   svchost.exe
0xe29a1030        1284   svchost.exe
```