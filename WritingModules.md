# Restrictions #
X-TIER places almost no restrictions on the kernel modules that can be injected with it. The only requirement is that the module does not try to access its own `module` structure within the kernel space. Since the module is loaded by X-TIER and not by the guest operating system, there is no kernel structure that refers to the module. Thus a reference such as `__this_module` cannot be used within a module that should be injected by X-TIER.

# Writing Linux Kernel Modules #
There are a lot of reference that explain the process of writing a Linux Kernel Module in detail, which is why we will not describe the process here. The interested reader may use one of the following references as a starting point:
  * [The Linux Kernel Module Programming Guide](http://www.tldp.org/LDP/lkmpg/2.6/html/)
  * [“Hello World” Loadable Kernel Module](http://blog.markloiseau.com/2012/04/hello-world-loadable-kernel-module-tutorial/)
  * [Infecting loadable kernel modules (Phrack)](http://www.phrack.org/issues.html?issue=68&id=11)

# Writing Windows Drivers #
There are a lot of reference that explain the process of writing a Windows driver  in detail, which is why we will not describe the process here. The interested reader may use one of the following references as a starting point:
  * [Driver Development Part 1: Introduction to Drivers](http://www.codeproject.com/Articles/9504/Driver-Development-Part-1-Introduction-to-Drivers)
  * [Windows Programming/Device Driver Introduction (Wikibooks)](http://en.wikibooks.org/wiki/Windows_Programming/Device_Driver_Introduction)
  * [Getting Started Writing Windows Drivers](http://www.osronline.com/article.cfm?article=20)