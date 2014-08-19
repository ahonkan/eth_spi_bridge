/*************************************************************************
*
* FILE NAME
*
*      csgnu_arm.iar_stm32f429ii_sk.link_rom.ld
*
* DESCRIPTION
*
*      Linker description (ROM) file for the following:
*
*      BOARD:      IAR STM32F429II-SK
*
*************************************************************************/
ENTRY(ESAL_Entry);

/* Memory Spaces Definitions */
MEMORY
{
    ROM :   ORIGIN = 0x08000000, LENGTH = 0x00200000 /* 2   M */
    SRAM :  ORIGIN = 0x20000000, LENGTH = 0x00030000 /* 192 K */ 
}

SECTIONS 
{    
    .text :
    {
        KEEP(*(esal_vectors)) ;
        KEEP(*(esal_code)) ;
        *(.text .text*) ;
        *(.glue_7t) ;
        *(.glue_7) ;
        *(.gnu.linkonce.t*)

        . = ALIGN(4) ;
        PROVIDE (_rtl_init_start = .) ;
        
        /* Place only C RTL support initializers. */
        KEEP (*libc.a:(.init_array*)) ;
        
        PROVIDE (_rtl_init_end = .) ;

        . = ALIGN(4) ;
        PROVIDE (_cxx_ctor_start = .);
        *(.ctors);
        KEEP (*(.init_array))
        PROVIDE (_cxx_ctor_end = .);
        *(.dtors);
        LONG (0);
        
    } > ROM

    .ARM.extab : 
    { 
        *(.ARM.extab* .gnu.linkonce.armextab.*)
   
    } > ROM

    
    .ARM.exidx : 
    { 
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*) 
        __exidx_end = .;

    } > ROM

    .rodata :
    {
        *(.rodata*) ;
    
    } > ROM

    _etext = .;
    PROVIDE (etext = .);

    _ld_rom_data_start = .;
    
    .data : AT (_ld_rom_data_start)        
    {      
         . = ALIGN(4) ;  
        _ld_ram_data_start = . ;
        *(.data) ;
        *(.data*) ;
        *(.gnu.linkonce.d*)
        . = ALIGN(4) ;
        
    } > SRAM
    
    PROVIDE (edata = .);
    _ld_ram_data_end = ALIGN(0x4);    

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        end = .;
        _ld_bss_start = end ;
        *(.bss) ;
        *(.bss*) ;
        *(COMMON) ;
        . = ALIGN(4);
        _ld_bss_end = . ;
    
    } > SRAM
}

