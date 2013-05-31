//#include "stdafx.h"
#include "stdio.h"
#include "Ntifs.h"
#include "Ntddk.h"
#include "Wdm.h"

void tasklistUnload(IN PDRIVER_OBJECT DriverObject);
NTSTATUS tasklistCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS tasklistDefaultHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

#ifdef __cplusplus
extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING  RegistryPath);
#endif


#define FLINK_OFFSET 0xb8
#define BLINK_OFFSET 0xbc
#define PID_OFFSET 0xb4
#define NAME_OFFSET 0x16c 

void DriverUnload(PDRIVER_OBJECT pDriverObject)
{
    DbgPrint("Driver unloading\n");
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

        // Lets try to avoid endless loops.
        int safety = 0;

        // HackyDiHackHack
        // Comment the next line. This allows us to directly jump to the .text section.
	// DriverObject->DriverUnload = DriverUnload;


        // Get the current process
        start = getCurrentProcess();
		
        // Prepare For iteration
        start += FLINK_OFFSET;
        eprocess = start;

        // Print Header
        DbgPrint("Running Modules:\n\n");
        DbgPrint("  EPROCESS \t   PID  \t NAME\n");
        DbgPrint("  -------- \t   ---  \t ----\n");

        // Iterate over all processes
        do
        {
                // Print data
                eprocess -= FLINK_OFFSET;

                // Get Pid
                pid = *((unsigned int *)(eprocess + PID_OFFSET));

                // Somewehere in there will be the Process list head. Ignore it.
                if(pid < 65536)
                        DbgPrint("0x%08x \t % 5d \t %s\n", (unsigned long)eprocess, pid, (eprocess + NAME_OFFSET));

                // Get next process
                eprocess = (char *)(*((long *)(eprocess + FLINK_OFFSET)));

                safety++;
        }
        while(start != eprocess && eprocess != NULL && safety < 300);
		

    return STATUS_SUCCESS;

}


