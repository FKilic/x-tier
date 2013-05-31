;nasm -f elf32 -l sc_upgrade.lst sc_upgrade.asm
BITS 32 
        SECTION .text
        global main
main:
        jmp start


patcher:
   push edx
   push edi
   push ebx
   push ecx

   mov edx, [ebp+4]      ; counter
   mov edi, ebp

patchloop:
   add edi, 8
   test edx, edx        ; if count == 0
   jz endpatcher
   dec edx

   mov eax, ebp   	; baseaddr
   add eax, [edi]       ; addr where to write
   mov ebx, ebp         ; baseaddr
   add ebx, [edi+4]     ; addr what to write
   mov [eax], ebx
   jmp patchloop

endpatcher:
   mov eax, edi

   pop ecx
   pop ebx
   pop edi
   pop edx
   ret

esppatcher:
   pop eax		; ret addr
   pop edx		; distance to our data
   pop ecx		; ESP value
   push eax		; ret addr
   push ebp		; save EBP
   add ebp, edx		; pointing after patch stack

   mov esi, [ebp]	; counter
   mov edi, ebp

esppatchloop:
   add edi, 4
   test esi, esi	; if count == 0
   jz espendpatcher
   dec esi

   mov eax, ebp		; segment addr
   sub eax, edx		; baseaddr = segmentaddr - distance
   add eax, [edi]	; addr where to write
   mov [eax], ecx	; write ESP
   jmp esppatchloop

espendpatcher:
   mov eax, edi 	; return pointer to next segment
   pop ebp		; restore original ebp
   ret



saveregisters:
   add esp, 0x4		; ret value
   add esp, 0x8		; basep and espval on stack
   add esp, 0x1c	; all registers 7*4
   push eax
   push ebx
   push ecx
   push edx
   push edi
   push esi
   push ebp
   sub esp, 0x8		; 2 values
   sub esp, 0x4		; ret value
   ret

restoreregisters:
   add esp, 0x4		; ret value
   add esp, 0x4		; 1 value on stack
   pop ebp
   pop esi
   pop edi
   pop edx
   pop ecx
   pop ebx
   pop eax
   sub esp, 0x1c	; 7*8 registers
   sub esp, 0x4		; 1 value on stack
   sub esp, 0x4		; ret value
   ret

iskpcr:
   pop eax		; ret
   pop ecx		; kprc test addr
   push eax		; ret addr
   push edx		; save register
   push ebx 		; save 

   mov ebx, ecx
   add ecx, 0x1c
   mov edx, [ecx]
   cmp ebx, edx
   jz kpcrfound
 
   mov eax, 0x1
   jmp endiskpcr

kpcrfound:
   mov eax, 0x0

endiskpcr:
   pop ebx
   pop edx
   ret
   

bruteKPCR:
   pop eax		; ret
   pop edi		; KPCR lower bytes
   push eax		; ret
   push ecx
   push edx
   push ebx
 
   mov ebx, 0x0
   mov edx, 0x0		; counter until 0x8f
   jmp bruteKPCRloop

bruteKPCRcmp:
   mov eax, edx
   shl eax, 0xc
   add eax, edi		; add kpcr lower bytes
   mov ebx, eax
   push ebx
   call iskpcr
   test eax, eax
   jz bruteKPCRfound
   inc edx

bruteKPCRloop:
   cmp edx, 0x8f	
   jng bruteKPCRcmp

   mov eax, 0x0		; not found
   jmp endbruteKPCR

bruteKPCRfound:
   mov eax, ebx

endbruteKPCR:
   pop ebx
   pop edx
   pop ecx
   ret

   
errorEXITtoVM:
   mov eax, 0xdeadbeef
   int 0x50

getKernelBase:
   pop eax		; ret
   pop edi		; KPCR 
   push eax		; ret
   push ecx
   add edi, 0x34	; KDVersionBlock
   mov ecx, [edi]
   add ecx, 0x10	; Kernelbase
   mov eax, [ecx]
   pop ecx
   ret

symbolpatcher:
   pop eax		; ret
   pop esi		; distance
   push eax		; ret
 
   push ebx
   push ecx
   push edx
   push edi
   push ebp

   add ebp, esi		; pointing to KPCR segment
   mov edi, [ebp]	; KPCR lower addr
   push edi
   call bruteKPCR
   test eax, eax
   jz errorEXITtoVM
   mov edi, eax		; else save KPCR Addr in EDI
   push edi
   call getKernelBase
   mov ecx, eax		; Save Kernelbase in ecx
   nop			; reserved for debug
   mov edx, [ebp+0x4]	; symbol count
   mov edi, ebp		; segment base

symbolpatcherloop:
   add edi, 0x8		; pointing to first data / or nextsegment if count == 0
   test edx, edx	; if count == 0
   jz endsymbolpatcher

   dec edx		; count--


   mov eax, ebp 	; segmentpointer
   sub eax, esi		; distance to base
   add eax, [edi]	; offset where to write
   
   mov ebx, ecx		; Kernelbase
   add ebx, [edi+4] 	; Offset based on kernelbase

   mov [eax], ebx	; Patch!
   nop
   nop 
   jmp symbolpatcherloop 
   
endsymbolpatcher:
   mov eax, edi
   pop ebp
   pop edi
   pop edx
   pop ecx
   pop ebx
   ret



sc:
   call saveregisters	; Saving all registers
   pop ebp           	; our base poiting to our data
   pop esi		; original ESP value, which is pushed by the hypervisor
   mov ecx, [ebp]     	; entrypoint
   call patcher
   mov ebx, eax		; pointer to next data
   sub ebx, ebp		; distance to next data

   push ecx		; save entrypoint

   push esi		; original ESP value
   push ebx		; distance to data
   call esppatcher
   mov ebx, eax
   sub ebx, ebp

   pop ecx		; restore entrypoint

   push ebx
   call symbolpatcher

exit:
   nop
   mov eax, ecx       	; entrypointoffset
   add eax, ebp      	; add base offset
   push eax		; add to call
   call restoreregisters; Restore original register values
   pop eax
   add esp, 800		; Constant offset to stackEND (=original stack)
   call eax
   hlt               	; VMI exit

start:
   call sc
stack:
   xor eax, eax      	; will be changed to string


; Aufbau:
; entrypoint
; patch_count
; patch_add_off
; patch_val_off
; ...
; esppatch_count
; esppatch_add_off
; ...
; KPCR_start_lowerbytes
; count symbols
; offset in module + base
; offset in kernelbase
; ...




