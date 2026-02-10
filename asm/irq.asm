section .text
    extern isr_irq_handler
    extern task_scheduler_tick

irq_handler:
    pusha
    mov ax, ds
    push eax

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call isr_irq_handler
    pop esp

    pop ebx
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx

    popa
    add esp, 0x8

    sti
    iret

; Special handler for timer interrupt (IRQ0) with task switching
global irq_0_with_task_switch
irq_0_with_task_switch:
    cli
    push byte 0         ; Error code
    push byte 32        ; Interrupt number (IRQ0 = 32)
    
    ; Save all registers
    pusha
    mov ax, ds
    push eax

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Call scheduler tick - it will handle task switching
    ; and return the ESP to use
    push esp
    call task_scheduler_tick
    mov esp, eax        ; ESP returned might be different (task switched)

    ; Restore registers from (potentially new) stack
    pop ebx
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx

    popa
    add esp, 0x8        ; Skip error code and int_no

    sti
    iret


%macro IRQ 2
  global irq_%1
  irq_%1:
    cli
    push byte 0
    push byte %2
    jmp irq_handler
%endmacro


IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47


