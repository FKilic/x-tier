#!/usr/bin/python

# PEFILE
# ------
# 	-> URL: http://code.google.com/p/pefile/
#
#	-> More info: help(pefile)
#
# 	-> Important functions:
# 		pefile.get_dword_at_rva(<rva>) - Get a DWORD at a given RVA
# 		pefile.get_physical_by_rva(<rva>) - Get the physical address of a RVA
#

import pefile
import sys
import os
import traceback
import struct
import subprocess

# SETUP
VERSION = "0.2"
DEBUG = 1
FILTER = 1
EXTENSION = ".inject"
WRAPPER_EXTENSION = ".wrapper"
WRAPPER_PATH = "../wrapper/win7-32/"
SYMBOL_OFFSETS_NTOS = "./relative-offsets.data"
KPCR_START = 0x82700c00
PRINTF = "\xb8\x02\x00\x00\x00\xcd\x2a\xc3" 

SHELLCODE = (
	      "\xe9\x69\x01\x00\x00\x52\x57\x53\x51\x8b\x55\x04\x89\xef\x83\xc7"
	      "\x08\x85\xd2\x74\x0e\x4a\x89\xe8\x03\x07\x89\xeb\x03\x5f\x04\x89"
	      "\x18\xeb\xeb\x89\xf8\x59\x5b\x5f\x5a\xc3\x58\x5a\x59\x50\x55\x01"
	      "\xd5\x8b\x75\x00\x89\xef\x83\xc7\x04\x85\xf6\x74\x0b\x4e\x89\xe8"
	      "\x29\xd0\x03\x07\x89\x08\xeb\xee\x89\xf8\x5d\xc3\x83\xc4\x04\x83"
	      "\xc4\x08\x83\xc4\x1c\x50\x53\x51\x52\x57\x56\x55\x83\xec\x08\x83"
	      "\xec\x04\xc3\x83\xc4\x04\x83\xc4\x04\x5d\x5e\x5f\x5a\x59\x5b\x58"
	      "\x83\xec\x1c\x83\xec\x04\x83\xec\x04\xc3\x58\x59\x50\x52\x53\x89"
	      "\xcb\x83\xc1\x1c\x8b\x11\x39\xd3\x74\x07\xb8\x01\x00\x00\x00\xeb"
	      "\x05\xb8\x00\x00\x00\x00\x5b\x5a\xc3\x58\x5f\x50\x51\x52\x53\xbb"
	      "\x00\x00\x00\x00\xba\x00\x00\x00\x00\xeb\x14\x89\xd0\xc1\xe0\x0c"
	      "\x01\xf8\x89\xc3\x53\xe8\xc0\xff\xff\xff\x85\xc0\x74\x10\x42\x81"
	      "\xfa\x8f\x00\x00\x00\x7e\xe4\xb8\x00\x00\x00\x00\xeb\x02\x89\xd8"
	      "\x5b\x5a\x59\xc3\xb8\xef\xbe\xad\xde\xcd\x50\x58\x5f\x50\x51\x83"
	      "\xc7\x34\x8b\x0f\x83\xc1\x10\x8b\x01\x59\xc3\x58\x5e\x50\x53\x51"
	      "\x52\x57\x55\x01\xf5\x8b\x7d\x00\x57\xe8\x9b\xff\xff\xff\x85\xc0"
	      "\x74\xd2\x89\xc7\x57\xe8\xd1\xff\xff\xff\x89\xc1\x90\x8b\x55\x04"
	      "\x89\xef\x83\xc7\x08\x85\xd2\x74\x12\x4a\x89\xe8\x29\xf0\x03\x07"
	      "\x89\xcb\x03\x5f\x04\x89\x18\x90\x90\xeb\xe7\x89\xf8\x5d\x5f\x5a"
	      "\x59\x5b\xc3\xe8\x14\xff\xff\xff\x5d\x5e\x8b\x4d\x00\xe8\xc3\xfe"
	      "\xff\xff\x89\xc3\x29\xeb\x51\x56\x53\xe8\xdc\xfe\xff\xff\x89\xc3"
	      "\x29\xeb\x59\x53\xe8\x92\xff\xff\xff\x90\x89\xc8\x01\xe8\x50\xe8"
	      "\xff\xfe\xff\xff\x58\x81\xc4\x20\x03\x00\x00\xff\xd0\xf4\xe8\xc0"
	      "\xff\xff\xff"
	    )


