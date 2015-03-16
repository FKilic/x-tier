# Overview #
The X-TIER parser for Linux kernel modules is capable of transforming 64-bit Linux kernel modules to the [X-Format](XFormat.md).

# Usage #
In the following the arguments that can be provided to the parser are listed and explained.

```

X-TIER_linux_parser [<options>] <kernelmodule>

         <kernelmodule>        The complete path to the Linux kernel module (*.ko) that should be parsed.

         Options:
                 -i, --init-function    The name of the function that should be executed when the module is loaded.
                                        DEFAULT = 'module_init'
                 -w, --wrapper-file     The path to the text file that contains all functions that are substituted by a wrapper.
                                        DEFAULT = './wrapper.txt'
                 -w, --wrapper-path     The path to the directory that contains the wrapper functions.
                                        DEFAULT = '../wrapper/linux64/#
                 -e, --extension        The extension of the transformed module. It's name will be equal to the module name.
                                        DEFAULT = '.inject'
```

# Example #
```
$ ./X-TIER_linux_parser files.ko
```

# Output #
```
         X-TIER Linux Kernel Module Parser
                 |_ Processing File:           'files.ko'
                 |_ Wrappers are specified in: 'wrapper.txt'
                 |_ Wrappers are located at:   '../wrapper/linux64/'
                 |_ Resulting file will be:    'files.ko.inject'

Checking for ELF Executable Object... OK! Seems to be executable
Checking Class... OK! 64-Bit binary found
Obtaining necessary sections...
         -> Found RELA section '.rela.text' containing 7 entries
         -> Found RELA section '.rela.init.text' containing 8 entries
         -> Found RELA section '.rela.exit.text' containing 1 entry
         -> Found RELA section '.rela__mcount_loc' containing 1 entry
         -> Found RELA section '.rela.gnu.linkonce.this_module' containing 2 entries
         -> Found '.symtab' section cotaining 32 symbols
Parsing 5 RELA sections... 1 2 3 4 5 OK!
Looking for entry point...
         -> Entry Point @ 0x1b0
Generating Injection File...
         -> Parsing wrapper names from 'wrapper.txt'... Found 10 wrappers!
         -> Outfile will be ../files.inject
         -> Calculating Offsets...
                 # COMPLETE Offset is 0x149
         -> Creating File...OK!
         -> Writing Shellcode (365 bytes)... OK!
         -> Calculating NEW Entry Point...OK! Entry Point @ 0x2f9
         -> Writing Entry Point... OK!
         -> Generating WRAPPER file 'files.inject.wrapper' for 1 external function(s)...
                 # Trying to find a WRAPPER for 'd_path'...
                         <> Found Wrapper '../wrapper/linux64/d_path/d_path'...
                         <> Reading Wrapper...
                         <> Writing Wrapper...
                         <> Found Kernel ESP Offset @ 0x110...
                         <> Kernel Stack address will be written to 0x163a...
                         <> 'd_path' @ 0x1db will be set to 0x164a...
         -> Writing Patch Symbols (14)... 
                 # FUNCTION PATCH printk @ 0x1e5 (0x2c) will be set to 0x151c
                 # PATCH 0x270 (0xb7) will be set to '.rodata.str1.1' (0x3ab)
                 # PATCH 0x2c6 (0x10d) will be set to '.rodata.str1.1 + 0x14' (0x3bf)
                 # PATCH 0x2e1 (0x128) will be set to '.rodata.str1.1 + 0x1d' (0x3c8)
                 # FUNCTION PATCH printk @ 0x2ed (0x134) will be set to 0x151c
                 # PATCH 0x2fc (0x3) will be set to '.rodata.str1.1 + 0x1f' (0x3ca)
                 # PATCH 0x30d (0x14) will be set to '.text' (0x1b9)
                 # FUNCTION PATCH printk @ 0x31a (0x21) will be set to 0x151c
                 # PATCH 0x32a (0x31) will be set to '.rodata.str1.8' (0x3e9)
                 # PATCH 0x338 (0x3f) will be set to '.rodata.str1.8 + 0x28' (0x411)
                 # PATCH 0x360 (0x67) will be set to '.rodata.str1.1 + 0x2d' (0x3d8)
                 # FUNCTION PATCH printk @ 0x36c (0x73) will be set to 0x151c
                 # PATCH 0x4a9 (0x0) will be set to '.text + 0x15' (0x1ce)
                 # Setting Function @ 0x1db to call Wrapper @ 0x164a...
         -> Writing ESP Patch Symbols (1)... 
                 # Kernel ESP will be written to 0x163a...
         -> Writing Symmap begin... OK!
         -> Writing Symmap end... OK!
         -> Writing Resolve Symbols (2)... 
                 # RESOLVE patching mcount @ 0x85...
                 # RESOLVE d_path @ 0x1642 must be resolved...
                 # RESOLVE init_task @ 0x346 must be resolved...
                 # RESOLVE patching mcount @ 0x25c...
         -> Writing Binary... OK!
         -> Writing 'printk' Wrapper... OK!
         -> Writing Remaining Wrapper Section... OK!


DONE.
```