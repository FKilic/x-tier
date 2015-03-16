# Performance #
Similar output should be displayed within the QEMU Monitor.
```
Injection Statistics:
         | File: '/tmp/linux/lsmod.inject'
         | Injections: 1
         | Temp Removals/Resumes: 0
         | Hypercalls: 12
         | Average Load Time: 0 s 0 ms 186605 ns
         | Average Exec Time: 0 s 0 ms 608535 ns
         | Average Unload Time: 0 s 0 ms 100279 ns
         |
         | Average Hypercall Time: 0 s 0 ms 35181 ns
         | Average Hypercall Time per Injection: 0 s 0 ms 422176 ns
         | Total Hypercall Time: 0 s 0 ms 422176 ns
         |
         | Average Total Time: 0 s 0 ms 895419 ns
         | Average Exec Time w/o Hypercalls: 0 s 0 ms 186359 ns
         | Total Time: 0 s 0 ms 895419 ns
         ___________________________________

Injection Finished (Return value 0)!
        Switching to 'XTIER' Mode...
        The VM will be stopped...
(X-TIER >> 
```

# Modules #
Similar output should be printed on the console where the virtual machine was started from.

```
Loaded Modules:

 BASE ADDRESS      SIZE          NAME
 ------------      ----          ----
0x7ff0fffc7000     17113         ppdev
0x7ff0fb254000     73535         psmouse
0x7ff0fdc58000     36959         parport_pc
0x7ff0fb32e000     13166         serio_raw
0x7ff0fec10000     13303         i2c_piix4
0x7ff0fe782000     17789         lp
0x7ff0fee86000     46458         parport
0x7ff0faaee000     74120         floppy
0x7ff0faa68000    111862         e1000
```