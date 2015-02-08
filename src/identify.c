/******************************************************************
    Copyright (C) 2009  Henrik Carlqvist

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
******************************************************************/
#ifdef __linux__
#include <linux/hdreg.h>
#include <linux/fd.h>
#endif
/* Ugly fix for compability with both older libc and newer kernels */
#include <sys/mount.h>
#ifdef __linux__
#ifndef BLKGETSIZE
#include <linux/fs.h>
#endif
#endif
/* end of ugly fix */
#include <sys/ioctl.h>

#ifdef __FreeBSD__
#include <sys/disk.h>
#endif

#include "br.h"
#include "fat12.h"
#include "fat16.h"
#include "fat16fd.h"
#include "fat32.h"
#include "fat32nt.h"
#include "fat32pe.h"
#include "fat32fd.h"
#include "nls.h"
#include "ntfs.h"
#include "identify.h"

#ifdef __APPLE__
#include <sys/disk.h>
#include <sys/stat.h>
#endif

/* This one should be in stdio.h, however it seems to be missing. Declared here
   to avoid warnings... */
#ifndef fileno
int fileno( FILE *stream);
#endif

/* Returns TRUE if file is a device, otherwise FALSE */
static int is_disk_device(FILE *fp);

/* Returns TRUE if file is a partition device, otherwise FALSE */
static int is_partition(FILE *fp);

static int is_disk_device(FILE *fp)
{
#if defined(BLKGETSIZE) || defined(HDIO_REQ) || defined(DIOCFWSECTORS) || defined(DKIOCGETBLOCKSIZE)
   int iRes1;
   int iRes2;
   long lSectors;
   int iFd = fileno(fp);

#if defined(BLKGETSIZE)
   struct hd_geometry sGeometry;
   iRes1 = ioctl(iFd, BLKGETSIZE, &lSectors);
#ifdef HDIO_REQ
   iRes2 = ioctl(iFd, HDIO_REQ, &sGeometry);
#else
   iRes2 = ioctl(iFd, HDIO_GETGEO, &sGeometry);
#endif
#endif

#if defined(DKIOCGETBLOCKSIZE)
   iRes1 = ioctl(iFd, DKIOCGETBLOCKSIZE, &lSectors);
   iRes2 = 0;
#endif

#ifdef DIOCGFWSECTORS
   uint start_sector = 0;
   iRes1 = ioctl(iFd, DIOCGFWSECTORS, &start_sector);
   iRes2 = 0;
   lSectors = 0;
#endif

   return !(iRes1 && iRes2);
#else
   return 0;
#endif
} /* is_device */

int is_floppy(FILE *fp)
{
#ifdef DIOCGETBLOCKCOUNT
    return (partition_number_of_heads(fp) == 2);
#endif

#if defined(FDGETPRM) || defined(DIOCGFWHEADS)
#ifdef FDGETPRM
   struct floppy_struct sFloppy;
   
   return !ioctl(fileno(fp), FDGETPRM, &sFloppy);
#endif

#ifdef DIOCGFWHEADS
   int iFd = fileno(fp);
   unsigned heads;
   int iRes1 = ioctl(iFd, DIOCGFWHEADS, &heads);
   if (! iRes1 )
      return heads == 2;

   return 0;
#endif
#else
   return 0;
#endif
} /* is_floppy */

