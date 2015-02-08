#ifndef BR_H
#define BR_H

#include <stdio.h>

/* returns TRUE if the file has a boot record, otherwise FALSE.
   The file position will change when this function is called! */
int is_br(FILE *fp);

/* returns TRUE if the file has a LILO boot record, otherwise FALSE.
   The file position will change when this function is called! */
int is_lilo_br(FILE *fp);

/* returns TRUE if the file has a Microsoft dos master boot record, otherwise
   FALSE.The file position will change when this function is called! */
int is_dos_mbr(FILE *fp);

/* returns TRUE if the file has a Microsoft dos master boot record with the
   undocumented F2 instruction, otherwise FALSE. The file position will change
   when this function is called! */
int is_dos_f2_mbr(FILE *fp);

/* returns TRUE if the file has a Microsoft 95b master boot record, otherwise
   FALSE.The file position will change when this function is called! */
int is_95b_mbr(FILE *fp);

/* returns TRUE if the file has a Microsoft 2000 master boot record, otherwise
   FALSE.The file position will change when this function is called! */
int is_2000_mbr(FILE *fp);

/* returns TRUE if the file has a Microsoft Vista master boot record, otherwise
   FALSE.The file position will change when this function is called! */
int is_vista_mbr(FILE *fp);

/* returns TRUE if the file has a Microsoft 7 master boot record, otherwise
   FALSE.The file position will change when this function is called! */
int is_win7_mbr(FILE *fp);

/* returns TRUE if the file has a Microsoft 7 FAT32 master boot record, otherwise
 FALSE.The file position will change when this function is called! */
int is_win7fat_mbr(FILE *fp);

/* returns TRUE if the file has a syslinux master boot record, otherwise
   FALSE.The file position will change when this function is called! */
int is_syslinux_mbr(FILE *fp);

/* returns TRUE if the file has a syslinux GPT master boot record, otherwise
   FALSE.The file position will change when this function is called! */
int is_syslinux_gpt_mbr(FILE *fp);

/* returns TRUE if the file has a zeroed master boot record, otherwise
   FALSE.The file position will change when this function is called! */
int is_zero_mbr(FILE *fp);

/* Writes a dos master boot record to a file, returns TRUE on success, otherwise
   FALSE */
int write_dos_mbr(FILE *fp);

/* Writes a 95b master boot record to a file, returns TRUE on success, otherwise
   FALSE */
int write_95b_mbr(FILE *fp);

/* Writes a 2000 master boot record to a file, returns TRUE on success, otherwise
   FALSE */
int write_2000_mbr(FILE *fp);

/* Writes a Vista master boot record to a file, returns TRUE on success, otherwise
   FALSE */
int write_vista_mbr(FILE *fp);

/* Writes a 7 master boot record to a file, returns TRUE on success, otherwise
   FALSE */
int write_win7_mbr(FILE *fp);

/* Writes a 7 FAT master boot record to a file, returns TRUE on success, otherwise
 FALSE */
int write_win7fat_mbr(FILE *fp);

/* Writes a syslinux master boot record to a file, returns TRUE on success, otherwise
   FALSE */
int write_syslinux_mbr(FILE *fp);

/* Writes a syslinux GPT master boot record to a file, returns TRUE on success, otherwise
   FALSE */
int write_syslinux_gpt_mbr(FILE *fp);

/* Writes an empty (zeroed) master boot record to a file, returns TRUE on success, otherwise
   FALSE */
int write_zero_mbr(FILE *fp);

/* returns TRUE if the file has an exact match of the FAT32 Win 7 boot record
 this program would create, otherwise FALSE.
 The file position will change when this function is called! */
int entire_fat_32_w7_br_matches(FILE *fp);

/* returns TRUE if the file has an exact match of the FAT12 Win 7 boot record
 this program would create, otherwise FALSE.
 The file position will change when this function is called! */
int entire_fat_12_w7_br_matches(FILE *fp);

/* Write a Win 7 FAT32 partition boot record */
int write_fat_32_w7_br(FILE *fp, int bKeepLabel);

/* Write a Win 7 FAT12 partition boot record */
int write_fat_12_w7_br(FILE *fp, int bKeepLabel);

/* Write a Win 7 FAT master boot record */
int write_win7_fat_mbr(FILE *fp);

#endif
