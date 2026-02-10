section .text
    extern syscall_handler
    global syscall_int_0x80

; Syscall handler for int 0x80
; Called from user mode (Ring 3)
syscall_int_0x80:
    cli
    push byte 0         ; Error code (not used for syscalls)
    push byte 0x80      ; Interrupt number
    
    ; Save all registers
    pusha
    mov ax, ds
    push eax
    
    ; Load kernel data segment
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Call C syscall handler
    push esp
    call syscall_handler
    pop esp
    
    ; Restore registers
    pop ebx
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx
    
    popa
    add esp, 0x8        ; Skip error code and int_no
    
    sti
    iret
