# Performance #
Similar output should be displayed within the QEMU Monitor.
```
Injection Statistics:
         | File: '/tmp/linux/tasklist.inject'
         | Injections: 1
         | Temp Removals/Resumes: 0
         | Hypercalls: 63
         | Average Load Time: 0 s 0 ms 182143 ns
         | Average Exec Time: 0 s 2 ms 808977 ns
         | Average Unload Time: 0 s 0 ms 100788 ns
         |
         | Average Hypercall Time: 0 s 0 ms 31031 ns
         | Average Hypercall Time per Injection: 0 s 1 ms 954964 ns
         | Total Hypercall Time: 0 s 1 ms 954964 ns
         |
         | Average Total Time: 0 s 3 ms 91908 ns
         | Average Exec Time w/o Hypercalls: 0 s 0 ms 854013 ns
         | Total Time: 0 s 3 ms 91908 ns
         ___________________________________

Injection Finished (Return value 0)!
        Switching to 'XTIER' Mode...
        The VM will be stopped...
(X-TIER >> 
```

# Processes #
Similar output should be printed on the console where the virtual machine was started from.

```
 Running Processes:

   PID   CMD
   ---   ---
     1   init
     2   kthreadd
     3   ksoftirqd/0
     5   kworker/u:0
     6   migration/0
     7   cpuset
     8   khelper
     9   netns
    10   sync_supers
    11   bdi-default
    12   kintegrityd
    13   kblockd
    14   kacpid
    15   kacpi_notify
    16   kacpi_hotplug
    17   ata_sff
    18   khubd
    19   md
    20   kworker/0:1
    21   khungtaskd
    23   kswapd0
    24   ksmd
    25   fsnotify_mark
    26   aio
    27   ecryptfs-kthrea
    28   crypto
    32   kthrotld
    33   scsi_eh_0
    34   scsi_eh_1
    35   kmpathd
    36   kworker/u:2
    38   kmpath_handlerd
    39   kondemand
    40   kconservative
    41   kworker/0:2
   249   kdmflush
   258   kdmflush
   271   jbd2/dm-0-8
   272   ext4-dio-unwrit
   321   upstart-udev-br
   323   udevd
   448   udevd
   449   udevd
   489   rsyslogd
   493   dbus-daemon
   575   upstart-socket-
   586   kpsmoused
   615   getty
   620   getty
   625   getty
   628   getty
   633   getty
   644   cron
   645   atd
   727   getty
   728   getty
   789   dhclient3
   805   sshd
   820   kworker/0:0
   821   flush-251:0


```