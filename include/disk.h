#ifndef DISK_H
#define DISK_H
#define MAX_FILES 128

#include <stdint.h>

typedef struct {
    char name[32];
    uint32_t size;
    uint32_t start_lba;
} File;

extern File file_table[MAX_FILES];
extern uint32_t file_count;

void ata_wait();
void ata_read_sector(uint32_t lba, uint8_t *buffer);
void ata_write_sector(uint32_t lba, uint8_t *buffer);
void create_file(const char *name, uint8_t *data, uint32_t size);
void read_file(const char *name, uint8_t *buffer);

#endif