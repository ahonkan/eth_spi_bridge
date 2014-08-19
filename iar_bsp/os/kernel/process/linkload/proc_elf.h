/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2013
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/
/*************************************************************************
*
*   FILE NAME
*
*       proc_elf.h
*
*   DESCRIPTION
*
*       Generic ELF format information.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       nucleus.h
*
*************************************************************************/

#ifndef PROC_ELF_H
#define PROC_ELF_H

#include    "nucleus.h"

/* ELF base types - 32-bit. */
typedef     UINT32          Elf32_Addr;
typedef     UINT16          Elf32_Half;
typedef     UINT32          Elf32_Off;
typedef     INT32           Elf32_Sword;
typedef     UINT32          Elf32_Word;

/* Size of ELF identifier field in the ELF file header. */
#define     EI_NIDENT       16

/* ELF file header */
typedef struct
{
    unsigned char           e_ident[EI_NIDENT];
    Elf32_Half              e_type;
    Elf32_Half              e_machine;
    Elf32_Word              e_version;
    Elf32_Addr              e_entry;
    Elf32_Off               e_phoff;
    Elf32_Off               e_shoff;
    Elf32_Word              e_flags;
    Elf32_Half              e_ehsize;
    Elf32_Half              e_phentsize;
    Elf32_Half              e_phnum;
    Elf32_Half              e_shentsize;
    Elf32_Half              e_shnum;
    Elf32_Half              e_shstrndx;

} Elf32_Ehdr;

/* e_ident */
#define     ET_NONE         0
#define     ET_REL          1               /* Re-locatable file         */
#define     ET_EXEC         2               /* Executable file           */
#define     ET_DYN          3               /* Shared object file        */
#define     ET_CORE         4               /* Core file                 */
#define     ET_LOOS         0xfe00          /* Operating system-specific */
#define     ET_HIOS         0xfeff          /* Operating system-specific */
#define     ET_LOPROC       0xff00          /* Processor-specific        */
#define     ET_HIPROC       0xffff          /* Processor-specific        */

/* e_machine */
#define     EM_ARM          40              /* ARM/Thumb Architecture    */

/* e_version */
#define     EV_CURRENT      1               /* Current version           */

/* e_ident[] Identification Indexes */
#define     EI_MAG0         0               /* File identification       */
#define     EI_MAG1         1               /* File identification       */
#define     EI_MAG2         2               /* File identification       */
#define     EI_MAG3         3               /* File identification       */
#define     EI_CLASS        4               /* File class                */
#define     EI_DATA         5               /* Data encoding             */
#define     EI_VERSION      6               /* File version              */
#define     EI_OSABI        7               /* Operating system/ABI identification */
#define     EI_ABIVERSION   8               /* ABI version               */
#define     EI_PAD          9               /* Start of padding bytes    */
#define     EI_NIDENT       16              /* Size of e_ident[]         */

/* EI_MAG0 to EI_MAG3 - A file's first 4 bytes hold amagic number, identifying the file as an ELF object file */
#define     ELFMAG0         0x7f            /* e_ident[EI_MAG0]          */
#define     ELFMAG1         'E'             /* e_ident[EI_MAG1]          */
#define     ELFMAG2         'L'             /* e_ident[EI_MAG2]          */
#define     ELFMAG3         'F'             /* e_ident[EI_MAG3]          */

/* EI_CLASS - The next byte, e_ident[EI_CLASS], identifies the file's class, or capacity. */
#define     ELFCLASSNONE    0               /* Invalid class             */
#define     ELFCLASS32      1               /* 32-bit objects            */
#define     ELFCLASS64      2               /* 64-bit objects            */
           
/* EI_DATA - Byte e_ident[EI_DATA] specifies the data encoding of the processor-specific data in the object
file. The following encodings are currently defined. */
#define     ELFDATANONE     0               /* Invalid data encoding     */
#define     ELFDATA2LSB     1               /* See Data encodings, below */
#define     ELFDATA2MSB     2               /* See Data encodings, below */

/* EI_OSABI - We do not define a Nucleus specific OS ABI */
#define     ELFOSABI_NONE   0

/* ELF section header. */
typedef struct
{
    Elf32_Word              sh_name;
    Elf32_Word              sh_type;
    Elf32_Word              sh_flags;
    Elf32_Addr              sh_addr;
    Elf32_Off               sh_offset;
    Elf32_Word              sh_size;
    Elf32_Word              sh_link;
    Elf32_Word              sh_info;
    Elf32_Word              sh_addralign;
    Elf32_Word              sh_entsize;

} Elf32_Shdr;

/* sh_type */
#define     SHT_NULL                0
#define     SHT_PROGBITS            1
#define     SHT_SYMTAB              2
#define     SHT_STRTAB              3
#define     SHT_RELA                4
#define     SHT_HASH                5
#define     SHT_DYNAMIC             6
#define     SHT_NOTE                7                                              
#define     SHT_NOBITS              8
#define     SHT_REL                 9
#define     SHT_SHLIB               10
#define     SHT_DYNSYM              11
#define     SHT_INIT_ARRAY          14
#define     SHT_FINI_ARRAY          15
#define     SHT_PREINIT_ARRAY       16
#define     SHT_GROUP               17
#define     SHT_SYMTAB_SHNDX        18
#define     SHT_LOOS                0x60000000
#define     SHT_HIOS                0x6fffffff
#define     SHT_LOPROC              0x70000000
#define     SHT_HIPROC              0x7fffffff
#define     SHT_LOUSER              0x80000000
#define     SHT_HIUSER              0xffffffff 

/* sh_flags */
#define     SHF_WRITE       0x1
#define     SHF_ALLOC       0x2
#define     SHF_EXECINSTR   0x4
#define     SHF_MASKPROC    0xf0000000

/* Relocation entry (without addend) */
typedef struct
{
    Elf32_Addr              r_offset;
    Elf32_Word              r_info;

} Elf32_Rel;

/* Relocation entry with addend */
typedef struct
{
    Elf32_Addr              r_offset;
    Elf32_Word              r_info;
    Elf32_Sword             r_addend;

} Elf32_Rela;

/* Macros to extract information from 'r_info' field of relocation entries */
#define     ELF32_R_SYM(i)  ((i)>>8)
#define     ELF32_R_TYPE(i) ((unsigned char)(i))

/* Symbol table entry */
typedef struct
{
    Elf32_Word              st_name;
    Elf32_Addr              st_value;
    Elf32_Word              st_size;
    unsigned char           st_info;
    unsigned char           st_other;
    Elf32_Half              st_shndx;

} Elf32_Sym;

#endif  /* !PROC_ELF_H */
