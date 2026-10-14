ENTRY(_start)

SECTIONS
{
    . = 0x00100000;

    __kernel_section_start = .;

    .text ALIGN(4096) : {
        __kernel_text_section_start = .;
        code = .; _code = .; __code = .;
        *(.multiboot)
        *(.text*)
        __kernel_text_section_end = .;
    }

    .rodata ALIGN(4096) : {
        __kernel_rodata_section_start = .;
        rodata = .; _rodata = .; __rodata = .;
        *(.rodata*)
        __kernel_rodata_section_end = .;
    }

    .data ALIGN(4096) : {
        __kernel_data_section_start = .;
        data = .; _data = .; __data = .;
        *(.data*)
        __kernel_data_section_end = .;
    }

    .bss ALIGN(4096) : {
        __kernel_bss_section_start = .;
        bss = .; _bss = .; __bss = .;
        *(COMMON)
        *(.bss*)
        __kernel_bss_section_end = .;
    }

    . = ALIGN(4096);
    end = .; _end = .; __end = .;
    __kernel_section_end = .;
}
