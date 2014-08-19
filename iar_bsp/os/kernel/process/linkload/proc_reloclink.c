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
*       proc_reloclink.c
*
*   COMPONENT
*
*       Nucleus Processes - Linker / Loader
*
*   DESCRIPTION
*
*       Support for relocation and runtime linking of ELF objects.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       proc_elf_get_needed_sections
*       proc_elf_process_relocs_specific
*       proc_elf_get_entry_point_address
*       proc_elf_relocate_link
*       proc_elf_get_nu_symbol_table
*       proc_elf_get_nu_ksymbol_table
*       proc_elf_get_memory_reqs
*       proc_elf_file_seek_and_read
*       proc_elf_file_read_headers
*       proc_elf_file_read_sections
*       proc_elf_file_get_decode_info
*
*       PROC_ELF_File_Load
*       PROC_ELF_Get_Dynamic_Symbol_Name
*       PROC_ELF_Get_Dynamic_Symbol_Addr
*
*************************************************************************/

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "os/kernel/process/core/proc_core.h"
#include "process/arch_proc_extern.h"
#include "storage/nu_storage.h"

#include "proc_reloclink.h"
#include "proc_linkload.h"

#include <string.h>

#ifdef  CFG_NU_OS_KERN_PROCESS_SHELL_ENABLE
extern  BOOLEAN     PROC_Shell_Tryload;
extern  STATUS      PROC_Shell_Tryload_Status;
#endif

/* Local functions. */

static  Elf32_Shdr *
                proc_elf_get_needed_sections(PROC_ELF_DECODE_INFO *elf_decode_info);
static  STATUS  proc_elf_process_relocs_specific(PROC_CB *process,
                                                 PROC_ELF_DECODE_INFO *elf_decode_info,
                                                 UINT8 *runtime_buffer,
                                                 UINT8 *load_buffer,
                                                 Elf32_Shdr *section);
static  VOID    *proc_elf_get_entry_point_address(PROC_ELF_DECODE_INFO *elf_decode_info,
                                                  UINT8 *runtime_buffer);
static  STATUS  proc_elf_relocate_link(PROC_CB *process, PROC_ELF_DECODE_INFO *elf_decode_info,
                                       UINT8 *runtime_buffer, UINT8 *load_buffer);
static  NU_SYMBOL_ENTRY *
                proc_elf_get_nu_symbol_table(PROC_ELF_DECODE_INFO *elf_decode_info,
                                             UINT8 *runtime_buffer);
static  VOID    proc_elf_get_memory_reqs(PROC_ELF_DECODE_INFO *elf_decode_info,
                                         UINT32 *runtime_reqs, UINT32 *load_reqs);
static  STATUS  proc_elf_file_seek_and_read(INT fd, VOID *destination,
                                            Elf32_Off offset, Elf32_Word size);
static  STATUS  proc_elf_file_read_headers(INT fd, PROC_ELF_DECODE_INFO *elf_decode_info);
static  STATUS  proc_elf_file_read_sections(INT fd, PROC_ELF_DECODE_INFO *elf_decode_info,
                                            UINT8 *runtime_buffer, UINT8 *load_buffer);
static  STATUS  proc_elf_file_get_decode_info(INT fd, PROC_ELF_DECODE_INFO *elf_decode_info);

