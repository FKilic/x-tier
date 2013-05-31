;nasm -f elf64 -l sc.lst sc.asm
BITS 64
        SECTION .text
        global main
main:
        jmp start

strcmp:
   pop rax              ; ret addr
   pop rcx              ; len(symbolname)
   pop rdi              ; *symbolname
   push rax             ; ret addr

   mov rsi, [rbx+0x8]   ;
   repe cmpsb           ; [rsi],[rdi] 
   mov rax, 0
   jne strcmpret
   mov rax, [rbx]

strcmpret:
   ret



patcher:
   mov r8, [rbp+8]      ; counter
   mov r9, rbp

patchloop:
   add r9, 16
   test r8, r8          ; if count == 0
   jz endpatcher
   dec r8

   mov rax, rbp         ; baseaddr
   add rax, [r9]        ; addr where to write
   mov rbx, rbp         ; baseaddr
   add rbx, [r9+8]      ; addr what to write
   mov [rax], rbx
   jmp patchloop

endpatcher:
   ret


sc:
   pop rbp           ; our base poiting to our data
   mov r11, [rbp]     ; entrypoint
   call patcher
   mov rbp, r9       ; pointing after the patch stack
   mov rbx, [rbp]    ; Begin ; End = [rbp+8]

suchloop:
   mov r8, [rbp+16]  ; counter symbols
   lea r15, [rbp+24] ; *(len of first symbol)

vergleichloop:
   test r8, r8       ; if countsymbols == 0
   jz endvergleich  
   dec r8
   mov r9, [r15]     ; len of symbol
   add r15, 8        ; next pointer
   lea r10, [r15]    ; addr of symbolname
   add r15, r9
   push r10
   push r9
   call strcmp
   test rax, rax
   jz nosymbolfound
   mov r9, [r15]  ; addr to write
   add r9, rbp
   mov [r9], rax  ; writing addr of symbol to destination
nosymbolfound:
   add r15, 0x8
   jmp vergleichloop  

endvergleich:
   add rbx, 0x10     ; next list_head
   cmp rbx, [rbp+8]  ; if begin == end
   je exit
   jmp suchloop

  

exit:
   nop
   mov rax, r11       ; entrypointoffset
   add rax, rbp      ; add base offset
   call rax
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
; __start___ksymtab
; __stop___ksymtab
; count symbols
; len(printk)
; printk\0
; addtowrite printk
; ...



