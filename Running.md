# Overview #
In the following we will describe the individual steps that are necessary to build and run X-TIER. Before doing so, however, we will first state the system requirements of X-TIER.

# Requirements #
X-TIER is implemented on top of [KVM](http://www.linux-kvm.org/) and [QEMU](http://www.qemu.org/). To run X-TIER your system needs to meet the following requirements:
  * Intel x86\_64 CPU with Virtualization Extensions (X-TIER is currently only implemented for [VT-x](http://www.intel.com))
  * 64-bit Linux Host Operating System
  * `sudo` capabilities to load the X-TIER KVM modules

More information about the systems that X-TIER has been tested on can be found [here](TestEnvironment.md).

# Building & Running X-TIER #
1. **Clone the GIT repository**
```
 ~/$ git clone https://code.google.com/p/x-tier/ X-TIER
```

2. **Change to the X-TIER directory and switch to the master branch**
```
 ~/X-TIER$ cd X-TIER
 ~/X-TIER$ git checkout master
```

3. **Configure KVM and QEMU**
> a) Using the configure script that comes with X-TIER (recommended)
```
 ~/X-TIER$ ./configure.sh
```
> b) Manually
> > KVM:
```
 ~/X-TIER$ cd kvm
 ~/X-TIER/kvm$ ./configure.sh [<options>]
```
> > QEMU:
```
 ~/X-TIER$ cd qemu
 ~/X-TIER/qemu$ ./configure --target-list=x86_64-softmmu --enable-kvm [<options>]
```

4. **Configure the X-TIER start script** (optional, but recommended)

> X-TIER provides a start script that will build KVM and QEMU, load the X-TIER enabled KVM modules, and launch a virtual machine of your choice. Before you can use this script, you need to configure it to your personal setup:
    1. Open the `startX-TIER.sh` script in an editor of your choice
    1. Change the `DESTINATION_FOLDER` variable, if you want to specify a specific folder that files such as your modules will be copied to before a virtual machine is started. By default all files are copied to `/tmp/`.
    1. Update the `LINUX_CMD` and `WINDOWS_CMD` variables. These variables should contain all options that should be passed to QEMU when the virtual machine is started. Most importantly point the `-hda` option to a virtual hard disc stored on your machine. More information about QEMU and its options can be found [here](http://qemu.weilnetz.de/qemu-doc.html#sec_005finvocation).

5. **Run X-TIER**
> a) Using the `startX-TIER.sh` script
```
 ~/X-TIER$ ./startX-TIER.sh
```
> > Select the virtual machine you want to start, e.g. Linux:
```
 Which VM would you like to start?
  -> Linux: 1
  -> Windows: 2

 1
```
> > Enter your password when the script asks for it to be able to load the modules
```
 Building KVM and QEMU...
         [01/04] Removing injection file for building KVM...                 [ FAIL ]
                Error not fatal. Continuing.
         [02/04] Building KVM...                                             [ PASS ]
         [03/04] Removing injection file for building QEMU...                [ PASS ]
         [04/04] Building QEMU...                                            [ PASS ]
 DONE!

 Getting higher privileges...
 [sudo] password for vogls:
```
> > If no errors occur the script should finally print the following output:
```
 QEMU waiting for connection on: telnet:127.0.0.1:3333,server
```
> > This means that QEMU is ready to launch your virtual machine. To do so, you have to connect to the QEMU monitor. For this purpose open another shell and use the following telnet command:
```
 ~$ telnet 127.0.0.1 3333
```
> > Your virtual machine should now be running. To Connect to it, either use VNC (the example below requires [krdc](http://www.kde.org/applications/internet/krdc/) to be installed)
```
 ~$ krdc 127.0.0.1:5900
```
> > or ssh, given that your virtual machine runs an SSH server
```
 ~$ ssh -p 12345 <username>@localhost
```


> b) Manually
> > `1.` Build the KVM modules:
```
 ~/X-TIER$ cd kvm
 ~/X-TIER/kvm$ make
```
> > 2. Unload the current KVM modules:
```
 ~/X-TIER/kvm$ sudo rmmod kvm-intel
 ~/X-TIER/kvm$ sudo rmmod kvm
```
> > 3. Load the X-TIER enabled KVM modules:
```
 ~/X-TIER/kvm$ sudo insmod x86/kvm.ko
 ~/X-TIER/kvm$ sudo insmod x86/kvm-intel.ko
```
> > 4. Build QEMU:
```
 ~/X-TIER$ cd qemu
 ~/X-TIER/qemu$ make
```
> > 4. Start QEMU with the desired options
```
 ~/X-TIER/qemu$ cd x86_64-softmmu
 ~/X-TIER/qemu/x86_64-softmmu$ ./qemu-system-x86_64 <options>
```


