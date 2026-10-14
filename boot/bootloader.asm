; Multiboot 1 specification
MULTIBOOT_PAGE_ALIGN    equ 1 << 0   ; Align all boot modules on i386 page (4KB) boundaries
MULTIBOOT_MEMORY_INFO   equ 1 << 1   ; Must provide memory information
FLAGS                   equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
MAGIC_HEADER            equ 0x1BADB002
CHECKSUM                equ -(MAGIC_HEADER + FLAGS)

BOOTLOADER_MAGIC        equ 0x2BADB002

section .multiboot
    align 4
    dd MAGIC_HEADER
    dd FLAGS
    dd CHECKSUM

section .data
    align 4096


section .initial_stack, nobits
    align 4

stack_bottom:
    resb 104856
stack_top:

section .text
    global _start
    global MAGIC_HEADER
    global BOOTLOADER_MAGIC


_start:
    mov esp, stack_top
    extern kmain
    push ebx
    push eax
    call kmain
loop:
    jmp loop
