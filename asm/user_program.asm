section .text
    global user_program_start
    global user_program_end

; User mode program
; This code runs in Ring 3 and makes syscalls
user_program_start:
    ; First syscall - SYS_WRITE
    ; Calculate address of msg1 relative to current position
    call .get_eip
.get_eip:
    pop ebx                     ; EBX = current EIP
    add ebx, msg1 - .get_eip    ; EBX = address of msg1
    mov eax, 1                  ; syscall number
    int 0x80                    ; make syscall
    
    ; Second syscall
    call .get_eip2
.get_eip2:
    pop ebx
    add ebx, msg2 - .get_eip2
    mov eax, 1
    int 0x80
    
    ; Loop forever
.loop:
    jmp .loop

; Messages (part of the program)
msg1:
    db "Hello from Ring 3 (user mode)!", 10, 0

msg2:
    db "Syscall completed successfully!", 10, 0

user_program_end:
