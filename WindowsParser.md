# Overview #
The X-TIER parser for Windows drivers is capable of transforming 32-bit Windows driver files (**.sys) to the [X-Format](XFormat.md).**

# Prerequisites #
In order to use the X-TIER parser for Windows, we first have to obtain the relative offsets of the individual external functions that the Windows kernel provides:
  1. Start the Windows virtual machine that you want to inject modules in.
  1. Save the `C:\System32\ntoskrnl.exe` file of the Windows virtual machine to a directory of your choice.
  1. Locate the Windows kernel binary in memory with [LiveKd](http://technet.microsoft.com/de-de/sysinternals/bb897415.aspx)
    1. Start LiveKd
    1. Type "!pcr" to obtain the KPCR
    1. display the KPCR with "dt _KPCR <address from step 1)>"
    1. Display KdVersionBlock with "dt_DBGKD\_GET\_VERSION64 <address from step 2)>"
  1. Save the Windows kernel image with the QEMU monitor
```
(qemu) memsave <address of kernel base> 0x10000000 "/tmp/ntos.bin"
```
  1. Use the `offsetExtractor.py` (`X-TIER/parser/win/`) that comes with X-TIER to extract the relative addresses of the external functions. To do this, the `OffsetExtractor` must be placed into the same directory as `ņtoskrnl.exe` and `ntos.bin`, before it is executed. Alternatively, you also can change the `offsetExtractor.py` script.

# Usage #
In the following the arguments that can be provided to the parser are listed and explained.

```

peparser.py <driver>

         <driver>        The complete path to the Windows driver (*.sys) that should be parsed.
```

# Example #
```
$ ./peparser.py modules.sys
```

# Output #
```
         PEparser
                 |_ Version: 0.2


 [+] Preparing 'modules.sys' for injection...
 [+] Parsing file 'modules.sys'...
 [-] Parsing Relocation Entries...
         [+] Relocatiion @ 0x444 (RVA: 0x1044) will be set to 0x800 (RVA: 0x3000)
         [+] Relocatiion @ 0x481 (RVA: 0x1081) will be set to 0x574 (RVA: 0x1174)
         [+] Relocatiion @ 0x48d (RVA: 0x108d) will be set to 0x560 (RVA: 0x1160)
         [+] Relocatiion @ 0x499 (RVA: 0x1099) will be set to 0x54c (RVA: 0x114c)
         [+] Relocatiion @ 0x4b2 (RVA: 0x10b2) will be set to 0x600 (RVA: 0x2000)
         [+] Relocatiion @ 0x4c1 (RVA: 0x10c1) will be set to 0x53e (RVA: 0x113e)
         [+] Relocatiion @ 0x4fa (RVA: 0x10fa) will be set to 0x800 (RVA: 0x3000)
         [+] Relocatiion @ 0x51a (RVA: 0x111a) will be set to 0x804 (RVA: 0x3004)
         [+] Relocatiion @ 0x520 (RVA: 0x1120) will be set to 0x800 (RVA: 0x3000)
         [+] Relocatiion @ 0x52e (RVA: 0x112e) will be set to 0x608 (RVA: 0x2008)
         [+] Relocatiion @ 0x53a (RVA: 0x113a) will be set to 0x60c (RVA: 0x200c)
         [+] Relocatiion @ 0xa06 (RVA: 0x4006) will be set to 0x800 (RVA: 0x3000)
         [+] Relocatiion @ 0xa18 (RVA: 0x4018) will be set to 0x604 (RVA: 0x2004)
         [+] Relocatiion @ 0xa1f (RVA: 0x401f) will be set to 0x800 (RVA: 0x3000)
         [+] Relocatiion @ 0xa24 (RVA: 0x4024) will be set to 0x800 (RVA: 0x3000)
         [+] Relocatiion @ 0xa2d (RVA: 0x402d) will be set to 0x800 (RVA: 0x3000)
         [+] Relocatiion @ 0xa34 (RVA: 0x4034) will be set to 0x804 (RVA: 0x3004)
         [#] Parsing of 17 relocation record(s) completed!
 [-] Looking for imported symbols...
         [-] Processing imports from 'ntoskrnl.exe'...
                 [+] Resolving RtlUnicodeStringToAnsiString (0x600)
                 [W] Appending Wrapper '../wrapper/win7-32/RtlUnicodeStringToAnsiString/RtlUnicodeStringToAnsiString' with entry point in wrapper section @ 0x224!
                         [!] Adding Wrapper relocation: 0x106e will be set to 0x1024!
                         [!] Adding Wrapper relocation: 0x1087 will be set to 0x1024!
                         [!] Adding Wrapper relocation: 0x10b6 will be set to 0x1028!
                         [!] Adding Wrapper relocation: 0x10bb will be set to 0x1024!
                         [!] Adding ESP Patch Symbol 0x1024
                 [!] Relocating 'RtlUnicodeStringToAnsiString' @ 0x600 to Wrapper @ 0x102c!
                 [!] Setting Import 'RtlUnicodeStringToAnsiString' to target_address within wrapper @ 0x1028!
                 [+] Resolving KeTickCount (0x604)
                 [+] Resolving KeTickCount (0x604)
                 [+] Resolving KeBugCheckEx (0x608)
                 [+] Resolving KeBugCheckEx (0x608)
                 [+] Resolving DbgPrint (0x60c)
                 [!] Adding 'DbgPrint' to relocations @ 0x60c to 0xe00!
         [#] Parsing of 3 import(s) completed!
 [-] Output will be written to 'modules.inject'...
         [+] Writing Shellcode... (Size: 371, Data Offset: 232, Total Offset: 0x25b)
         [+] Searching for Entry Point...  Entry Point @ 0x11038 (Offset: 0x38)
         [+] Writing Entry Point (0x520) ...
         [-] Writing Relocations...
                 [+] Writing Relocation Count... (23)
                 [+] Setting Relocation @ 0x52c to 0x8e8
                 [+] Setting Relocation @ 0x569 to 0x65c
                 [+] Setting Relocation @ 0x575 to 0x648
                 [+] Setting Relocation @ 0x581 to 0x634
                 [+] Setting Relocation @ 0x59a to 0x6e8
                 [+] Setting Relocation @ 0x5a9 to 0x626
                 [+] Setting Relocation @ 0x5e2 to 0x8e8
                 [+] Setting Relocation @ 0x602 to 0x8ec
                 [+] Setting Relocation @ 0x608 to 0x8e8
                 [+] Setting Relocation @ 0x616 to 0x6f0
                 [+] Setting Relocation @ 0x622 to 0x6f4
                 [+] Setting Relocation @ 0xaee to 0x8e8
                 [+] Setting Relocation @ 0xb00 to 0x6ec
                 [+] Setting Relocation @ 0xb07 to 0x8e8
                 [+] Setting Relocation @ 0xb0c to 0x8e8
                 [+] Setting Relocation @ 0xb15 to 0x8e8
                 [+] Setting Relocation @ 0xb1c to 0x8ec
                 [+] Setting Relocation @ 0x1156 to 0x110c
                 [+] Setting Relocation @ 0x116f to 0x110c
                 [+] Setting Relocation @ 0x119e to 0x1110
                 [+] Setting Relocation @ 0x11a3 to 0x110c
                 [+] Setting Relocation @ 0x6e8 to 0x1114
                 [+] Setting Relocation @ 0x6f4 to 0xee8
         [-] Writing ESP Patch Symbols...
                 [+] Writing ESP Patch Symbol Count... (1)
                 [+] Setting ESP Patch Symbol to 0x1024
         [-] Writing Imports...
                 [+] Writing Start Address (0x82700c00) for KPCR search...
                 [+] Writing Import Count... (3)
                 [+] Reading Relative offsets from './relative-offsets.data'...
                 [+] Setting Import 'RtlUnicodeStringToAnsiString' @ 0x1110 to 0x267d6b
                 [+] Setting Import 'KeTickCount' @ 0x6ec to 0x12f580
                 [+] Setting Import 'KeBugCheckEx' @ 0x6f0 to 0xdef02
         [+] Writing PE...
         [+] Writing Function Stubs...
         [+] Writing Wrapper...
```