/*************************************************************************
*
*   FUNCTION
*
*       proc_elf_get_needed_sections
*
*   DESCRIPTION
*
*       Retrieves the sections we need during the load and link from the
*       section headers list.
*
*   INPUTS
*
*       elf_decode_info - ELF object decode info container.
*
*   OUTPUTS
*
*       Elf32_Shdr * - Pointer to the ELF section header of 'nuprocinfo'.
*       NU_NULL - If 'nuprocinfo' section is not found.
*
*************************************************************************/
static Elf32_Shdr *proc_elf_get_needed_sections(PROC_ELF_DECODE_INFO *elf_decode_info)
{
    Elf32_Shdr *current = (Elf32_Shdr *)(elf_decode_info->section_headers_start);

    /* We are interested in the following sections:
        .dynsym
        .dynstr
        .rel.plt
        .rel.dyn
        nuprocinfo
        nusymtab
    */
    INT         sections_to_find = 6;
    Elf32_Shdr  *nuprocinfo = NU_NULL;

    /* Search for sections but skip the reserved null section. */

    INT     section_count = elf_decode_info->elf_header.e_shnum - 1;
    while ((section_count > 0) && (sections_to_find > 0))
    {
        /* Compute the section header pointer. */
        current = (Elf32_Shdr *)(((UINT8 *)current) + elf_decode_info->elf_header.e_shentsize);

        /* Get the name of current section. */
        CHAR *current_name = elf_decode_info->shstrtab + current->sh_name;

        /* Proceed if the section is allocatable and is not executable. */
        if ((current->sh_flags & SHF_ALLOC) && !(current->sh_flags & SHF_EXECINSTR))
        {
            /* Check for '.dynsym' or '.dynstr' or '.rel.plt' or '.rel.dyn'. */
            if (*current_name == '.')
            {
                current_name++;

                /* Check for '.dynsym' or 'dynstr'. */
                if (*current_name == 'd')
                {
                    current_name++;

                    /* Check for '.dynsym'. */
                    if (strcmp(current_name, "ynsym") == 0)
                    {
                        elf_decode_info->dynsym = current;
                        sections_to_find--;
                    }

                    /* Check for '.dynstr'. */
                    else if (strcmp(current_name, "ynstr") == 0)
                    {
                        elf_decode_info->dynstr = current;
                        sections_to_find--;
                    }
                }

                /* Check for '.rel.plt' or '.rel.dyn'. */
                else if (*current_name == 'r')
                {
                    current_name++;

                    /* Check for '.rel.plt'. */
                    if (strcmp(current_name, "el.plt") == 0)
                    {
                        elf_decode_info->rel_plt = current;
                        sections_to_find--;
                    }

                    /* Check for '.rel.dyn'. */
                    else if (strcmp(current_name, "el.dyn") == 0)
                    {
                        elf_decode_info->rel_dyn = current;
                        sections_to_find--;
                    }
                }
            }

            /* Check for 'nusymtab' or 'nuprocinfo'. */
            else if ((*current_name == 'n') && (*(current_name + 1) == 'u'))
            {
                current_name += 2;

                /* Check for 'nuksymtab'. */
                if (strcmp(current_name, "ksymtab") == 0)
                {
                    elf_decode_info->nuksymtab = current;
                    sections_to_find--;
                }

                /* Check for 'nusymtab'. */
                if (strcmp(current_name, "symtab") == 0)
                {
                    elf_decode_info->nusymtab = current;
                    sections_to_find--;
                }

                /* Check for 'nuprocinfo'. */
                else if (strcmp(current_name, "procinfo") == 0)
                {
                    nuprocinfo = current;
                    sections_to_find--;
                }
            }
        }

        /* Move to the next section. */
        section_count--;
    }

    /* Return the 'nuprocinfo' section. */
    return (nuprocinfo);
}

