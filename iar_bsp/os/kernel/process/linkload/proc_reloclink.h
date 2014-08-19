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
*       proc_reloclink.h
*
*   COMPONENT
*
*       Nucleus Processes - Linker / Loader
*
*   DESCRIPTION
*
*       Linker / Loader component internal API.
*
*   DATA STRUCTURES
*
*       PROC_NUPROCINFO
*       PROC_ELF_DECODE_INFO
*
*************************************************************************/

#ifndef PROC_RELOCLINK_H
#define PROC_RELOCLINK_H

#include    "proc_elf.h"


/* ELF region information */
typedef struct proc_nuprocinfo
{
    UINT32          nu_process_version;
    Elf32_Addr      text_start;
    Elf32_Word      text_end;
    Elf32_Addr      rodata_start;
    Elf32_Word      rodata_end;
    Elf32_Addr      wrdata_start;
    Elf32_Word      wrdata_end;
    Elf32_Addr      load_info_start;
    Elf32_Word      load_info_end;
    Elf32_Addr      initdata_start;
    Elf32_Addr      data_start;
    Elf32_Word      data_size;
    Elf32_Addr      bss_start;
    Elf32_Word      bss_size;

} PROC_NUPROCINFO;

/* ELF decoding information */
typedef struct proc_elf_decode_info
{
    Elf32_Ehdr      elf_header;
    UINT8           *section_headers_start;
    char            *shstrtab;

    PROC_NUPROCINFO nuprocinfo;

    Elf32_Shdr      *dynsym;
    Elf32_Shdr      *dynstr;
    Elf32_Shdr      *rel_plt;
    Elf32_Shdr      *rel_dyn;
    Elf32_Shdr      *nusymtab;
    Elf32_Shdr      *nuksymtab;

    UINT8           *dynsym_addr;
    UINT8           *dynstr_addr;

} PROC_ELF_DECODE_INFO;

/* ELF API */
CHAR *  PROC_ELF_Get_Dynamic_Symbol_Name(PROC_ELF_DECODE_INFO *elf_decode_info, int index);
STATUS  PROC_ELF_File_Load(PROC_CB *process, INT fd, VOID **load_addr, VOID **entry_addr,
		                   VOID **root_stack, UINT32 root_stack_size,
		                   UINT32 heap_size, UINT32 page_size,
		                   NU_SYMBOL_ENTRY **symbol_table, NU_SYMBOL_ENTRY **ksymbol_table);
Elf32_Addr PROC_ELF_Get_Dynamic_Symbol_Addr(PROC_ELF_DECODE_INFO *elf_decode_info, int index);

#endif /* PROC_RELOCLINK_H */