# CLASSES
class Relocations:
	data = []

	def __init__(self):
		self.data = []

	def add(self, fileOffset, rva):
		self.data.append((fileOffset, rva))
	
	def __iter__(self):
		return self.data.__iter__()

	def size(self):
		return len(self.data)
		
class Esp_Patch_Symbol:
	data = []

	def __init__(self):
		self.data = []

	def add(self, addr):
		self.data.append(addr)

	def __iter__(self):
		return self.data.__iter__()

	def size(self):
		return len(self.data)

class Imports:
	data = []

	def __init__(self):
		self.data = []

	def add(self, name, addr):
		self.data.append((name, addr))

	def __iter__(self):
		return self.data.__iter__()

	def size(self):
		return len(self.data)
	
class ImportRelative:
	address = 0
	name = ""
	number = 0

	def __init__(self, data):
		sep = data.split("\t")
		self.number = int(sep[0].strip())
		self.address = int(sep[1].strip(), 16)
		self.name = sep[2].strip(" \n")

	def getName(self):
		return self.name

	def getAddress(self):
		return self.address

	def getNumber(self):
		return self.number

	def setAddress(self, value):
		self.address = value
		

# FUNC
def printBanner():
	banner = (
			"\n\t PEparser\n"
			"\t\t |_ Version: " + VERSION + "\n\n"
		 )

	print banner


def handleError(err):
	print "\n [!] ERROR: \n " + str(err) + "\n"
	sys.exit(-1)

def getImageBase(pe):
	return pe.OPTIONAL_HEADER.ImageBase
	
def is64bit(pe):
	if pe.FILE_HEADER.Machine == 0x8664:
		return True
	else:
		return False

def getTextOffset(pe):
	return pe.get_offset_from_rva(getTextOffsetVA(pe))
		

def getTextOffsetVA(pe):
	for section in pe.sections:
		if section.Name.rstrip('\x00').lower() == ".text":
			return section.VirtualAddress
		
	return 0

def printInfo(msg):
	print msg,

def printDebug(msg):
	if DEBUG:
		print msg,
		
def addWrapper(path, wrapper_name, relocs, esp_patch_symbols):
	statinfo = os.stat(path)
	all_wrapper = os.path.splitext(path)[0] + WRAPPER_EXTENSION
	
	# Get file size
	all_wrapper_size = 0
	all_wrapper_file = 0
	try:
		all_wrapper_size = os.stat(all_wrapper).st_size
		all_wrapper_file = open(all_wrapper, "ab")
	except Exception:
		all_wrapper_size = 0
		all_wrapper_file = open(all_wrapper, "wb")
		
	# Open new wrapper
	wrapper = WRAPPER_PATH + wrapper_name + "/" + wrapper_name
	wrapper_file = open(wrapper, "rb")
	
	# Find entry point
	entryPoint = subprocess.check_output("objdump -h " + wrapper + " | grep .text | awk '{print $6}'", shell = True)
        entryPoint = int(entryPoint, 16) + 8 # Increase entry point by size of kernel_stack and size of target_address
        entryPoint += all_wrapper_size
	
	# Append wrapper
	printInfo("\t\t [W] Appending Wrapper '%s' with entry point in wrapper section @ 0x%x!\n" % (wrapper, entryPoint))
	buf_size = 1024
	buf = wrapper_file.read(buf_size)	

	while buf != "":
		all_wrapper_file.write(buf)
		buf = wrapper_file.read(buf_size)
	
	# Patch the wrapper offsets
	# All offsets within the wrapper are absolute which should make the patching easy
	lines = subprocess.check_output("objdump -R " + wrapper + " | grep -e '\([0-9a-f]\{8,8\}\)' | awk '{print $1}'", shell = True)
	lines = lines.splitlines()
	
	for line in lines:
	    # Get the current address and value
	    current_address = long(line, 16)
	    wrapper_file.seek(current_address)
	    current_value = struct.unpack("<L", wrapper_file.read(4))[0]
	    
	    # Add relocation
	    printInfo("\t\t\t [!] Adding Wrapper relocation: 0x%x will be set to 0x%x!\n" % (all_wrapper_size + current_address + statinfo.st_size + len(PRINTF), 
											    current_value + all_wrapper_size + statinfo.st_size + len(PRINTF)))
		
	    relocs.add(all_wrapper_size + current_address + statinfo.st_size + len(PRINTF), current_value + all_wrapper_size + statinfo.st_size + len(PRINTF))
	
	# Write Esp_Patch_Symbol
	printInfo("\t\t\t [!] Adding ESP Patch Symbol 0x%x\n" % (entryPoint - 8 + all_wrapper_size + statinfo.st_size + len(PRINTF)))
	esp_patch_symbols.add(entryPoint - 8 + all_wrapper_size + statinfo.st_size + len(PRINTF))
	
	# Cleanup
	all_wrapper_file.close()
	wrapper_file.close()
	
	# Return entry point
	return entryPoint
	

