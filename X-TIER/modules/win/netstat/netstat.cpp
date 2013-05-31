#include "stdio.h"
#include "Ntifs.h"
#include "Ntddk.h"
#include "Wdm.h"

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

#define SOCKET_OFFSET 0xc
#define SOCKET_UDP 0xafd1
#define SOCKET_TCP 0xafd2
#define SOCKET_INFO_OFFSET 0x70
#define SOCKET_SRC_OFFSET 0x80
#define SOCKET_DEST_OFFSET 0x90


#ifdef __cplusplus
extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING  RegistryPath);
#endif

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
        unsigned long obj = 0;

        // HANDLE_TABLE
        PHANDLE_TABLE htbl = NULL;
        PHANDLE_TABLE_ENTRY htbl_entry = NULL;
        unsigned short type = 0;
        unsigned long device_object = 0;
        unsigned long fscontext = 0;
        unsigned long fscontext2 = 0;
        int existing_count = 0;
        int tbl_index = 0;

        unsigned long some_struct = 0;
        unsigned char *src = 0;
        unsigned char *dest = 0;


        // HackyDiHackHack
        // Comment the next line. This allows us to directly jump to the .text section.
    // DriverObject->DriverUnload = DriverUnload;


        // Get the current process
        start = getCurrentProcess();

        // Prepare For iteration
        start += FLINK_OFFSET;
        eprocess = start;

        // Print Header
        DbgPrint("Network Connections:\n\n");
        DbgPrint("TYPE \t SOURCE IP        PORT \t DESTINATION IP \t  PORT\n");
        DbgPrint("---- \t ---------        ---- \t -------------- \t  ----\n");

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
                        // Get Handle table
                        htbl =  (PHANDLE_TABLE) *((unsigned int *)(eprocess + HANDLE_TABLE_OFFSET));


                        htbl_entry = getFirstEntry(htbl);
                        // DbgPrint("Handles: %d\n", htbl->HandleCount);

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
                                        if((type & 0xff) == 5)
                                        {
                                                fscontext = *(unsigned long *)(obj + SOCKET_OFFSET);
                                                fscontext2 = *(unsigned long *)(obj + SOCKET_OFFSET + 4);
                                                device_object = *(unsigned long *)(obj + FILE_DEVICE_OFFSET);

                                                // Crude Filter
                                                if(fscontext != 0 &&
                                                   fscontext2 == 0 &&
                                                   device_object != 0 &&
                                                   fscontext > 0x83000000 &&
                                                   fscontext < 0xb0000000)
                                                {
                                                        type = *((unsigned short *)(fscontext));

                                                        if(type == SOCKET_UDP || type == SOCKET_TCP)
                                                        {
                                                                // We do not know what it is, but it holds the info we need.
                                                                some_struct = *(unsigned long *)(fscontext + SOCKET_INFO_OFFSET);

                                                                // Get Source and Destination
                                                                src = (unsigned char *)(some_struct + SOCKET_SRC_OFFSET);
                                                                dest = (unsigned char *)(some_struct + SOCKET_DEST_OFFSET);

                                                                // Filter empty
                                                                if((*(src + 2) == 0 && *(src + 3) == 0) ||
                                                                        (*(dest + 2) == 0 && *(dest + 3) == 0))
                                                                {

                                                                }
                                                                else
                                                                {
                                                                        if(type == SOCKET_UDP)
                                                                        {
                                                                                DbgPrint(" UDP \t %d.%d.%d.%d \t % 5d \t %d.%d.%d.%d \t % 5d\n", *(src + 4), *(src + 5),
                                                                                                                                                                                         *(src + 6), *(src + 7),
                                                                                                                                                                                         ((*(src + 2) << 8) + *(src + 3)),
                                                                                                                                                                                         *(dest + 4), *(dest + 5),
                                                                                                                                                                                         *(dest + 6), *(dest + 7),
                                                                                                                                                                                         ((*(dest + 2) << 8) + *(dest + 3)));
                                                                        }
                                                                        else
                                                                        {
                                                                                DbgPrint(" TCP \t %d.%d.%d.%d \t % 5d \t %d.%d.%d.%d \t % 5d\n", *(src + 4), *(src + 5),
                                                                                                                                                                                         *(src + 6), *(src + 7),
                                                                                                                                                                                         ((*(src + 2) << 8) + *(src + 3)),
                                                                                                                                                                                         *(dest + 4), *(dest + 5),
                                                                                                                                                                                         *(dest + 6), *(dest + 7),
                                                                                                                                                                                         ((*(dest + 2) << 8) + *(dest + 3)));
                                                                        }
                                                                }

                                                        }


                                                        // Get Unicode String

                                                        // Print Data
                                                        //DbgPrint("\t\t\t %s\n", buf);


                                                }
                                        }


                                }

                        }

                }

                // Get next process
                eprocess = (char *)(*((long *)(eprocess + FLINK_OFFSET)));

                safety++;
        }
        while(start != eprocess && eprocess != NULL && safety < 300);



    return STATUS_SUCCESS;
}
                                                   