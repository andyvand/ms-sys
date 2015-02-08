#ifndef IDENTIFY_H
#define IDENTIFY_H

#include <stdio.h>

enum
{
    NO_WRITING = 0,
    AUTO_BR,
    MBR_DOS,
    FAT12_BR,
    FAT16_BR,
    FAT32_BR,
    FAT32NT_BR,
    MBR_95B,
    MBR_2000,
    MBR_VISTA,
    MBR_WIN7,
    MBR_SYSLINUX,
    MBR_ZERO,
    FAT16FD_BR,
    FAT32FD_BR,
    NTFS_BR,
    MBR_GPT_SYSLINUX,
    FAT32PE_BR,
    MBR_WIN7FAT,
    FAT32W7_BR,
    FAT12W7_BR,
    NUMBER_OF_RECORD_TYPES
};

/* Returns the number of sectors on disk before partition */
unsigned long partition_start_sector(FILE *fp);
/* Returns the number of heads for the drive of a partition */
unsigned short partition_number_of_heads(FILE *fp);

/* Returns TRUE if writing a boot record of type iBr seems like a good idea,
   otherwise FALSE */
int sanity_check(FILE *fp, const char *szPath, int iBr, int bPrintMessages);

/* Prints some information about a device */
void diagnose(FILE *fp, const char *szPath);

/* Does a smart automatic selection of which boot record to write, returns
   the kind of boot record as defined above */
int smart_select(FILE *fp);

/* Is the device a floppy device? */
int is_floppy(FILE *fp);

#endif