# Add wrappers here
def filterFunctionStubs(pe, imp, imgbase, path, relocs, esp_patch_symbols, imps):
	
	if FILTER == 0:
		return False

	# filter print
	if imp.name == "DbgPrint":
		statinfo = os.stat(path)
		printInfo("\t\t [!] Adding '%s' to relocations @ 0x%x to 0x%x!\n" % (imp.name, pe.get_offset_from_rva(imp.address - imgbase), statinfo.st_size))
		
		relocs.add(pe.get_offset_from_rva(imp.address - imgbase), statinfo.st_size)

		return True
	elif imp.name == "RtlUnicodeStringToAnsiString":
		# Append wrapper
		wrapper_entry = addWrapper(path, "RtlUnicodeStringToAnsiString", relocs, esp_patch_symbols)
	  
		statinfo = os.stat(path)
		
		# Write relocation for entry point
		printInfo("\t\t [!] Relocating '%s' @ 0x%x to Wrapper @ 0x%x!\n" % (imp.name, pe.get_offset_from_rva(imp.address - imgbase), wrapper_entry + statinfo.st_size + len(PRINTF)))
		
		relocs.add(pe.get_offset_from_rva(imp.address - imgbase), wrapper_entry + statinfo.st_size + len(PRINTF))

		# Write Import for target address
		printInfo("\t\t [!] Setting Import '%s' to target_address within wrapper @ 0x%x!\n" % (imp.name, wrapper_entry - 4 + statinfo.st_size + len(PRINTF)))
		imps.add(imp.name, wrapper_entry - 4 + statinfo.st_size + len(PRINTF))
		
		return True

	return False
		
# Get the given Value in the correct Machien format. 
# On 64-bit machines this function will return 8 bytes, while it will only retunr 4 bytes on 32-bit machines.
def getValueRva(pe, rva):
	if is64bit(pe):
		return struct.unpack("<L", pe.get_data(rva, 8))[0]
	else:
		return struct.unpack("<L", pe.get_data(rva, 4))[0]

# Parse the imports
def parseImports(pe, imgbase, path, relocs, esp_patch_symbols):
	printInfo(" [-] Looking for imported symbols...\n")

	imps = Imports()

	for entry in pe.DIRECTORY_ENTRY_IMPORT:
		printInfo("\t [-] Processing imports from '" + entry.dll + "'...\n")
  		for imp in entry.imports:
    			printDebug('\t\t [+] Resolving %s (0x%x)\n' % (imp.name, pe.get_offset_from_rva(imp.address - imgbase)))

			if filterFunctionStubs(pe, imp, imgbase, path, relocs, esp_patch_symbols, imps):
				pass
			else:
				printDebug('\t\t [+] Resolving %s (0x%x)\n' % (imp.name, pe.get_offset_from_rva(imp.address - imgbase)))
				imps.add(imp.name, pe.get_offset_from_rva(imp.address - imgbase))

	printInfo("\t [#] Parsing of %d import(s) completed!\n" % (imps.size()))

	return imps

# Parse the relocations
def parseRelocs(pe, imgbase):
	printInfo(" [-] Parsing Relocation Entries...\n")
	
	relocs = Relocations()

	for relocdir in pe.DIRECTORY_ENTRY_BASERELOC:
		for reloc in relocdir.entries:
			if reloc.type != 0:
				rva = reloc.rva
				current_rva = getValueRva(pe, rva) - imgbase
				
				printDebug("\t [+] Relocatiion @ 0x%x (RVA: 0x%x) will be set to 0x%x (RVA: 0x%x)\n" % (pe.get_offset_from_rva(rva), rva, pe.get_offset_from_rva(current_rva), current_rva))

				relocs.add(pe.get_offset_from_rva(rva), pe.get_offset_from_rva(current_rva))

	printInfo("\t [#] Parsing of %d relocation record(s) completed!\n" % (relocs.size()))

	return relocs
	