static int is_partition(FILE *fp)
{
#if defined(BLKGETSIZE) || defined(HDIO_REQ) || defined(DIOCGMEDIASIZE) || defined(DKIOCISFORMATTED)
   int iRes1;
   int iRes2;
   long lSectors;
   int iFd = fileno(fp);

#ifdef BLKGETSIZE
   struct hd_geometry sGeometry;
   iRes1 = ioctl(iFd, BLKGETSIZE, &lSectors);
#ifdef HDIO_REQ
   iRes2 = ioctl(iFd, HDIO_REQ, &sGeometry);
#else
   iRes2 = ioctl(iFd, HDIO_GETGEO, &sGeometry);
#endif

   return (! (iRes1 && iRes2)) && (sGeometry.start);
#endif

#ifdef DKIOCISFORMATTED
   uint32_t formatted = 0;
   iRes1 = ioctl(iFd, DKIOCISFORMATTED, &formatted);
   iRes2 = 0;
   lSectors = 0;

   return (formatted != 0);
#endif

#ifdef DIOCGMEDIASIZE
   off_t bytes;
   unsigned int start_sector;
   iRes1 = ioctl(iFd, DIOCGMEDIASIZE, &bytes);
   iRes2 = ioctl(iFd, DIOCGFWSECTORS, &start_sector);
   lSectors = 0;

   return (! (iRes1 && iRes2));
#endif
#else
   return 0;
#endif
} /* is_partition */

#if defined(DIOCGETBLOCKSIZE)
static void make_partd(prt_t *partn, off_t offset, off_t reloff, void *prt)
{
    unsigned char *p = prt;
    prt_t tmp;
    off_t off;

    tmp.shead = partn->shead;
    tmp.ssect = partn->ssect;
    tmp.scyl = (partn->scyl > 1023)? 1023: partn->scyl;
    tmp.ehead = partn->ehead;
    tmp.esect = partn->esect;
    tmp.ecyl = (partn->ecyl > 1023)? 1023: partn->ecyl;
    if (!PRT_check_chs(partn) && PRT_check_chs(&tmp)) {
        partn->shead = tmp.shead;
        partn->ssect = tmp.ssect;
        partn->scyl = tmp.scyl;
        partn->ehead = tmp.ehead;
        partn->esect = tmp.esect;
        partn->ecyl = tmp.ecyl;
        printf("Cylinder values are modified to fit in CHS.\n");
    }
    if ((partn->id == DOSPTYP_EXTEND) || (partn->id == DOSPTYP_EXTENDL))
        off = reloff;
        else
            off = offset;
            
            if (PRT_check_chs(partn)) {
                *p++ = partn->flag & 0xFF;
                
                *p++ = partn->shead & 0xFF;
                *p++ = (partn->ssect & 0x3F) | ((partn->scyl & 0x300) >> 2);
                *p++ = partn->scyl & 0xFF;
                
                *p++ = partn->id & 0xFF;
                
                *p++ = partn->ehead & 0xFF;
                *p++ = (partn->esect & 0x3F) | ((partn->ecyl & 0x300) >> 2);
                *p++ = partn->ecyl & 0xFF;
            } else {
                /* should this really keep flag, id and set others to 0xff? */
                *p++ = partn->flag & 0xFF;
                *p++ = 0xFF;
                *p++ = 0xFF;
                *p++ = 0xFF;
                *p++ = partn->id & 0xFF;
                *p++ = 0xFF;
                *p++ = 0xFF;
                *p++ = 0xFF;
                printf("Warning CHS values out of bounds only saving LBA values\n");
            }
    
    putlong(p, partn->bs - off);
    putlong(p+4, partn->ns);
}

static void get_part(disk, prt, offset, reloff, partn, pn)
disk_t *disk;
void *prt;
off_t offset;
off_t reloff;
prt_t *partn;
int pn;
{
    unsigned char *p = prt;
    off_t off;

    partn->flag = *p++;
    partn->shead = *p++;
    
    partn->ssect = (*p) & 0x3F;
    partn->scyl = ((*p << 2) & 0xFF00) | (*(p+1));
    p += 2;
    
    partn->id = *p++;
    partn->ehead = *p++;
    partn->esect = (*p) & 0x3F;
    partn->ecyl = ((*p << 2) & 0xFF00) | (*(p+1));
    p += 2;
    
    if ((partn->id == DOSPTYP_EXTEND) || (partn->id == DOSPTYP_EXTENDL))
        off = reloff;
        else
            off = offset;
            
            partn->bs = getlong(p) + off;
            partn->ns = getlong(p+4);
            
            
        /* Zero out entry if not used */
            if (partn->id == DOSPTYP_UNUSED ) {
                memset(partn, 0, sizeof(*partn));
            }
}
#endif

