#include "stdafx.h"

void filesUnload(IN PDRIVER_OBJECT DriverObject);
NTSTATUS filesCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS filesDefaultHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

#ifdef __cplusplus
extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING  RegistryPath);
#endif


#define FLINK_OFFSET 0xb8
#define BLINK_OFFSET 0xbc
#define PID_OFFSET 0xb4
#define NAME_OFFSET 0x16c 

#define HANDLE_TABLE_OFFSET 0xf4
#define HANDLE_TABLE_FLINK_OFFSET 0x10
#define HANDLE_TABLE_BLINK_OFFSET 0x14
#define HANDLE_TABLE_EPROC_OFFSET 0x4
#define HANDLE_TABLE_TABLE_OFFSET 0x0
#define HANDLE_TABLE_COUNT_OFFSET 0x30

#define FILE_NAME_OFFSET 0x30
#define FILE_DEVICE_OFFSET 0x4

/*
typedef struct _LIST_ENRTY
{
	PVOID FLINK;
	PVOID BLINK;
} LIST_ENTRY, *PLIST_ENRTY;
*/

typedef struct _HANDLE_TABLE_ENRTY
{
	PVOID Object;
	ULONG Mask;
} HANDLE_TABLE_ENRTY, *PHANDLE_TABLE_ENTRY;

typedef struct _HANDLE_TABLE
{
    unsigned long Table;
	PEPROCESS QuotaProcess;
	PVOID UniqueProcessId;
	PVOID HandleLock;
	LIST_ENTRY HandleTableList;
	PVOID HandleContentionEvent;
	PVOID DebugInfo;
	LONG ExtraInfoPages;
	ULONG Flags;
	LONG FirstFreeHandle;
	PHANDLE_TABLE_ENTRY LastFreeHandleEntry;
	LONG HandleCount;
	ULONG NextHandleNeedingPool;
} HANDLE_TABLE, *PHANDLE_TABLE;


void DriverUnload(PDRIVER_OBJECT pDriverObject)
{
    DbgPrint("Driver unloading\n");
}

PHANDLE_TABLE_ENTRY getFirstEntry(PHANDLE_TABLE tbl)
{
	switch(tbl->Table & 0x3)
	{
		case 0:
			return (PHANDLE_TABLE_ENTRY) (tbl->Table);
			
		case 1:
			return (PHANDLE_TABLE_ENTRY) *((unsigned long *)(tbl->Table - 1));
	
		default:
			DbgPrint("Cannot Handle 0x%x\n", tbl->Table);
	}
	
	return NULL;
}

PHANDLE_TABLE_ENTRY getNextTable(PHANDLE_TABLE tbl, unsigned int i)
{
	return (PHANDLE_TABLE_ENTRY) *((unsigned long *)(tbl->Table - 1 + (4 * i)));
}

char * getCurrentProcess() {

	__asm{
		mov eax, fs:[0x124]
		mov eax, [eax+0x150]
		
	}

	return;
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING  RegistryPath)
{
	char *eprocess = NULL;
	char *start = NULL;
	char *tmp = NULL;
	unsigned int pid = 0;
	int i = 0;
	int safety = 1;
	unsigned short length = 0;
	char printed = 0;
	unsigned long obj = 0;
	
	// HANDLE_TABLE
	PHANDLE_TABLE htbl = NULL;
	PHANDLE_TABLE_ENTRY htbl_entry = NULL;
	unsigned short type = 0;
	unsigned long device_object = 0;
	unsigned long unicode_str = 0;
	int existing_count = 0;
	int tbl_index = 0;
	
	// String
	char buf[512];
	ANSI_STRING str;
	str.Length = 0;
	str.MaximumLength = 512;
	str.Buffer = buf;
	
	
	// HackyDiHackHack
	// Comment the next line. This allows us to directly jump to the .text section.
    // DriverObject->DriverUnload = DriverUnload;
	
	
	// Get the current process
	start = getCurrentProcess();
	
	// Prepare For iteration
	start += FLINK_OFFSET;
	eprocess = start;
	
	// Print Header
	DbgPrint("Open Files:\n\n");
	DbgPrint("  EPROCESS \t   PID \t NAME\n");
	DbgPrint("  -------- \t   --- \t ----\n");
	
	// Iterate over all processes
	do
	{
		// Print data
		eprocess -= FLINK_OFFSET;
		
		// Get Pid
		pid = *((unsigned int *)(eprocess + PID_OFFSET));
		
		// Somewehere in there will be the Process list head. Ignore it.
		if(pid < 65536)
		{
			DbgPrint("0x%08x \t % 5d \t %s\n", (unsigned long)eprocess, pid, (eprocess + NAME_OFFSET));
			
			
			// Get Handle table
			htbl =  (PHANDLE_TABLE) *((unsigned int *)(eprocess + HANDLE_TABLE_OFFSET));
		
			
			htbl_entry = getFirstEntry(htbl);
			// DbgPrint("Handles: %d\n", htbl->HandleCount);
		
			// Reset printed
			printed = 0;
		
			// We have to count existing objects only!
			// We have to reset the tbl_index when we switch to a new table!
			for(i = 0, tbl_index = 0, existing_count = 0; 
				existing_count < htbl->HandleCount; 
				i++, tbl_index++)
			{
				// The HANDLE_TABLES are splitted. After 512 entries we have to
				// take the next one.
				if(i != 0 && i % 512 == 0)
				{
					htbl_entry = getNextTable(htbl, i / 512);
					tbl_index = 0;
				}
				
				// Is this a valid object?
				if(htbl_entry != NULL && htbl_entry[tbl_index].Object != NULL)
				{
					// Yep -> increase count
					existing_count++;
					
					// Get Object
					// We have to add the offset of 0x17 to get the offset!
					// We have to make sure to hit the word boundary.
					obj = ((((unsigned long)(htbl_entry[tbl_index].Object)) + 0x17) & 0xfffffffc);
					
					// Get Type
					type = *((unsigned short *)obj);
					
					// File is 5
					if(type == 5)
					{
						device_object = *(unsigned long *)(obj + FILE_DEVICE_OFFSET);
						length = *(unsigned short *)(obj + FILE_NAME_OFFSET);
						unicode_str = *(unsigned long *)(obj + FILE_NAME_OFFSET + 4);
						
						// Crude Filter
						if( device_object != 0 &&
							device_object> 0x80000000 &&
							unicode_str != 0 &&
							unicode_str > 0x80000000 &&
							length > 0)
						{	
							//DbgPrint("String @ 0x%x - Length: %d\n", (((unsigned long)(htbl_entry[tbl_index].Object)) + 0x17 + FILE_NAME_OFFSET), length);
							
							// Get Unicode String
							RtlUnicodeStringToAnsiString(&str, 
													(PUNICODE_STRING)(obj + FILE_NAME_OFFSET), 
													0);
						
							// Print Data
							DbgPrint("\t\t\t %s\n", buf);
							
							printed = 1;
							
						}
					}
				
					
				}
				
			}
			
			if(printed == 1)
				DbgPrint("\n");
			
			
		}
		
		// Get next process
		eprocess = (char *)(*((long *)(eprocess + FLINK_OFFSET)));
	
		safety++;
	}
	while(start != eprocess && eprocess != NULL && safety < 300);
	
	
	
    return STATUS_SUCCESS;
}



