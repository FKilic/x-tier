#!/usr/bin/python

import pefile
import struct

# Extract the export table for a given ntoskrnl.exe in memory.

# You have to change this settings to generate a file
# The file where the export table is loaded from
# Simply copy C:\System32\ntoskrnl.exe
NTOS ="ntoskrnl.exe"

# The binary in MEMORY. It will be used to extract the correct offsets.
# To locate the binary on Windows / using LiveKD execute the following steps:
# 1) Type "!pcr" to obtain the KPCR
# 2) Display the KPCR with "dt _KPCR <address from step 1)>
# 3) Display KdVersionBlock with "dt _DBGKD_GET_VERSION64 <address from step 2)>
# 4) Save it with QEMU monitor 'memsave 0x82611000 0x10000000 "/tmp/ntos.bin"'
NTOSMEM ="ntos.bin"

# Name of the output File
OUT = "relative-offsets.data"



# DO IT
print "\n\n-> Kernel Image Offset Extractor\n\n"

def is64bit(pe):
	if pe.FILE_HEADER.Machine == 0x8664:
		return True
	else:
		return False


# Get the offset of the export directory from the memory dump of the running kernel image
print "\t [+] Searching for address of the export table within the kernel memory dump '%s'..." % (NTOSMEM)
pe = pefile.PE(NTOSMEM)
export_table_address = pe.OPTIONAL_HEADER.DATA_DIRECTORY[0].VirtualAddress
print "\t\t -> Kernel Image Export Directory @ 0x%x" % (export_table_address)
pe.close()

# open mem file
memfile = open(NTOSMEM, 'rb')

# Seek to the export table which is references by the export directory
memfile.seek(export_table_address + 28)

# Get the address of the export table
if is64bit(pe):	
	export_table_address = struct.unpack("<Q", memfile.read(8))[0]
else:
	export_table_address = struct.unpack("<I", memfile.read(4))[0]
	
# Seek to the export table
memfile.seek(export_table_address)

print "\t\t -> Kernel Image Export Table @ 0x%x" % (export_table_address)

# open out file
print "\t [+] Creating file '%s'..." % (OUT)
outfile = open(OUT, 'w')

# Parse exports with names form kernel image
print "\t [+] Getting Exported Function Names from '%s'..." % (NTOS)
pe = pefile.PE(NTOS)

# Parse Exports
exports = {}
for exp in pe.DIRECTORY_ENTRY_EXPORT.symbols:
  	exports[exp.ordinal] = exp.name

# Sort by key
sorted_exports = exports.keys()
sorted_exports.sort()

# Write data
print "\t [+] Writing data..."
counter = 0x353000
for k in sorted_exports:
	outfile.write("% 4d\t" % (k))

	if is64bit(pe):
		data = memfile.read(8)
		
		if len(data) < 8:
			print "WARNING: read less than 8 byte!\n"
		
		outfile.write("0x%016x" % (struct.unpack("<Q", data)[0]))
        else:
		data = memfile.read(4)
		
		if len(data) < 4:
			print "WARNING: read less than 4 byte!\n"
	  
		outfile.write("0x%08x" % (struct.unpack("<I", data)[0]))
	
	
	outfile.write("\t%s\n" % (exports[k]))
	counter += 4