unsigned long partition_start_sector(FILE *fp)
{
#if defined(DIOCGETBLOCKSIZE)
   int iRes1;
   int iRes2;
   int start = 0;
   int end = 0;
   long lSectors;
   int iFd = fileno(fp);
   struct stat st;
   int heads = 4;
   int spt = 63;
   int cylinders = 0;
   int spc = 0;

   fstat(iFd, &st);
    
   if (!S_ISREG(st.st_mode) || S_ISBLK(st.st_mode))
   {
        iRes1 = ioctl(iFd, DKIOCGETBLOCKCOUNT, &blkcnt);
        iRes2 = ioctl(iFd, DKIOCGETBLOCKSIZE, &secsize);
   }

   if (secsize == 0)
   {
        secsize = 512;
        blkcnt = st.st_size / secsize;
   }
    
   cylinders = (int)(blkcnt / heads / spt);

   while ((cylinders > 1024) && (heads < 256))
   {
        heads *= 2;
        cylinders /= 2;
   }
    
   if (heads == 256)
   {
        heads = 255;
        cylinders = (int)(blkcnt / heads / spt);
   }

   if(start <= spt)
   {
        /* Figure out "real" starting CHS values */
        cylinders = (start / spc);
        start -= (cylinders * spc);
        heads = (start / spt);
        start -= (heads * spt);
        blkcnt = (start + 1);
   } else {
        cylinders = 1023;
        heads = 254;
        blkcnt =  63;
   }

   spc = spt * heads;
   start += cylinders * spc;
   start += heads * spt;
   start += sect - 1;

   return (unsigned long)start;
#endif

#if defined(BLKGETSIZE) || defined(HDIO_REQ) || defined(DIOCGFWSECTORS)
   int iRes1;
   int iRes2;
   long lSectors;
   int iFd = fileno(fp);

#ifdef BLKGETSIZE
   struct hd_geometry sGeometry;
   iRes1 = ioctl(iFd, BLKGETSIZE, &lSectors);
#ifdef HDIO_REQ
   iRes2 = ioctl(iFd, HDIO_REQ, &sGeometry);
#else
   iRes2 = ioctl(iFd, HDIO_GETGEO, &sGeometry);
#endif
   if(! (iRes1 && iRes2) )
      return sGeometry.start;
   else
      return 0L;
#endif

#ifdef DIOCGFWSECTORS
   uint start_sector = 0;
   iRes1 = ioctl(iFd, DIOCGFWSECTORS, &start_sector);
   iRes2 = 0;
   lSectors = 0;
   
   if( ! iRes1 )
      return start_sector;

   return 0L;
#endif
#else
   return 0;
#endif
} /* partition_start_sector */

unsigned short partition_number_of_heads(FILE *fp)
{
#if defined(DKIOCGETBLOCKCOUNT)
   int iRes1 = 0;
   int iRes2 = 0;
   uint64_t blkcnt = 0;
   uint32_t secsize = 0;
   int iFd = fileno(fp);
   struct stat st;
   int heads = 4;
   int spt = 63;
   int cylinders = 0;
    
   fstat(iFd, &st);

   if (!S_ISREG(st.st_mode) || S_ISBLK(st.st_mode))
   {
       iRes1 = ioctl(iFd, DKIOCGETBLOCKCOUNT, &blkcnt);
       iRes2 = ioctl(iFd, DKIOCGETBLOCKSIZE, &secsize);
   }

   if (secsize == 0)
   {
       secsize = 512;
       blkcnt = st.st_size / secsize;
   }

   cylinders = (int)(blkcnt / heads / spt);
 
   while ((cylinders > 1024) && (heads < 256))
   {
        heads *= 2;
        cylinders /= 2;
   }
    
   if (heads == 256)
   {
        heads = 255;
        cylinders = (int)(blkcnt / heads / spt);
   }

   if (!(iRes1 && iRes2))
   {
       return (unsigned short)heads;
   }

   return 0;
#endif

#if defined(BLKGETSIZE) || defined(HDIO_REQ) || defined(DIOCGFWHEADS)
   int iRes1;
   int iRes2;
   long lSectors;
   int iFd = fileno(fp);
   
#ifdef BLKGETSIZE
   struct hd_geometry sGeometry;
   iRes1 = ioctl(iFd, BLKGETSIZE, &lSectors);
#ifdef HDIO_REQ
   iRes2 = ioctl(iFd, HDIO_REQ, &sGeometry);
#else
   iRes2 = ioctl(iFd, HDIO_GETGEO, &sGeometry);
#endif
   if(! (iRes1 && iRes2) )
      return (unsigned short) sGeometry.heads;

   return 0;
#endif

#ifdef DIOCGFWHEADS
   unsigned heads;
   iRes1 = ioctl(iFd, DIOCGFWHEADS, &heads);
   iRes2 = 0;
   lSectors = 0;

   if (! iRes1 )
      return (unsigned short) heads;

   return 0;
#endif
#else
   return 0;
#endif
} /* partition_number_of_heads */