def parse(path):
	printInfo(" [+] Preparing '" + path + "' for injection...\n")

	
	# Get the File
	printInfo(" [+] Parsing file '" + path + "'...\n")
	pe = pefile.PE(path)
	
	# Remove existing wrapper file if any
	try:
	      subprocess.check_output('rm ' + os.path.splitext(path)[0] + WRAPPER_EXTENSION, shell = True)
	except:
	      pass

	# Get the Image Base
	imgbase = getImageBase(pe)
	
	# Esp Patch Symbols
	esp_patch_symbols = Esp_Patch_Symbol()

	# Parse the Relocs
	relocs = parseRelocs(pe, imgbase)	

	# Parse the Imports
	imports = parseImports(pe, imgbase, path, relocs, esp_patch_symbols)

	# Open in file
	infile = open(path, 'rb')
	
	# Calculate data offset that has to be added to all addresses
	data_offset = 0
	data_offset += 4 		   		# Entry Point
	data_offset += 4				# Reloc Count
	data_offset += (2 * 4 * relocs.size()) 		# Relocations
	data_offset += 4				# ESP Patch Count
	data_offset += (4 * esp_patch_symbols.size())	# ESP Patch Symbols
	data_offset += 4				# KPCR Address
	data_offset += 4				# Import Count
	data_offset += (2 * 4 * imports.size())		# Imports

	# Create new Filename
	outpath = os.path.splitext(path)[0] + EXTENSION
	printInfo(" [-] Output will be written to '" + outpath + "'...\n")

	# Open file
	outfile = open(outpath, 'wb')

	# Write Shellcode
	printInfo("\t [+] Writing Shellcode... (Size: %d, Data Offset: %d, Total Offset: 0x%x)\n" % (len(SHELLCODE), data_offset, len(SHELLCODE) + data_offset))
	outfile.write(SHELLCODE)

	# Write Entry Point
	#printInfo("\t [+] Writing Entry Point (0x%x) ...\n" % (pe.get_offset_from_rva(pe.OPTIONAL_HEADER.AddressOfEntryPoint)))
	#if is64bit(pe):
	#	outfile.write(struct.pack("Q", pe.get_offset_from_rva(pe.OPTIONAL_HEADER.AddressOfEntryPoint)))
	#else:
	#	outfile.write(struct.pack("I", pe.get_offset_from_rva(pe.OPTIONAL_HEADER.AddressOfEntryPoint)))

	# We have to calculate the entry point!
	# The compiler adds a function stub on Windows that is the entry point of the module. This entry point is different
        # from the 'DriverEntry' function in the source code. However this is the function that we want. We cannot use .INIT 
        # as entry point because it should lie at a different virtual address than the rest of the text segment so the call 
	# to 'DriverEntry' will fail. 

	printInfo("\t [+] Searching for Entry Point...")

	# The address of the real entry point is the address of ImageBase + AddressOfEntryPoint
	realEntryPoint = pe.OPTIONAL_HEADER.AddressOfEntryPoint + pe.OPTIONAL_HEADER.ImageBase

	# The init function that we are looking for is called by the 'EntryPointFunction' using a jmp.
	# Quick and dirty: We use objdump to find the address of the entry point
	entryPoint = subprocess.check_output('objdump -d ' + path + ' 2> /dev/null | grep "' + hex(realEntryPoint).lstrip("0x") + 
					     '" -A 20 | grep "jmp" | egrep -o \'0x[a-fA-F0-9]+\'',
                                             shell = True)
	

        entryPoint = int(entryPoint, 16) - pe.OPTIONAL_HEADER.ImageBase - getTextOffsetVA(pe)
	printInfo(" Entry Point @ 0x%x (Offset: 0x%x)\n" % (entryPoint + pe.OPTIONAL_HEADER.ImageBase + getTextOffsetVA(pe), entryPoint))

	printInfo("\t [+] Writing Entry Point (0x%x) ...\n" % (entryPoint + getTextOffset(pe) + data_offset))
        if is64bit(pe):
               outfile.write(struct.pack("Q", entryPoint + getTextOffset(pe) + data_offset))
        else:
               outfile.write(struct.pack("I", entryPoint + getTextOffset(pe) + data_offset))


	# Write Relocs
	printInfo("\t [-] Writing Relocations...\n")
	# Writing Reloc Count
	printInfo("\t\t [+] Writing Relocation Count... (%d)\n" % (relocs.size()))
	if is64bit(pe):
		outfile.write(struct.pack("Q", relocs.size()))
	else:
		outfile.write(struct.pack("I", relocs.size()))

	
	for r in relocs:
		printInfo("\t\t [+] Setting Relocation @ 0x%x to 0x%x\n"	% (r[0] + data_offset, r[1] + data_offset))
		if is64bit(pe):
			outfile.write(struct.pack("Q", r[0] + data_offset))
			outfile.write(struct.pack("Q", r[1] + data_offset))
		else:
			outfile.write(struct.pack("I", r[0] + data_offset))
			outfile.write(struct.pack("I", r[1] + data_offset))
	
	
	# Write Relocs
	printInfo("\t [-] Writing ESP Patch Symbols...\n")
	# Writing Reloc Count
	printInfo("\t\t [+] Writing ESP Patch Symbol Count... (%d)\n" % (esp_patch_symbols.size()))
	if is64bit(pe):
		outfile.write(struct.pack("Q", esp_patch_symbols.size()))
	else:
		outfile.write(struct.pack("I", esp_patch_symbols.size()))

	
	for e in esp_patch_symbols:
		printInfo("\t\t [+] Setting ESP Patch Symbol to 0x%x\n"	% (e))
		if is64bit(pe):
			outfile.write(struct.pack("Q", e + data_offset))
		else:
			outfile.write(struct.pack("I", e + data_offset))


	# Write Imports
	printInfo("\t [-] Writing Imports...\n")

	# Write start address for KPCR search
	printInfo("\t\t [+] Writing Start Address (0x%x) for KPCR search...\n" % (KPCR_START))
	if is64bit(pe):
		outfile.write(struct.pack("Q", KPCR_START))
	else:
		outfile.write(struct.pack("I", KPCR_START))

	# Writing Reloc Count
	printInfo("\t\t [+] Writing Import Count... (%d)\n" % (imports.size()))
	
	if is64bit(pe):
		outfile.write(struct.pack("Q", imports.size()))
	else:
		outfile.write(struct.pack("I", imports.size()))
	
	
	# Get relative offsets
	printInfo("\t\t [+] Reading Relative offsets from '%s'...\n" % (SYMBOL_OFFSETS_NTOS))	

	
	relativefile = open(SYMBOL_OFFSETS_NTOS, 'r')
	imports_rel = {}
	
	for line in relativefile:
		tmp = ImportRelative(line)
		imports_rel[tmp.getName()] = tmp
	
	relativefile.close()

	for i in imports:
		printInfo("\t\t [+] Setting Import '%s' @ 0x%x to 0x%x\n"	% (i[0], i[1] + data_offset, imports_rel[i[0]].getAddress()))
		
		if is64bit(pe):
			outfile.write(struct.pack("Q", i[1] + data_offset))
			outfile.write(struct.pack("Q", imports_rel[i[0]].getAddress()))
		else:
			outfile.write(struct.pack("I", i[1] + data_offset))
			outfile.write(struct.pack("I", imports_rel[i[0]].getAddress()))


	# Writing File
	printInfo("\t [+] Writing PE...\n")
	buf_size = 1024
	buf = infile.read(buf_size)	

	while buf != "":
		outfile.write(buf)
		buf = infile.read(buf_size)

	# Write Function stubs
	printInfo("\t [+] Writing Function Stubs...\n")
	outfile.write(PRINTF)
	
	# Write Wrapper
	wrapper_file = False
	
	try:
	    	infile = open(os.path.splitext(path)[0] + WRAPPER_EXTENSION, "rb")
	    	wrapper_file = True
	except IOError:
		wrapper_file = False
	
	if wrapper_file:
		printInfo("\t [+] Writing Wrapper...\n")
		
		buf = infile.read(buf_size)	

		while buf != "":
			outfile.write(buf)
			buf = infile.read(buf_size)
			
		infile.close()


# Here we go
printBanner()

try:
	parse(sys.argv[1])
except Exception, e:
	handleError(e)
	
