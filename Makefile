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
		$(BUILD)/load_idt.o $(BUILD)/exception.o $(BUILD)/irq.o $(BUILD)/syscall.o $(BUILD)/user_program_asm.o\
		$(BUILD)/io_ports.o $(BUILD)/string.o $(BUILD)/gdt.o $(BUILD)/idt.o $(BUILD)/isr.o $(BUILD)/8259_pic.o\
		$(BUILD)/keyboard.o $(BUILD)/mouse.o $(BUILD)/memory.o $(BUILD)/task.o $(BUILD)/process.o $(BUILD)/ipc.o $(BUILD)/shm.o\
		$(BUILD)/input.o $(BUILD)/network.o $(BUILD)/html.o $(BUILD)/layout.o\
		$(BUILD)/syscall_c.o $(BUILD)/usermode.o $(BUILD)/ux.o $(BUILD)/kernel.o\
		$(BUILD)/video.o $(BUILD)/disk.o $(BUILD)/sound.o


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

$(BUILD)/syscall.o : $(ASM)/syscall.asm
	$(NASM) $(ASM_FLAGS) $(ASM)/syscall.asm -o $(BUILD)/syscall.o

$(BUILD)/user_program_asm.o : $(ASM)/user_program.asm
	$(NASM) $(ASM_FLAGS) $(ASM)/user_program.asm -o $(BUILD)/user_program_asm.o





# Kernel core files
$(BUILD)/kernel.o : $(KERNEL)/core/kernel.c
	$(CC) $(CC_FLAGS) -c $(KERNEL)/core/kernel.c -o $(BUILD)/kernel.o

$(BUILD)/memory.o : $(KERNEL)/core/memory.c
	$(CC) $(CC_FLAGS) -c $(KERNEL)/core/memory.c -o $(BUILD)/memory.o

$(BUILD)/task.o : $(KERNEL)/core/task.c
	$(CC) $(CC_FLAGS) -c $(KERNEL)/core/task.c -o $(BUILD)/task.o

$(BUILD)/process.o : $(KERNEL)/core/process.c
	$(CC) $(CC_FLAGS) -c $(KERNEL)/core/process.c -o $(BUILD)/process.o

# Kernel architecture files
$(BUILD)/io_ports.o : $(KERNEL)/arch/io_ports.c
	$(CC) $(CC_FLAGS) -c $(KERNEL)/arch/io_ports.c -o $(BUILD)/io_ports.o

$(BUILD)/gdt.o : $(KERNEL)/arch/gdt.c
	$(CC) $(CC_FLAGS) -c $(KERNEL)/arch/gdt.c -o $(BUILD)/gdt.o

$(BUILD)/idt.o : $(KERNEL)/arch/idt.c
	$(CC) $(CC_FLAGS) -c $(KERNEL)/arch/idt.c -o $(BUILD)/idt.o

$(BUILD)/isr.o : $(KERNEL)/arch/isr.c
	$(CC) $(CC_FLAGS) -c $(KERNEL)/arch/isr.c -o $(BUILD)/isr.o

$(BUILD)/8259_pic.o : $(KERNEL)/arch/8259_pic.c
	$(CC) $(CC_FLAGS) -c $(KERNEL)/arch/8259_pic.c -o $(BUILD)/8259_pic.o

# Kernel IPC files
$(BUILD)/ipc.o : $(KERNEL)/ipc/ipc.c
	$(CC) $(CC_FLAGS) -c $(KERNEL)/ipc/ipc.c -o $(BUILD)/ipc.o

$(BUILD)/shm.o : $(KERNEL)/ipc/shm.c
	$(CC) $(CC_FLAGS) -c $(KERNEL)/ipc/shm.c -o $(BUILD)/shm.o

# Kernel syscall files
$(BUILD)/syscall_c.o : $(KERNEL)/syscall/syscall.c
	$(CC) $(CC_FLAGS) -c $(KERNEL)/syscall/syscall.c -o $(BUILD)/syscall_c.o

$(BUILD)/usermode.o : $(KERNEL)/syscall/usermode.c
	$(CC) $(CC_FLAGS) -c $(KERNEL)/syscall/usermode.c -o $(BUILD)/usermode.o

# Kernel UX files
$(BUILD)/ux.o : $(KERNEL)/ux/ux.c
	$(CC) $(CC_FLAGS) -c $(KERNEL)/ux/ux.c -o $(BUILD)/ux.o

# Kernel browser files
$(BUILD)/network.o : $(KERNEL)/browser/network.c
	$(CC) $(CC_FLAGS) -c $(KERNEL)/browser/network.c -o $(BUILD)/network.o

$(BUILD)/html.o : $(KERNEL)/browser/html.c
	$(CC) $(CC_FLAGS) -c $(KERNEL)/browser/html.c -o $(BUILD)/html.o

$(BUILD)/layout.o : $(KERNEL)/browser/layout.c
	$(CC) $(CC_FLAGS) -c $(KERNEL)/browser/layout.c -o $(BUILD)/layout.o



# Input drivers
$(BUILD)/keyboard.o : $(DRIVERS)/input/keyboard.c
	$(CC) $(CC_FLAGS) -c $(DRIVERS)/input/keyboard.c -o $(BUILD)/keyboard.o

$(BUILD)/mouse.o : $(DRIVERS)/input/mouse.c
	$(CC) $(CC_FLAGS) -c $(DRIVERS)/input/mouse.c -o $(BUILD)/mouse.o

$(BUILD)/input.o : $(DRIVERS)/input/input.c
	$(CC) $(CC_FLAGS) -c $(DRIVERS)/input/input.c -o $(BUILD)/input.o

# Video drivers
$(BUILD)/video.o : $(DRIVERS)/video/video.c
	$(CC) $(CC_FLAGS) -c $(DRIVERS)/video/video.c -o $(BUILD)/video.o

# Storage drivers
$(BUILD)/disk.o : $(DRIVERS)/storage/disk.c
	$(CC) $(CC_FLAGS) -c $(DRIVERS)/storage/disk.c -o $(BUILD)/disk.o

# Audio drivers
$(BUILD)/sound.o : $(DRIVERS)/audio/sound.c
	$(CC) $(CC_FLAGS) -c $(DRIVERS)/audio/sound.c -o $(BUILD)/sound.o



$(BUILD)/string.o : $(LIB)/string.c
	$(CC) $(CC_FLAGS) -c $(LIB)/string.c -o $(BUILD)/string.o

