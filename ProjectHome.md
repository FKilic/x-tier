# About #
<p align='justify'>
<b>X-TIER</b> enables security applications residing within the hypervisor to <b>inject kernel modules</b>, also referred to as kernel drivers, into a <b>running virtual machine (VM)</b>. An injected module will thereby, similar to a module that was loaded by the operating system (OS), be able to <b>access all exported guest OS data structures and functions</b>. In contrast to a normally loaded module, however, an injected module will be <b>inaccessible to any other code</b> residing within the VM. Even in the case that the injected module invokes a guest OS function, the function will neither be aware of the existence of the module nor be able to access any data of the module besides its function arguments. In fact, if a module constrains itself to only reading state information, its execution leaves no detectable traces within the VM (with the exception of timing attacks). A module may, however, apply selective changes to the state, for example, to remove a rootkit from a compromised system. Consequently, X-TIER provides a <b>secure and elegant way</b> for hypervisor-based security applications <b>to bridge the semantic gap</b>.<br>
</p>

<p>
<a href='http://www.tum.de/'><img src='http://www.sec.in.tum.de/themes/sec/images/tum-logo-pantone.png' align='right' /></a> X-TIER is based on <a href='http://www.linux-kvm.org/'>KVM</a> and <a href='http://www.qemu.org/'>QEMU</a>. It is developed by <a href='http://www.sec.in.tum.de/sebastian-vogl/'>Sebastian Vogl</a> and <a href='http://www.sec.in.tum.de/fatih-kilic/'>Fatih Kilic</a> in the context of their research at the <a href='http://www.sec.in.tum.de/'>Chair for IT Security</a> at the <a href='http://www.tum.de/'>Technische Universität München</a>, Munich, Germany. While Sebastian's research focus lies in the field of virtual machine introspection, Fatih is mainly interested in Reverse Engineering and Binary Exploitation.<br>
</p>

# Features #
  * Support for Linux and Windows guests
  * Support for 32-bit and 64-bit guests
  * Run-time Isolation
  * Protection during external function calls
  * Module arguments
  * Function Call Translation
  * Hypercalls
  * ...