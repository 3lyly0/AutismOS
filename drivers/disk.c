#include <string.h>
#include <stdint.h>
#include "disk.h"
#include "io_ports.h"
#include "video.h"

File file_table[MAX_FILES];
uint32_t file_count = 0;

#define ATA_PRIMARY_IO 0x1F0
#define ATA_PRIMARY_CONTROL 0x3F6
#define ATA_CMD_READ 0x20
#define ATA_CMD_WRITE 0x30

void ata_wait() {
    while (inportb(ATA_PRIMARY_IO + 7) & 0x80);
}

void ata_read_sector(uint32_t lba, uint8_t *buffer) {
    outportb(ATA_PRIMARY_IO + 6, 0xE0 | ((lba >> 24) & 0x0F));
    outportb(ATA_PRIMARY_IO + 2, 1);
    outportb(ATA_PRIMARY_IO + 3, (uint8_t)(lba & 0xFF));
    outportb(ATA_PRIMARY_IO + 4, (uint8_t)((lba >> 8) & 0xFF));
    outportb(ATA_PRIMARY_IO + 5, (uint8_t)((lba >> 16) & 0xFF));
    outportb(ATA_PRIMARY_IO + 7, ATA_CMD_READ);
    ata_wait();

    for (int i = 0; i < 256; i++) {
        ((uint16_t *)buffer)[i] = inportw(ATA_PRIMARY_IO);
    }
}

void ata_write_sector(uint32_t lba, uint8_t *buffer) {
    outportb(ATA_PRIMARY_IO + 6, 0xE0 | ((lba >> 24) & 0x0F));
    outportb(ATA_PRIMARY_IO + 2, 1);
    outportb(ATA_PRIMARY_IO + 3, (uint8_t)(lba & 0xFF));
    outportb(ATA_PRIMARY_IO + 4, (uint8_t)((lba >> 8) & 0xFF));
    outportb(ATA_PRIMARY_IO + 5, (uint8_t)((lba >> 16) & 0xFF));
    outportb(ATA_PRIMARY_IO + 7, ATA_CMD_WRITE);

    ata_wait();

    for (int i = 0; i < 256; i++) {
        outportw(ATA_PRIMARY_IO, ((uint16_t *)buffer)[i]);
    }
}

void ata_read_sectors(uint32_t lba, uint8_t *buffer, uint32_t sector_count) {
    for (uint32_t i = 0; i < sector_count; i++) {
        ata_read_sector(lba + i, buffer + (i * 512));
    }
}

void ata_write_sectors(uint32_t lba, uint8_t *buffer, uint32_t sector_count) {
    for (uint32_t i = 0; i < sector_count; i++) {
        ata_write_sector(lba + i, buffer + (i * 512));
    }
}


void create_file(const char *name, uint8_t *data, uint32_t size) {
    if (file_count >= MAX_FILES) {
        print("File table is full.\n");
        return;
    }

    File *file = &file_table[file_count++];
    strncpy(file->name, name, 32);
    file->size = size;
    file->start_lba = 1 + file_count;

    uint32_t sector_count = (size + 511) / 512;
    ata_write_sectors(file->start_lba, data, sector_count);

    print("File created: ");
    print(name);
    print("\n");
}

void read_file(const char *name, uint8_t *buffer) {
    for (uint32_t i = 0; i < file_count; i++) {
        if (strncmp(file_table[i].name, name, 32) == 0) {
            uint32_t sector_count = (file_table[i].size + 511) / 512;
            ata_read_sectors(file_table[i].start_lba, buffer, sector_count);
            print("File read: ");
            print(name);
            print("\n");
            return;
        }
    }

    print("File not found: ");
    print(name);
    print("\n");
}