/*************************************************************************
*
*   FUNCTION
*
*       proc_elf_process_relocs_specific
*
*   DESCRIPTION
*
*       Processes the relocations contained in the specified section.
*
*   INPUTS
*
*       process - The process which is being relocated.
*       elf_decode_info - ELF decoding information.
*       runtime_buffer - Buffer containing ELF sections which are part of runtime.
*       load_buffer - Buffer containing ELF sections which are needed only at load and link time.
*       section - Header of the specified relocation section.
*
*   OUTPUTS
*
*       NU_SUCCESS - Indicates successful operation.
*       <other> - Indicates (other) internal error occurred.
*
*************************************************************************/
static STATUS proc_elf_process_relocs_specific(PROC_CB *process, PROC_ELF_DECODE_INFO *elf_decode_info,
                                               UINT8 *runtime_buffer, UINT8 *load_buffer,
                                               Elf32_Shdr *section)
{
    STATUS      status = NU_SUCCESS;
    UINT8      *section_load_addr = load_buffer + section->sh_addr - elf_decode_info->nuprocinfo.load_info_start;
    int         i;

    /* Check the section type. */
    if (section->sh_type == SHT_REL)
    {
        /* Traverse the list of relocation entries contained in the section. */
        for (i = 0; (i < section->sh_size) && (status == NU_SUCCESS); i += section->sh_entsize)
        {
            /* Compute the relocation entry address. */
            Elf32_Rel  *rel_entry = (Elf32_Rel *)(section_load_addr + i);

            /* Process the relocation entry. */
            status = PROC_ELF_Process_Reloc_Entry(process, elf_decode_info, runtime_buffer, rel_entry);
        }
    }

    /* Return status to caller. */
    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       proc_elf_get_entry_point_address
*
*   DESCRIPTION
*
*       Retrieves the entry point address from the specified ELF object.
*
*   INPUTS
*
*       elf_decode_info - ELF object decode info container.
*       runtime_buffer - Buffer containing ELF sections which are part of runtime.
*
*   OUTPUTS
*
*       VOID * - Entry point address of the specified ELF object.
*
*************************************************************************/
static VOID *proc_elf_get_entry_point_address(PROC_ELF_DECODE_INFO *elf_decode_info,
                                              UINT8 *runtime_buffer)
{
    return (runtime_buffer + elf_decode_info->nuprocinfo.text_start +
                             elf_decode_info->elf_header.e_entry);
}

/*************************************************************************
*
*   FUNCTION
*
*       PROC_ELF_Relocate_Link
*
*   DESCRIPTION
*
*       Relocates and links the given ELF object.
*
*   INPUTS
*
*       process - The process being relocated and linked.
*       elf_decode_info - ELF object decode info container.
*       runtime_buffer - Buffer containing ELF sections which are part of runtime.
*       load_buffer - Buffer containing ELF sections which are needed only at load and link time.
*
*   OUTPUTS
*
*       NU_SUCCESS - Indicates successful operation.
*       <other> - Indicates (other) internal error occurred.
*
*************************************************************************/
static STATUS proc_elf_relocate_link(PROC_CB *process, PROC_ELF_DECODE_INFO *elf_decode_info,
                                     UINT8 *runtime_buffer, UINT8 *load_buffer)
{
    STATUS      status = NU_SUCCESS;

    /* Check of .rel.dyn section exists in the ELF. */
    if (elf_decode_info->rel_dyn)
    {
        /* Relocate and link .rel.dyn section. */
        status = proc_elf_process_relocs_specific(process, elf_decode_info,
                                                  runtime_buffer, load_buffer,
                                                  elf_decode_info->rel_dyn);
    }

    /* Proceed to check if .rel.plt section exists, if no error encountered yet. */
    if (status == NU_SUCCESS && elf_decode_info->rel_plt)
    {
        /* Relocate and link .rel.plt section. */
        status = proc_elf_process_relocs_specific(process, elf_decode_info,
                                                  runtime_buffer, load_buffer,
                                                  elf_decode_info->rel_plt);
    }

    /* Return status to caller */
    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       proc_elf_get_nu_symbol_table
*
*   DESCRIPTION
*
*       Retrieves the exported symbols table from the specified (Nucleus
*       process) ELF object.
*
*   INPUTS
*
*       elf_decode_info - ELF object decode info container.
*       runtime_buffer - Buffer containing ELF sections which are part of runtime.
*
*   OUTPUTS
*
*       NU_SYMBOL_ENTRY * - Pointer to exported symbols table.
*       NU_NULL - Indicates that exported symbols were not found.
*
*************************************************************************/
static NU_SYMBOL_ENTRY *proc_elf_get_nu_symbol_table(PROC_ELF_DECODE_INFO *elf_decode_info,
                                                     UINT8 *runtime_buffer)
{
    Elf32_Shdr      *section = elf_decode_info->nusymtab;
    NU_SYMBOL_ENTRY *symbol_table = NU_NULL;

    /* Ensure that symbol table exists and is not empty. */
    if (section && (section->sh_size > 4))
    {
        /* Compute symbol table address. */
        symbol_table = (NU_SYMBOL_ENTRY *)(runtime_buffer + section->sh_addr);
    }

    /* Return the symbol table address. */
    return (symbol_table);
}

/*************************************************************************
*
*   FUNCTION
*
*       proc_elf_get_nu_ksymbol_table
*
*   DESCRIPTION
*
*       Retrieves the exported kernel mode symbols table from the specified (Nucleus
*       process) ELF object.
*
*   INPUTS
*
*       elf_decode_info - ELF object decode info container.
*       runtime_buffer - Buffer containing ELF sections which are part of runtime.
*
*   OUTPUTS
*
*       NU_SYMBOL_ENTRY * - Pointer to exported symbols table.
*       NU_NULL - Indicates that exported symbols were not found.
*
*************************************************************************/
static NU_SYMBOL_ENTRY *proc_elf_get_nu_ksymbol_table(PROC_ELF_DECODE_INFO *elf_decode_info,
                                                      UINT8 *runtime_buffer)
{
    Elf32_Shdr      *section = elf_decode_info->nuksymtab;
    NU_SYMBOL_ENTRY *symbol_table = NU_NULL;

    /* Ensure that symbol table exists and is not empty. */
    if (section && (section->sh_size > 4))
    {
        /* Compute symbol table address. */
        symbol_table = (NU_SYMBOL_ENTRY *)(runtime_buffer + section->sh_addr);
    }

    /* Return the symbol table address. */
    return (symbol_table);
}

/*************************************************************************
*
*   FUNCTION
*
*       proc_elf_get_memory_reqs
*
*   DESCRIPTION
*
*       Computes the load time and runtime memory requirements for the
*       specified Nucleus Process ELF object.
*
*   INPUTS
*
*       elf_decode_info - ELF object decode info container.
*       runtime_reqs - Return pointer to memory requirement for ELF sections which are part of runtime layout.
*       load_reqs - Return pointer to memory requirement for ELF sections which are needed only at load and link time.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
static VOID proc_elf_get_memory_reqs(PROC_ELF_DECODE_INFO *elf_decode_info,
                                     UINT32 *runtime_reqs, UINT32 *load_reqs)
{
    *runtime_reqs = elf_decode_info->nuprocinfo.wrdata_end;
    *load_reqs = elf_decode_info->nuprocinfo.load_info_end -
                 elf_decode_info->nuprocinfo.load_info_start;
}

/*************************************************************************
*
*   FUNCTION
*
*       proc_elf_file_seek_and_read
*
*   DESCRIPTION
*
*       Seeks to the specified offset in the given file and reads the data
*       into the specified destination location.
*
*   INPUTS
*
*       fd - File to read from.
*       destination - Location into which the data should be read.
*       offset - Offset to seek in the file.
*       size - Size of the data to read.
*
*   OUTPUTS
*
*       NU_SUCCESS - Specified data was read successfully.
*       <other> - Indicates (other) internal error occurred.
*
*************************************************************************/
static STATUS proc_elf_file_seek_and_read(INT fd, VOID *destination,
                                          Elf32_Off offset, Elf32_Word size)
{
    STATUS      status = NU_SUCCESS;
    INT32       readsize;

    /* Seek to the specified offset. */
    readsize = NU_Seek(fd, offset, PSEEK_SET);

    /* Check if the seek was successful. */
    if (readsize == offset)
    {
        /* Read the data. */
        readsize = NU_Read(fd, (CHAR *)destination, size);

        /* Check if the data was read successfully. */
        if (readsize != size)
        {
            /* Return the read error. */
            status = readsize;
        }
    }
    else
    {
        /* Return the seek error. */
        status = readsize;
    }

    /* Return status to caller. */
    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       proc_elf_read_headers
*
*   DESCRIPTION
*
*       Reads the ELF headers (ELF header, section headers and the section
*       headers string table) essential to access further information from
*       the file containing the ELF object.
*
*   INPUTS
*
*       fd - File containing the ELF object.
*       elf_decode_info - ELF object decode info container.
*
*   OUTPUTS
*
*       NU_SUCCESS - ELF headers successfully read.
*       <other> - Indicates (other) internal error occurred.
*
*************************************************************************/
static STATUS proc_elf_file_read_headers(INT fd, PROC_ELF_DECODE_INFO *elf_decode_info)
{
    STATUS      status = NU_SUCCESS;
    UINT32      section_count;

    /* Read the ELF header. */
    status = proc_elf_file_seek_and_read(fd, &elf_decode_info->elf_header, 0, sizeof(Elf32_Ehdr));

    /* Ensure the read was successful. */
    if (status == NU_SUCCESS)
    {
        /* Get section count from the ELF header. */
        section_count = elf_decode_info->elf_header.e_shnum;

        /* Allocate memory to read in the section headers. */
        status = PROC_Alloc((VOID **)&elf_decode_info->section_headers_start,
                            section_count * elf_decode_info->elf_header.e_shentsize,
                            sizeof(Elf32_Shdr));

        /* Check if the allocation was successful. */
        if (status == NU_SUCCESS)
        {
            /* Read the section headers list. */
            status = proc_elf_file_seek_and_read(fd, elf_decode_info->section_headers_start,
                                                 elf_decode_info->elf_header.e_shoff,
                                                 section_count * elf_decode_info->elf_header.e_shentsize);

            /* Ensure the read was successful. */
            if (status == NU_SUCCESS)
            {
                /* Compute the pointer to section header string table section. */
                Elf32_Shdr *section_header_string_table =
                        (Elf32_Shdr *)(elf_decode_info->section_headers_start +
                                       elf_decode_info->elf_header.e_shstrndx *
                                       elf_decode_info->elf_header.e_shentsize);

                /* Allocate the memory for section header string table. */
                status = PROC_Alloc((VOID **)&elf_decode_info->shstrtab, section_header_string_table->sh_size, 1);

                /* Ensure the allocation was successful. */
                if (status == NU_SUCCESS)
                {
                    /* Read the section headers string table. */
                    status = proc_elf_file_seek_and_read(fd, elf_decode_info->shstrtab,
                                                         section_header_string_table->sh_offset,
                                                         section_header_string_table->sh_size);
                }
            }
        }
    }

    /* Return status to caller. */
    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       proc_elf_file_read_sections
*
*   DESCRIPTION
*
*       Reads the ELF section contents from the specified file containing
*       the ELF object.
*
*   INPUTS
*
*       fd - File containing the ELF object.
*       elf_decode_info - ELF object decode info container.
*       runtime_buffer - Buffer containing ELF sections which are part of runtime.
*       load_buffer - Buffer containing ELF sections which are needed only at load and link time.
*
*   OUTPUTS
*
*       NU_SUCCESS - ELF sections were read from the file successfully.
*       <other> - Indicates (other) internal error occurred.
*
*************************************************************************/
static STATUS proc_elf_file_read_sections(INT fd, PROC_ELF_DECODE_INFO *elf_decode_info,
                                          UINT8 *runtime_buffer, UINT8 *load_buffer)
{
    STATUS      status = NU_SUCCESS;
    Elf32_Shdr *current = (Elf32_Shdr *)(elf_decode_info->section_headers_start);

    /* Traverse all sections except the reserved null section. */
    int    section_count = elf_decode_info->elf_header.e_shnum - 1;
    while ((section_count > 0) && (status == NU_SUCCESS))
    {
        /* Compute the pointer to section header. */
        current = (Elf32_Shdr *)(((UINT8 *)current) + elf_decode_info->elf_header.e_shentsize);

        /* Make sure the section is allocateable and is not empty. */
        if ((current->sh_flags & SHF_ALLOC) && (current->sh_size))
        {
            CHAR    *destination = NU_NULL;

            /* Check if the section is part of runtime and is not BSS - clearing of BSS is handled later when process is started. */
            if ((current->sh_addr < elf_decode_info->nuprocinfo.wrdata_end) && ((current->sh_type & SHT_NOBITS) == 0))
            {
                /* Compute the destination address where the section should be copied. */
                destination = (CHAR *)(runtime_buffer + current->sh_addr);
            }

            /* Check if the section is needed for load and link. */
            else if ((current->sh_addr >= elf_decode_info->nuprocinfo.load_info_start) &&
                     (current->sh_addr <  elf_decode_info->nuprocinfo.load_info_end))
            {
                /* Compute the destination address where the section should be copied. */
                destination = (CHAR *)(load_buffer + current->sh_addr - elf_decode_info->nuprocinfo.load_info_start);
            }

            /* Check if the section needs to be copied. */
            if (destination)
            {
                /* Read the section contents. */
                status = proc_elf_file_seek_and_read(fd, destination, current->sh_offset, current->sh_size);
            }
        }

        /* Move to the next section. */
        section_count--;
    }

    /* Return status to caller. */
    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       proc_elf_file_get_decode_info
*
*   DESCRIPTION
*
*       Retrieves the information necessary to decode the ELF object for
*       loading, relocating and linking.
*
*   INPUTS
*
*       fd - File containing the ELF object.
*       elf_decode_info - ELF object decode info container.
*
*   OUTPUTS
*
*       NU_SUCCESS - ELF decode info was retrieved successfully.
*       NU_INVALID_PROCESS - Specified ELF is not a valid procees image.
*       <other> - Indicates (other) internal error occurred.
*
*************************************************************************/
static STATUS proc_elf_file_get_decode_info(INT fd, PROC_ELF_DECODE_INFO *elf_decode_info)
{
    STATUS      status;
    Elf32_Shdr  *nuprocinfo;

    /* Read the ELF headers (ELF header and section headers including the section header string table). */
    status = proc_elf_file_read_headers(fd, elf_decode_info);

    /* Ensure that ELF headers were read successfully. */
    if (status == NU_SUCCESS)
    {
        /* Retrieve the sections required for load. */
        nuprocinfo = proc_elf_get_needed_sections(elf_decode_info);

        /* Check if Nucleus process info section exists in the ELF. */
        if ((nuprocinfo != NU_NULL) && (nuprocinfo->sh_size == sizeof(PROC_NUPROCINFO)))
        {
            /* Read the section contents. */
            status = proc_elf_file_seek_and_read(fd, &elf_decode_info->nuprocinfo,
                                                 nuprocinfo->sh_offset,
                                                 sizeof(PROC_NUPROCINFO));
        }
        else
        {
            /* A valid Nucleus process info section could not be found. */
            status = NU_INVALID_PROCESS;
        }
    }

    /* Return status to caller. */
    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       PROC_ELF_File_Load
*
*   DESCRIPTION
*
*       Loads, relocates and links the ELF object contained in the
*       given file.
*
*   INPUTS
*
*       process - The process which is being loaded.
*       fd - File containing the ELF object.
*       load_addr - Return pointer for the runtime load address.
*       entry_addr - Return pointer for the entry point address.
*       root_stack - Return pointer to the root stack memory pointer.
*       root_stack_size - Root stack size.
*       heap_size - Desired heap size.
*       page_size - Required page size.
*       symbol_table - Return pointer for the symbol table.
*       ksymbol_table - Return pointer for the kernel mode symbol table.
*
*   OUTPUTS
*
*       NU_SUCCESS - ELF was loaded successfully.
*       <other> - Indicates (other) internal error occurred.
*
*************************************************************************/
STATUS PROC_ELF_File_Load(PROC_CB *process, INT fd, VOID **load_addr, VOID **entry_addr,
                          VOID **root_stack, UINT32 root_stack_size,
                          UINT32 heap_size, UINT32 page_size,
                          NU_SYMBOL_ENTRY **symbol_table,
                          NU_SYMBOL_ENTRY **ksymbol_table)
{
    STATUS                  status;
    STATUS                  dealloc_status;
    PROC_ELF_DECODE_INFO    elf_decode_info;
    UINT32                  runtime_reqs;
    UINT32                  load_reqs;
    UINT8                  *load_buffer = NU_NULL;
    UINT8                  *runtime_buffer = NU_NULL;
    UINT32                  total_runtime_reqs;

    /* Clear the ELF decode struct. */
    memset(&elf_decode_info, 0, sizeof(PROC_ELF_DECODE_INFO));

    /* Clear the return pointers. */
    *load_addr     = NU_NULL;
    *entry_addr    = NU_NULL;
    *root_stack    = NU_NULL;
    *symbol_table  = NU_NULL;
    *ksymbol_table = NU_NULL;

    /* Get the essential information to decode the ELF. */
    status = proc_elf_file_get_decode_info(fd, &elf_decode_info);

    /* Ensure successful. */
    if (status == NU_SUCCESS)
    {
        /* Get the memory requirements for runtime and load and link. */
        proc_elf_get_memory_reqs(&elf_decode_info, &runtime_reqs, &load_reqs);

        /* Ensure run-time requirements size is aligned as per stack alignment requirement.
           This is needed because root task stack immediately follows the runtime section
           in process memory laypout. */
        runtime_reqs = (UINT32)ESAL_GE_MEM_PTR_ALIGN(runtime_reqs, PROC_STACK_ALIGNMENT);

        /* Compute the total runtime memory requirements counting in the root stack and heap. */
        total_runtime_reqs = runtime_reqs + root_stack_size + heap_size;

        /* Allocate the memory for the ELF runtime layout. */
        status = PROC_Alloc((VOID **)&runtime_buffer, total_runtime_reqs, page_size);

        /* Check if the allocation was successful. */
        if (status == NU_SUCCESS)
        {
            /* Allocate the memory for sections needed only during load & link. */
            status = PROC_Alloc((VOID **)&load_buffer, load_reqs, 0);

            /* Check if the allocation was successful. */
            if (status == NU_SUCCESS)
            {
                /* Read the section contents. */
                status = proc_elf_file_read_sections(fd, &elf_decode_info, runtime_buffer, load_buffer);

                /* Check if the section contents were read successful. */
                if (status == NU_SUCCESS)
                {
                    /* Close the file. */
                    status = NU_Close(fd);

                    /* Ensure the file was closed successfully. */
                    if (status == NU_SUCCESS)
                    {
                        /* Precompute the .dynsym and .dynstr load addresses for speed. */
                        elf_decode_info.dynsym_addr = load_buffer +
                                elf_decode_info.dynsym->sh_addr -
                                elf_decode_info.nuprocinfo.load_info_start;
                        elf_decode_info.dynstr_addr = load_buffer +
                                elf_decode_info.dynstr->sh_addr -
                                elf_decode_info.nuprocinfo.load_info_start;

                        /* Relocate and link the runtime sections. */
                        status = proc_elf_relocate_link(process, &elf_decode_info,
                                runtime_buffer, load_buffer);

#ifdef CFG_NU_OS_KERN_PROCESS_SHELL_ENABLE

                        if ((PROC_Shell_Tryload == NU_TRUE) && (PROC_Shell_Tryload_Status != NU_SUCCESS))
                        {
                            /* Set status to tryload status saved */
                            status = PROC_Shell_Tryload_Status;
                        }

#endif /* CFG_NU_OS_KERN_PROCESS_SHELL_ENABLE */

                        /* Check if the relocation and link was successful. */
                        if (status == NU_SUCCESS)
                        {
                            /* Return the (runtime) load address. */
                            *load_addr = runtime_buffer;

                            /* Return the entry point. */
                            *entry_addr = proc_elf_get_entry_point_address(&elf_decode_info, runtime_buffer);

                            /* Return the symbol table. */
                            *symbol_table = proc_elf_get_nu_symbol_table(&elf_decode_info, runtime_buffer);

                            /* Return the kernel mode symbol table. */
                            *ksymbol_table = proc_elf_get_nu_ksymbol_table(&elf_decode_info, runtime_buffer);

#if (CFG_NU_OS_KERN_PROCESS_LINKLOAD_DUP_SYMBOL_CHECK == NU_TRUE)

                            if (*symbol_table != NU_NULL)
                            {
                                /* Validate all exported user-mode symbols */
                                status = PROC_Validate_Symbols(*symbol_table);
                            }

                            if ((status == NU_SUCCESS) &&
                                (*ksymbol_table != NU_NULL))
                            {
                                /* Validate all exported kernel-mode symbols */
                                status = PROC_Validate_Symbols(*ksymbol_table);
                            }

#ifdef CFG_NU_OS_KERN_PROCESS_SHELL_ENABLE

                            if ((PROC_Shell_Tryload == NU_TRUE) && (PROC_Shell_Tryload_Status != NU_SUCCESS))
                            {
                                /* Set status to indicate a duplicate symbol was found. */
                                status = PROC_Shell_Tryload_Status;
                            }

#endif /* CFG_NU_OS_KERN_PROCESS_SHELL_ENABLE */

#endif /* CFG_NU_OS_KERN_PROCESS_LINKLOAD_DUP_SYMBOL_CHECK */

                            if (status == NU_SUCCESS)
                            {
                                /* Return the root stack memory pointer. */
                                *root_stack = runtime_buffer + runtime_reqs;

                                /* Save text section start address */
                                process -> text_start = (VOID *)elf_decode_info.nuprocinfo.text_start;

                                /* Save the read only data section start address */
                                process -> rodata_start = (VOID *)elf_decode_info.nuprocinfo.rodata_start;

                                /* Save the data section start address */
                                process -> data_bss_start = (VOID *)elf_decode_info.nuprocinfo.wrdata_start;

                                /* Save the data and initdata sections info */
                                process -> initdata_start = (VOID *)(runtime_buffer + elf_decode_info.nuprocinfo.initdata_start);
                                process -> data_start = (VOID *)(runtime_buffer + elf_decode_info.nuprocinfo.data_start);
                                process -> data_size = elf_decode_info.nuprocinfo.data_size;

                                /* Create initdata copy from relocated data section for reinitialization of data section on
                                 * process restart. */
                                memcpy(process -> initdata_start, process -> data_start, process -> data_size);

                                /* Save the BSS section info */
                                process -> bss_start = (VOID *)(runtime_buffer + elf_decode_info.nuprocinfo.bss_start);
                                process -> bss_size = elf_decode_info.nuprocinfo.bss_size;

                                /* Flush data cache / invalidate instruction cache */
                                ESAL_CO_MEM_ICACHE_FLUSH_INVAL(runtime_buffer, total_runtime_reqs);
                            }
                        }
                    }
                }
            }
        }
    }

    /* Deallocate the dynamically allocated memory blocks. */

    if (load_buffer)
    {
        /* Deallocate the memory allocated for section needed only for load & link. */
        dealloc_status = PROC_Free(load_buffer);
    }

    if ((dealloc_status == NU_SUCCESS) && (elf_decode_info.section_headers_start != NU_NULL))
    {
        /* Deallocate the memory allocated for section headers list. */
        dealloc_status = PROC_Free(elf_decode_info.section_headers_start);
    }

    if ((dealloc_status == NU_SUCCESS) && (elf_decode_info.shstrtab != NU_NULL))
    {
        /* Deallocate the memory allocated for section headers string table. */
        dealloc_status = PROC_Free(elf_decode_info.shstrtab);
    }

    /* Return the memory deallocation status only if original status was success. */
    if (status == NU_SUCCESS)
    {
        status = dealloc_status;
    }
    else
    {
        /* In case of any error, deallocate the memory allocated for runtime,
           if had been allocated. But we'll return the original status code,
           not of the deallocation. */
        if (runtime_buffer)
        {
            (VOID)PROC_Free(runtime_buffer);
        }
    }

    /* Return status to caller. */
    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       PROC_ELF_Get_Dynamic_Symbol_Name
*
*   DESCRIPTION
*
*       Retrieves the string name of the symbol specified as index from the
*       given ELF object.
*
*   INPUTS
*
*       elf_decode_info - ELF object decode info container.
*       index - Index of the desired symbol in the dynamic symbol table.
*
*   OUTPUTS
*
*       String containing the name of the specified symbol.
*
*************************************************************************/
CHAR *PROC_ELF_Get_Dynamic_Symbol_Name(PROC_ELF_DECODE_INFO *elf_decode_info, int index)
{
    CHAR        *symbol_name = NU_NULL;
    Elf32_Sym   *symbol_entry = (Elf32_Sym *)(elf_decode_info->dynsym_addr +
                                              index * elf_decode_info->dynsym->sh_entsize);

    /* Check if the symbol really has a string name. */
    if (symbol_entry->st_name)
    {
        /* Compute the symbol name address. */
        symbol_name = (CHAR *)(elf_decode_info->dynstr_addr + symbol_entry->st_name);
    }

    /* Return the symbol name. */
    return (symbol_name);
}

/*************************************************************************
*
*   FUNCTION
*
*       PROC_ELF_Get_Dynamic_Symbol_Addr
*
*   DESCRIPTION
*
*       Retrieves the (relocatable) address of the symbol specified as
*       index from the given ELF object.
*
*   INPUTS
*
*       elf_decode_info - ELF object decode info container.
*       index - Index of the desired symbol in the dynamic symbol table.
*
*   OUTPUTS
*
*       Address of the specified symbol.
*
*************************************************************************/
Elf32_Addr PROC_ELF_Get_Dynamic_Symbol_Addr(PROC_ELF_DECODE_INFO *elf_decode_info, int index)
{
    Elf32_Sym   *symbol_entry = (Elf32_Sym *)(elf_decode_info->dynsym_addr +
                                              index * elf_decode_info->dynsym->sh_entsize);

    /* Return the symbol address. */
    return (symbol_entry->st_value);
}

