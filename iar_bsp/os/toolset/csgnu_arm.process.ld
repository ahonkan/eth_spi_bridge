/*************************************************************************
*
* FILE NAME
*
*      csgnu_arm.process.ld
*
* DESCRIPTION
*
*      Linker description file for Nucleus Processes
*
*************************************************************************/
ENTRY(PROC_Premain);

SECTIONS 
{
    .text 0x00000000 :
    {
        PROVIDE_HIDDEN (_ld_text_start = .) ;
        KEEP(*nucleus_user.a:(.text .text*)) ;
        *(.text .text*) ;
        *(.glue_7t) ;
        *(.glue_7) ;
        *(.gnu.linkonce.t*)

        . = ALIGN(4) ;
        PROVIDE_HIDDEN (_rtl_init_start = .) ;
        
        /* Place only C RTL support initializers. */
        KEEP (*libc.a:(.init_array*)) ;
        
        PROVIDE_HIDDEN (_rtl_init_end = .) ;

        . = ALIGN(4) ;
        PROVIDE_HIDDEN (_cxx_ctor_start = .) ;
        
        /* Place all C++ constructors (initializers). */
        KEEP (*(.ctors)) ;
        KEEP (*(.init_array*)) ;     
        
        PROVIDE_HIDDEN (_cxx_ctor_end = .) ;
        
        . = ALIGN(4) ;
        
        /* Place all C++ destructors. */               
        *(.dtors) ;        
    }

    .ARM.extab : 
    { 
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    }

    .ARM.exidx : 
    {
        PROVIDE_HIDDEN (__exidx_start = .) ; 
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        PROVIDE_HIDDEN (__exidx_end = .) ; 
        PROVIDE_HIDDEN (_ld_text_end = ALIGN(PAGE_SIZE)) ;
    }

    .rodata ALIGN(PAGE_SIZE) :
    {
        PROVIDE_HIDDEN (_ld_rodata_start = .) ;
        *(.rodata*) ;
        
        /* Reserve space for the read-only copy of data section to be created at load time
           and used later as initdata to initialize the data section each time the process
           is started from the stopped state. */
        PROVIDE_HIDDEN (_ld_initdata_start = .) ;
        . = . + ( _ld_data_end  - _ld_data_start ) ;
    }

    nusymstr :
    {
        KEEP(*(nusymstr)) ;
    }
    
    nusymtab :
    {
        PROVIDE_HIDDEN (_nu_symbol_table_start = .) ;
        KEEP(*(nusymtab)) ;
        LONG(0x00000000) ;
        PROVIDE_HIDDEN (_ld_rodata_end = ALIGN(PAGE_SIZE)) ;
    }

    .got.plt ALIGN(PAGE_SIZE) : 
    { 
        PROVIDE_HIDDEN (_ld_wrdata_start = .) ;
        *(.got.plt)
    }

    .got : 
    { 
        *(.got)
    }

    .data :
    {
        PROVIDE_HIDDEN (_ld_data_start = .) ;
        *(.data) ;
        *(.data*) ;
        *(.gnu.linkonce.d*)
        PROVIDE_HIDDEN (_ld_data_end = .) ;
    }

    .bss (NOLOAD) : 
    {
        PROVIDE_HIDDEN (_ld_bss_start = .) ;
        *(.bss) ;
        *(.bss*) ;
        *(COMMON) ;
        
        /* Reserve space for C++ global destructors (registered via __cxa_atexit(). */
        PROVIDE_HIDDEN (_cxa_atexit_funcs = .) ;
        . = . + (_cxx_ctor_end - _cxx_ctor_start) * 2 ;

        PROVIDE_HIDDEN (_ld_bss_end = .) ;
        PROVIDE_HIDDEN (_ld_wrdata_end = ALIGN(PAGE_SIZE)) ;
    }

    nuprocinfo ALIGN(PAGE_SIZE) :
    {
        LONG(NUCLEUS_VERSION) ;
        LONG(_ld_text_start) ;
        LONG(_ld_text_end) ;
        LONG(_ld_rodata_start) ;
        LONG(_ld_rodata_end) ;
        LONG(_ld_wrdata_start) ;
        LONG(_ld_wrdata_end) ;
        LONG(_ld_load_info_start) ;
        LONG(_ld_load_info_end) ;
        LONG(_ld_initdata_start) ;
        LONG(_ld_data_start) ;
        LONG(_ld_data_end - _ld_data_start) ;
        LONG(_ld_bss_start) ;
        LONG(_ld_bss_end - _ld_bss_start) ;
    }
    
    .rel.plt : 
    { 
        PROVIDE_HIDDEN (_ld_load_info_start = .) ;
        *(.rel.plt)
    }

    .rel.dyn : 
    { 
        *(.rel.dyn)
    }

    .dynsym : 
    { 
        *(.dynsym)
    }

    .dynstr : 
    { 
        *(.dynstr)
        PROVIDE_HIDDEN (_ld_load_info_end = .) ;
    }

    .hash :
    { 
        *(.hash)
    }

    .shstrtab :
    { 
        *(.shstrtab)
    }

    .ARM.attributes :
    { 
        *(.ARM.attributes)
    }

    .interp :
    { 
        *(.interp)
    }

    .dynamic :
    { 
        *(.dynamic)
    }
    
    /* The following GNU specific ELF sections are not used in Nucleus Processes
       (so end up with zero size). These are specified here just to avoid them 
       being orphaned and cluttering up the areas of interest in the resulting
       linker map file. */

    .iplt : { *(.iplt) }
    .igot.plt : { *(.igot.plt) }
    .rel.iplt : { *(.rel.iplt) }
}
