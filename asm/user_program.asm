section .text
    global user_program_start
    global user_program_end

; User mode program
; This code runs in Ring 3 and makes syscalls
user_program_start:
    ; First syscall - SYS_WRITE
    mov eax, 1                  ; syscall number
    lea ebx, [rel msg1]         ; pointer to message (PC-relative)
    int 0x80                    ; make syscall
    
    ; Second syscall
    mov eax, 1
    lea ebx, [rel msg2]
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
