NASM = /usr/bin/nasm

CC = /usr/bin/gcc

LD = /usr/bin/ld

GRUB = /usr/bin/grub-mkrescue

LIB = lib
DRIVERS = drivers
KERNEL = kernel
ASM = asm
BUILD = build
BOOT = boot
INC = include
INCLUDE=-I$(INC) -I/usr/include/x86_64-linux-gnu

MKDIR= mkdir -p
CP = cp -f

ASM_FLAGS = -f elf32
CC_FLAGS = $(INCLUDE) -m32 -std=gnu99 -Wall -Wextra -ffreestanding 
LD_FLAGS = -m elf_i386 -T $(BOOT)/linker.ld -nostdlib -z noexecstack


TARGET=autismos.bin
TARGET_ISO=autismos.iso
ISO_DIR=isodir


OBJECTS=$(BUILD)/bootloader.o $(BUILD)/load_gdt.o\
		$(BUILD)/load_idt.o $(BUILD)/exception.o $(BUILD)/irq.o\
		$(BUILD)/io_ports.o $(BUILD)/string.o $(BUILD)/gdt.o $(BUILD)/idt.o $(BUILD)/isr.o $(BUILD)/8259_pic.o\
		$(BUILD)/keyboard.o $(BUILD)/mouse.o $(BUILD)/kernel.o


all: $(BUILD) $(OBJECTS)
	$(LD) $(LD_FLAGS) -o $(TARGET) $(OBJECTS)
	grub-file --is-x86-multiboot $(TARGET)
	$(MKDIR) $(ISO_DIR)/boot/grub
	$(CP) $(TARGET) $(ISO_DIR)/boot/
	$(CP) $(BOOT)/grub.cfg $(ISO_DIR)/boot/grub/
	$(GRUB) -o $(TARGET_ISO) $(ISO_DIR)
	rm -rf $(TARGET) $(ISO_DIR)
	rm -rf $(BUILD)


$(BUILD):
	$(MKDIR) $(BUILD)




$(BUILD)/bootloader.o : $(BOOT)/bootloader.asm
	$(NASM) $(ASM_FLAGS) $(BOOT)/bootloader.asm -o $(BUILD)/bootloader.o

$(BUILD)/load_gdt.o : $(ASM)/load_gdt.asm
	$(NASM) $(ASM_FLAGS) $(ASM)/load_gdt.asm -o $(BUILD)/load_gdt.o

$(BUILD)/load_idt.o : $(ASM)/load_idt.asm
	$(NASM) $(ASM_FLAGS) $(ASM)/load_idt.asm -o $(BUILD)/load_idt.o

$(BUILD)/exception.o : $(ASM)/exception.asm
	$(NASM) $(ASM_FLAGS) $(ASM)/exception.asm -o $(BUILD)/exception.o

$(BUILD)/irq.o : $(ASM)/irq.asm
	$(NASM) $(ASM_FLAGS) $(ASM)/irq.asm -o $(BUILD)/irq.o



$(BUILD)/kernel.o : $(KERNEL)/kernel.c
	$(CC) $(CC_FLAGS) -c $(KERNEL)/kernel.c -o $(BUILD)/kernel.o

$(BUILD)/io_ports.o : $(KERNEL)/io_ports.c
	$(CC) $(CC_FLAGS) -c $(KERNEL)/io_ports.c -o $(BUILD)/io_ports.o

$(BUILD)/gdt.o : $(KERNEL)/gdt.c
	$(CC) $(CC_FLAGS) -c $(KERNEL)/gdt.c -o $(BUILD)/gdt.o

$(BUILD)/idt.o : $(KERNEL)/idt.c
	$(CC) $(CC_FLAGS) -c $(KERNEL)/idt.c -o $(BUILD)/idt.o

$(BUILD)/isr.o : $(KERNEL)/isr.c
	$(CC) $(CC_FLAGS) -c $(KERNEL)/isr.c -o $(BUILD)/isr.o

$(BUILD)/8259_pic.o : $(KERNEL)/8259_pic.c
	$(CC) $(CC_FLAGS) -c $(KERNEL)/8259_pic.c -o $(BUILD)/8259_pic.o



$(BUILD)/keyboard.o : $(DRIVERS)/keyboard.c
	$(CC) $(CC_FLAGS) -c $(DRIVERS)/keyboard.c -o $(BUILD)/keyboard.o

$(BUILD)/mouse.o : $(DRIVERS)/mouse.c
	$(CC) $(CC_FLAGS) -c $(DRIVERS)/mouse.c -o $(BUILD)/mouse.o



$(BUILD)/string.o : $(LIB)/string.c
	$(CC) $(CC_FLAGS) -c $(LIB)/string.c -o $(BUILD)/string.o

