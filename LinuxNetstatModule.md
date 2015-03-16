# Performance #
Similar output should be displayed within the QEMU Monitor.
```
Injection Statistics:
         | File: '/tmp/linux/netstat.inject'
         | Injections: 1
         | Temp Removals/Resumes: 0
         | Hypercalls: 5
         | Average Load Time: 0 s 0 ms 166135 ns
         | Average Exec Time: 0 s 0 ms 562558 ns
         | Average Unload Time: 0 s 0 ms 101321 ns
         |
         | Average Hypercall Time: 0 s 0 ms 42306 ns
         | Average Hypercall Time per Injection: 0 s 0 ms 211531 ns
         | Total Hypercall Time: 0 s 0 ms 211531 ns
         |
         | Average Total Time: 0 s 0 ms 830014 ns
         | Average Exec Time w/o Hypercalls: 0 s 0 ms 351027 ns
         | Total Time: 0 s 0 ms 830014 ns
         ___________________________________

Injection Finished (Return value 0)!
        Switching to 'XTIER' Mode...
        The VM will be stopped...
(X-TIER >> 
```

# Network Connections #
Similar output should be printed on the console where the virtual machine was started from.

```
 SOURCE IP                PORT   DESTINATION IP            PORT
 ---------                ----   --------------            ----
 10.0.2.2                 42550   10.0.2.15                22  

```