int sanity_check(FILE *fp, const char *szPath, int iBr, int bPrintMessages)
{
   int bIsDiskDevice = is_disk_device(fp);
   int bIsFloppy = is_floppy(fp);
   int bIsPartition = is_partition(fp);
   switch(iBr)
   {
      case MBR_WIN7:
      case MBR_VISTA:
      case MBR_2000:
      case MBR_95B:
      case MBR_DOS:
      case MBR_SYSLINUX:
      case MBR_GPT_SYSLINUX:
      case MBR_ZERO:
      {
	 if(!bIsDiskDevice)
	 {
	    if(bPrintMessages)
	    {
	       printf(_("%s does not seem to be a disk device,\n"), szPath);
	       printf(
		  _("use the switch -f to force writing of a master boot record\n"));
	    }
	    return 0;
	 }
	 if(bIsFloppy)
	 {
	    if(bPrintMessages)
	    {
	       printf(_("%s seems to be a floppy disk device,\n"), szPath);
	       printf(
		  _("use the switch -f to force writing of a master boot record\n"));
	    }
	    return 0;
	 }
	 if(bIsPartition)
	 {
	    if(bPrintMessages)
	    {
	       printf(_("%s seems to be a disk partition device,\n"), szPath);
	       printf(
		  _("use the switch -f to force writing of a master boot record\n"));
	    }
	    return 0;
	 }
      }
      break;
      case FAT12W7_BR:
      case FAT12_BR:
      {
	 if(!bIsFloppy)
	 {
	    if(bPrintMessages)
	    {
	       printf(_("%s does not seem to be a floppy disk device,\n"),
		      szPath);
	       printf(
		  _("use the switch -f to force writing of a FAT12 boot record\n"));
	    }
	    return 0;
	 }
	 if(!is_fat_12_fs(fp))
	 {
	    if(bPrintMessages)
	    {
	       printf(_("%s does not seem to have a FAT12 file system,\n"),
		      szPath);
	       printf(
		  _("use the switch -f to force writing of a FAT12 boot record\n"));
	    }
	    return 0;
	 }
      }
      break;
      case FAT16_BR:
      case FAT16FD_BR:
      {
	 if( ! bIsPartition )
	 {
	    if(bPrintMessages)
	    {
	       printf(_("%s does not seem to be a disk partition device,\n"),
		      szPath);
	       printf(
		  _("use the switch -f to force writing of a FAT16 boot record\n"));
	    }
	    return 0;
	 }
	 if( ! is_fat_16_fs(fp))
	 {
	    if(bPrintMessages)
	    {
	       printf(_("%s does not seem to have a FAT16 file system,\n"),
		      szPath);
	       printf(
		  _("use the switch -f to force writing of a FAT16 boot record\n"));
	    }
	    return 0;
	 }
      }
      break;
      case FAT32_BR:
      case FAT32NT_BR:
      case FAT32PE_BR:
      case FAT32FD_BR:
      case FAT32W7_BR:
      {
	 if( ! bIsPartition )
	 {
	    if(bPrintMessages)
	    {
	       printf(_("%s does not seem to be a disk partition device,\n"),
		      szPath);
	       printf(
		  _("use the switch -f to force writing of a FAT32 boot record\n"));
	    }
	    return 0;
	 }
	 if(!is_fat_32_fs(fp))
	 {
	    if(bPrintMessages)
	    {
	       printf(_("%s does not seem to have a FAT32 file system,\n"),
		      szPath);
	       printf(
		  _("use the switch -f to force writing of a FAT32 boot record\n"));
	    }
	    return 0;
	 }
      }
      break;
      case NTFS_BR:
      {
	 if (!bIsPartition)
	 {
	    if(bPrintMessages)
	    {
	       printf(_("%s does not seem to be a disk partition device,\n"),
		      szPath);
	       printf(
		  _("use the switch -f to force writing of a NTFS boot record\n"));
	    }
	    return 0;
	 }
	 if( ! is_ntfs_fs(fp))
	 {
	    if(bPrintMessages)
	    {
	       printf(_("%s does not seem to have a NTFS file system,\n"),
		      szPath);
	       printf(
		  _("use the switch -f to force writing of a NTFS boot record\n"));
	    }
	    return 0;
	 }	 
      }
      break;
      default:
      {
	 if(bPrintMessages)
	 {
	    printf(_("Whoops, internal error, unknown boot record\n"));
	 }
	 return 0;
      }
      break;
   }
   return 1;
} /* sanity_check */

