section .text
    global load_gdt
    global detect_v86
    global enter_v86

load_gdt:
    mov eax, [esp + 4]
    lgdt [eax]

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    cli
    mov	eax, cr0
    or eax, 1
    mov	cr0, eax

    jmp 0x08:far_jump
far_jump:
    ret


detect_v86:
   smsw    ax
   and     eax,1
   ret


enter_v86:
   mov ebp, esp

   push dword  [ebp+4]
   push dword  [ebp+8]
   pushfd
   or dword [esp], (1 << 17)
   push dword [ebp+12]
   push dword  [ebp+16]
   iret
