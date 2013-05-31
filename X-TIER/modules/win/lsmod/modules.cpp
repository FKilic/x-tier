#include "stdafx.h"

void modulesUnload(IN PDRIVER_OBJECT DriverObject);
NTSTATUS modulesCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS modulesDefaultHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

#ifdef __cplusplus
extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING  RegistryPath);
#endif

#define KD_VERSION_OFFSET 0x34
#define PSLIST_OFFSET 0x18
#define FLINK_OFFSET 0x0
#define BLINK_OFFSET 0x4
#define MODULE_BASE_OFFSET 0x18
#define MODULE_NAME_OFFSET 0x2c


void DriverUnload(PDRIVER_OBJECT pDriverObject)
{
    DbgPrint("Driver unloading\n");
}

unsigned int findKPCR()
{
	int selfptr_offset = 0x1c;
	int kprcb_pointer = 0x20;
	int kprcb_location = 0x120;
	
	// ToDo: Fix Address!
	unsigned int start_address = 0x82700c00;
	
	// Go
	char *search = (char *)start_address;
	
	while((unsigned int)search <= 0x827ffc00)
	{
		//DbgPrint("0x%x\n", (unsigned int)search);
	
		if((unsigned int)search == *((unsigned int *)(search + selfptr_offset))
			&& *((unsigned int *)(search + kprcb_pointer)) == (unsigned int)(search + kprcb_location))
		{
			return (unsigned int) search;
		}
		
		search += 0x1000;
	}
	
	return 0;
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING  RegistryPath)
{
	
	char *module = NULL;
	char *list_head = NULL;
	int safety = 0;
	
	// String
	char buf[512];
	ANSI_STRING str;
	str.Length = 0;
	str.MaximumLength = 512;
	str.Buffer = buf;

	// HackyDiHackHack
	// Comment the next line. This allows us to directly jump to the .text section.
    //DriverObject->DriverUnload = DriverUnload;
	
	
	// Get the PsLoadedMOduleList
	list_head = (char *)findKPCR();
	list_head = (char *) *((unsigned int *)(list_head + KD_VERSION_OFFSET));
	list_head = (char *) *((unsigned int *)(list_head + PSLIST_OFFSET));
	
	// Get First Module
	module =  (char *) *((unsigned int *)list_head);
	
	//DbgPrint("0x%x\n", (unsigned int)module);
	
	// Print Header
	DbgPrint("Loaded Modules:\n\n");
	DbgPrint(" BASE       \t NAME\n");
	DbgPrint(" ----       \t ----\n");
	
	// Iterate over all modules
	do
	{
		// Set Pointer to Module Head
		module -= FLINK_OFFSET;
		
		// Get Unicode String
		RtlUnicodeStringToAnsiString(&str, (PUNICODE_STRING)(module + MODULE_NAME_OFFSET), 0);
		
		// Print Data
		DbgPrint(" 0x%08x \t %s\n", *((unsigned long *)(module + MODULE_BASE_OFFSET)), buf);
		
		// Next Module
		module = (char *) *((unsigned int *)(module + FLINK_OFFSET));
	
		safety++;
	}
	while(list_head != module && module != NULL && safety < 300);
	
	
    return STATUS_SUCCESS;
}