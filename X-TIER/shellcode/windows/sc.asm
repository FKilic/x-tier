;nasm -f elf64 -l sc.lst sc.asm
BITS 32 
        SECTION .text
        global main
main:
        jmp start


patcher:
   mov edx, [ebp+8]      ; counter
   mov edi, ebp

patchloop:
   add edi, 16
   test edx, edx          ; if count == 0
   jz endpatcher
   dec edx

   mov eax, ebp         ; baseaddr
   add eax, [edi]        ; addr where to write
   mov ebx, ebp         ; baseaddr
   add ebx, [edi+8]      ; addr what to write
   mov [eax], ebx
   jmp patchloop

endpatcher:
   ret


sc:
   pop ebp           ; our base poiting to our data
   mov ecx, [ebp]     ; entrypoint
   call patcher
   mov ebp, edi       ; pointing after the patch stack
;   mov rbx, [rbp]    ; Begin ; End = [rbp+8]


exit:
   nop
   mov eax, ecx       ; entrypointoffset
   add eax, ebp      ; add base offset
   call eax
   hlt               ; VMI exit

start:
        call sc
stack:
        xor eax, eax      ; will be changed to string


; Aufbau:
; entrypoint
; patch_count
; patch_add_off
; patch_val_off
; ...
; kernel module



