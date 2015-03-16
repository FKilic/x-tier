# Overview #
X-TIER is currently implemented for [KVM](http://www.linux-kvm.org/) 3.6 and [QEMU](http://www.qemu.org/) 1.5.0. In the following we will describe the steps that are necessary to port X-TIER to a new KVM or QEMU version.

# KVM #
## Dependencies ##
Most of the X-TIER related files are contained within the `x86/X-TIER/` directory. In addition, X-TIER requires modifications to the following files:
  * `x86/kvm_main.c`
  * `x86/vmx.c`
  * `x86/x86.c`
  * `x86/Kbuild`

## Creating a Patch ##
The easiest way to move to a new KVM version is to create a patch from the current X-TIER version and apply it to the new KVM version. For this purpose the unmodified original KVM source code is contained within the git repository. It can be located using the tag "kvm\_v`<`version\_number`>_`base". To create a patch for the current X-TIER version, one could for instance use the following command:

```
 ~/X-TIER/$ git diff kvm_v3.5_base HEAD -- kvm-kmod-3.5 > X-TIER_kvm.patch
```

To apply the patch, change into the source folder of the new KVM version and use the `git apply` command:

```
 ~/X-TIER/new_kvm/$ patch -p2 < ../X-TIER_kvm.patch
```

## Manually ##
In case a patch cannot be applied, the necessary steps to port X-TIER to a new KVM version are sketched below:

  1. Copy the entire `x86/X-TIER` folder to the `x86` source folder of the new KVM version.
  1. Modify `x86/kvm_main.c` of the new KVM version.
    1. Add the necessary includes at the top of the file. (See the X-TIER `x86/kvm_main.c` version for details)
    1. Add the required IOCTL commands to the `kvm_vcpu_ioctl` function. (See the X-TIER `x86/kvm_main.c` version for details)
  1. Modify `x86/vmx.c` of the new KVM version.
    1. Add the necessary includes at the top of the file. (See the X-TIER `x86/vmx.c` version for details)
    1. Modify the `vmx_fpu_activate` function to call X-TIER (See the X-TIER `x86/vmx.c` version for details)
    1. Modify the `vmx_fpu_deactivate` function to call X-TIER (See the X-TIER `x86/vmx.c` version for details)
    1. Modify the `vmx_inject_irq` function to call X-TIER (See the X-TIER `x86/vmx.c` version for details)
    1. Modify the `handle_exception` function to call X-TIER (See the X-TIER `x86/vmx.c` version for details)
    1. Modify the `handle_interrupt_window` function to call X-TIER (See the X-TIER `x86/vmx.c` version for details)
    1. Modify the `handle_ept_violation` function to call X-TIER (See the X-TIER `x86/vmx.c` version for details)
    1. Modify the `vmx_handle_exit` function to call X-TIER (See the X-TIER `x86/vmx.c` version for details)
    1. Modify the `vmx_vcpu_run` function to call X-TIER (See the X-TIER `x86/vmx.c` version for details)
  1. Modify `x86/x86.c` of the new KVM version.
    1. Add the necessary includes at the top of the file. (See the X-TIER `x86/x86.c` version for details)
    1. Modify the `vcpu_enter_guest` function to call X-TIER (See the X-TIER `x86/x86.c` version for details)
  1. Finally add the the X-TIER files to `x86/Kbuild` to build them with the kernel modules (See the X-TIER `x86/Kbuild` version for details)



# QEMU #
## Dependencies ##
Similarly to the KVM part of X-TIER, most of the X-TIER related files are contained within the `X-TIER` folder. On top of that, the following files must be updated:
  * `exec.c`
  * `include/exec/cpu-all.h`
  * `monitor.c`
  * `include/monitor/monitor.h`
  * `target-i386/kvm.c`
  * `hmp-commands.hx`
  * `Makefile.target`

## Creating a Patch ##
As in the case of KVM it is possible to port X-TIER to a new QEMU version by creating a patch. For this purpose the unmodified original QEMU source code is contained within the git repository. It can be located using the tag "qemu\_v`<`version\_number`>_`base". To create a patch for the current X-TIER version, one could for instance use the following command:

```
 ~/X-TIER/$ git diff qemu_v1.5.0_base HEAD -- qemu-1.5.0 > X-TIER_qemu.patch
```

To apply the patch, change into the source folder of the new KVM version and use the `git apply` command:

```
 ~/X-TIER/new_qemu/$ patch -p2 < ../X-TIER_qemu.patch
```

## Manually ##
In case a patch cannot be applied, the necessary steps to port X-TIER to a new QEMU version are sketched below:

  1. Copy the entire `X-TIER` folder to the source folder of the new QEMU version.
  1. Add the `gva_to_hva` function to `exec.c` of the new QEMU version. (See the X-TIER `exec.c` version for details)
  1. Add the prototype of the `gva_to_hva` function to `include/exec/cpu-all.h` of the new QEMU version. (See the X-TIER `include/exec/cpu-all.h` version for details)
  1. Modify `monitor.c` of the new QEMU version.
    1. Add the necessary includes at the top of the file. (See the X-TIER `monitor.c` version for details)
    1. Add the `XTIER_shell`, `XTIER_start_getting_user_input`, `XTIER_stop_getting_user_input`, and `XTIER_synchronize_state` function (See the X-TIER `monitor.c` version for details)
  1. Modify `include/monitor/monitor.h` of the new QEMU version.
    1. Add the necessary includes at the top of the file. (See the X-TIER `include/monitor/monitor.h` version for details)
    1. Add the prototypes for the `XTIER_start_getting_user_input`, the `XTIER_stop_getting_user_input`, and the `TIER_synchronize_state` function. (See the X-TIER `include/monitor/monitor.h` version for details)
  1. Modify ```target-i386/kvm.c``` of the new QEMU version.
    1. Add the necessary includes at the top of the file. (See the X-TIER `target-i386/kvm.c` version for details)
    1. Add the necessary VM Exit handlers to the `kvm_arch_handle_exit` function. (See the X-TIER `target-i386/kvm.c` version for details)
  1. Add the `x-tier` command to `hmp-commands.hx` of the new QEMU version. (See the X-TIER `hmp-commands.hx` version for details)
  1. Finally add the the X-TIER files to `Makefile.target` to build them with the kernel modules (See the X-TIER `Makefile.target` version for details)