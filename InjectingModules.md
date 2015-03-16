# Overview #
This page provides information about the steps that are necessary to inject a module into a virtual machine with the help of X-TIER. In the following, we will assume that X-TIER is already [running](Running.md) and that we have a [compiled](WritingModules.md) version of the kernel module that we want to inject. We will first describe the injection process for [Linux](InjectingModules#Linux_Guests.md) guests, before we will discuss the injection process for [Windows](InjectingModules#Windows_Guests.md) guests.

# Linux Guests #
  1. Create an "injectable" version of your module. To achieve this you have to parse the kernel module that you want to inject with the [Linux parser](LinuxParser.md) of X-TIER.
> 2. Open the X-TIER shell by typing the following command into the QEMU montior
```
(qemu) x-tier
```
> You should see the following output:
```
        Switching to 'XTIER' Mode...
        The VM will be stopped...
(X-TIER >>
```
> 3. Type "inject"
```
(X-TIER >> inject
```
> 4. Set the guest OS to Linux 64-bit
```
So far no OS has been specified. Please do so now.

Please specify the guest OS.
Type 'help' to see the available choices.
(X-TIER >> 2
```
> 5. Type the name of the module you want to inject e.g. the `files` [module](Modules.md) (assuming you used the [X-TIER start script](Running.md))
```
(X-TIER >> /tmp/win/files.inject
```
> 6. Resume execution
```
(X-TIER >> cont
```
> You should now see the output of the module on the shell window that you started the virtual machine on and the performance of the module in the QEMU monitor. For more details see the [module section](Modules.md).

# Windows Guests #
  1. Create an "injectable" version of your module. To achieve this you have to parse the kernel module that you want to inject with the [Windows parser](WindowsParser.md) of X-TIER.
> 2. Open the X-TIER shell by typing the following command into the QEMU montior
```
(qemu) x-tier
```
> You should see the following output:
```
        Switching to 'XTIER' Mode...
        The VM will be stopped...
(X-TIER >>
```
> 3. Type "inject"
```
(X-TIER >> inject
```
> 4. Set the guest OS to Windows 32-bit
```
So far no OS has been specified. Please do so now.

Please specify the guest OS.
Type 'help' to see the available choices.
(X-TIER >> 2
```
> 5. Type the name of the module you want to inject e.g. the `files` [module](Modules.md) (assuming you used the [X-TIER start script](Running.md))
```
(X-TIER >> /tmp/win/files.inject
```
> 6. Resume execution
```
(X-TIER >> cont
```
> You should now see the output of the module on the shell window that you started the virtual machine on and the performance of the module in the QEMU monitor. For more details see the [module section](Modules.md).