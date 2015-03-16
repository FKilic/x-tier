# Performance #
Similar output should be displayed within the QEMU Monitor.
```
Injection Statistics:
         | File: '/tmp/linux/files.inject'
         | Injections: 1
         | Temp Removals/Resumes: 103
         | Hypercalls: 84
         | Average Load Time: 0 s 0 ms 391033 ns
         | Average Exec Time: 0 s 11 ms 979642 ns
         | Average Unload Time: 0 s 0 ms 50553 ns
         |
         | Average Hypercall Time: 0 s 0 ms 37896 ns
         | Average Hypercall Time per Injection: 0 s 3 ms 183306 ns
         | Total Hypercall Time: 0 s 3 ms 183306 ns
         |
         | Average Temp Removal Time: 0 s 0 ms 28145 ns
         | Average Temp Resume Time: 0 s 0 ms 20881 ns
         | Average Temp Removal/Resume Time: 0 s 0 ms 49027 ns
         | Average Temp Removal/Resume Time per Injection: 0 s 5 ms 49782 ns
         | Total Temp Removal/Resume Time: 0 s 5 ms 49782 ns
         |
         | Average Total Time: 0 s 12 ms 421228 ns
         | Average Exec Time w/o TEMP Removal/Resume: 0 s 6 ms 929860 ns
         | Average Exec Time w/o Hypercalls: 0 s 8 ms 796336 ns
         | Average Exec Time w/o TEMP R/R & Hypercalls: 0 s 3 ms 746554 ns
         | Total Time: 0 s 12 ms 421228 ns
         ___________________________________

Injection Finished (Return value 0)!
        Switching to 'XTIER' Mode...
        The VM will be stopped...
(X-TIER >> 

```

# Files #
Similar output should be printed on the console where the virtual machine was started from.

```
Open Files:

   PID   CMD                     NAME 
   ---   ---                     ---- 
     1   init                    /dev/null
                                 /dev/null
                                 /dev/null

   321   upstart-udev-br         /dev/null
                                 /dev/null
                                 /dev/null

   323   udevd                   /dev/null
                                 /dev/null
                                 /dev/null
                                 /dev/.udev/queue.bin

   448   udevd                   /dev/null
                                 /dev/null
                                 /dev/null

   449   udevd                   /dev/null
                                 /dev/null
                                 /dev/null

   489   rsyslogd                /var/log/auth.log
                                 /proc/kmsg
                                 /var/log/syslog
                                 /var/log/kern.log

   493   dbus-daemon             /dev/null
                                 /dev/null
                                 /dev/null
                                 /dev/null

   575   upstart-socket-         /dev/null
                                 /dev/null
                                 /dev/null

   615   getty                   /dev/tty4
                                 /dev/tty4
                                 /dev/tty4

   620   getty                   /dev/tty5
                                 /dev/tty5
                                 /dev/tty5

   625   getty                   /dev/tty2
                                 /dev/tty2
                                 /dev/tty2

   628   getty                   /dev/tty3
                                 /dev/tty3
                                 /dev/tty3

   633   getty                   /dev/tty6
                                 /dev/tty6
                                 /dev/tty6

   644   cron                    /dev/null
                                 /dev/null
                                 /dev/null
                                 /var/run/crond.pid

   645   atd                     /dev/null
                                 /dev/null
                                 /dev/null
                                 /var/run/atd.pid

   727   getty                   /dev/tty1
                                 /dev/tty1
                                 /dev/tty1

   728   getty                   /dev/ttyS0
                                 /dev/ttyS0
                                 /dev/ttyS0

   789   dhclient3               /dev/null
                                 /dev/null
                                 /dev/null

   805   sshd                    /dev/null
                                 /dev/null
                                 /dev/null

```