void diagnose(FILE *fp, const char *szPath)
{
   if(is_fat_12_fs(fp))
      printf(_("%s has a FAT12 file system.\n"), szPath);
   if(is_fat_16_fs(fp))
      printf(_("%s has a FAT16 file system.\n"), szPath);
   if(is_fat_32_fs(fp))
      printf(_("%s has a FAT32 file system.\n"), szPath);
   if(is_ntfs_fs(fp))
      printf(_("%s has a NTFS file system.\n"), szPath);
   if(is_br(fp))
      printf(_("%s has an x86 boot sector,\n"), szPath);
   else
   {
      printf(_("%s has no x86 boot sector\n"), szPath);
      return;
   }
   if(entire_ntfs_br_matches(fp))
   {
      printf(
	 _("it is exactly the kind of NTFS boot record this program\n"));
      printf(
	 _("would create with the switch -n on a NTFS partition.\n"));      
   }
   else if(entire_fat_12_w7_br_matches(fp))
   {
       printf(
              _("it is exactly the kind of FAT12 Win 7 boot record this program\n"));
       printf(
              _("would create with the switch -0 on a floppy.\n"));
   }
   else if(entire_fat_12_br_matches(fp))
   {
      printf(
	 _("it is exactly the kind of FAT12 boot record this program\n"));
      printf(
	 _("would create with the switch -1 on a floppy.\n"));
   }
   else if(is_fat_16_br(fp) || is_fat_32_br(fp))
   {
      if(entire_fat_16_br_matches(fp))
      {
	 printf(
	    _("it is exactly the kind of FAT16 DOS boot record this program\n"));
	 printf(
	    _("would create with the switch -6 on a FAT16 partition.\n"));
      }
      else if(entire_fat_16_fd_br_matches(fp))
      {
	 printf(
	  _("it is exactly the kind of FAT16 FreeDOS boot record this program\n"));
	 printf(
	    _("would create with the switch -5 on a FAT16 partition.\n"));
      }
      else if(entire_fat_32_br_matches(fp))
      {
	 printf(
	  _("it is exactly the kind of FAT32 DOS boot record this program\n"));
	 printf(
	    _("would create with the switch -3 on a FAT32 partition.\n"));
      }
      else if(entire_fat_32_nt_br_matches(fp))
      {
	 printf(
	   _("it is exactly the kind of FAT32 NT boot record this program\n"));
	 printf(
	    _("would create with the switch -2 on a FAT32 partition.\n"));
      }
      else if(entire_fat_32_pe_br_matches(fp))
      {
	 printf(
	   _("it is exactly the kind of FAT32 NT boot record this program\n"));
	 printf(
	    _("would create with the switch -e on a FAT32 partition.\n"));
      }
      else if(entire_fat_32_w7_br_matches(fp))
      {
     printf(
       _("it is exactly the kind of FAT32 Windows 7 boot record this program\n"));
     printf(
        _("would create with the switch -7 on a FAT32 partition.\n"));
      }
      else if(entire_fat_32_fd_br_matches(fp))
      {
	 printf(
	   _("it is exactly the kind of FAT32 FreeDOS boot record this program\n"));
	 printf(
	    _("would create with the switch -4 on a FAT32 partition.\n"));
      }
      else
      {
	 printf(
	    _("it seems to be a FAT16 or FAT32 boot record, but it\n"));
	 printf(
	    _("differs from what this program would create with the\n"));
	 printf(_("switch -6, -2, -e or -3 on a FAT16 or FAT32 partition.\n"));
      }
   } 
   else if(is_lilo_br(fp))
   {
      printf(_("it seems to be a LILO boot record, please use lilo to\n"));
      printf(_("create such boot records.\n"));
   }
   else if(is_dos_mbr(fp))
   {
	 printf(
	    _("it is a Microsoft DOS/NT/95A master boot record, like the one this\n"));
	 printf(
	    _("program creates with the switch -d on a hard disk device.\n"));
   }
   else if(is_dos_f2_mbr(fp))
   {
	 printf(
	    _("it is a Microsoft DOS/NT/95A master boot record with the undocumented\n"));
	 printf(
	    _("F2 instruction. You will get equal functionality with the MBR this\n"));
	 printf(
	    _("program creates with the switch -d on a hard disk device.\n"));
   }
   else if(is_95b_mbr(fp))
   {
	 printf(
	    _("it is a Microsoft 95B/98/98SE/ME master boot record, like the one this\n"));
	 printf(
	    _("program creates with the switch -9 on a hard disk device.\n"));
   }
   else if(is_2000_mbr(fp))
   {
	 printf(
	    _("it is a Microsoft 2000/XP/2003 master boot record, like the one this\n"));
	 printf(
	    _("program creates with the switch -m on a hard disk device.\n"));
   }
   else if(is_vista_mbr(fp))
   {
         printf(
            _("it is a Microsoft Vista master boot record, like the one this\n"));
         printf(
            _("program creates with the switch -i on a hard disk device.\n"));
   }
   else if(is_win7_mbr(fp))
   {
         printf(
            _("it is a Microsoft 7 master boot record, like the one this\n"));
         printf(
            _("program creates with the switch -7 on a hard disk device.\n"));
   }
   else if(is_syslinux_mbr(fp))
   {
	 printf(
	    _("it is a public domain syslinux master boot record, like the one this\n"));
	 printf(
	    _("program creates with the switch -s on a hard disk device.\n"));
   }
   else if(is_syslinux_gpt_mbr(fp))
   {
	 printf(
	    _("it is a GPL syslinux GPT master boot record, like the one this\n"));
	 printf(
	    _("program creates with the switch -t on a hard disk device.\n"));
   }
   else if(is_zero_mbr(fp))
   {
	 printf(
	    _("it is a zeroed non-bootable master boot record, like the one this\n"));
	 printf(
	    _("program creates with the switch -z on a hard disk device.\n"));
   }
   else
      printf(_("it is an unknown boot record\n"));
} /* diagnose */

int smart_select(FILE *fp)
{
   int i;

   for(i=AUTO_BR+1; i<NUMBER_OF_RECORD_TYPES; i++)
      if(sanity_check(fp, "", i, 0))
	 return i;
   return NO_WRITING;
} /* smart_select */
