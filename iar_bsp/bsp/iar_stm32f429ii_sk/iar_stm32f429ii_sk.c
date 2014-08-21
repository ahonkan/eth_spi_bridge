/***********************************************************************
*
*             Copyright 2013 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
************************************************************************

************************************************************************
*
*   FILE NAME
*
*       iar_stm32f429ii_sk.c
*
*   DESCRIPTION
*
*       This file contains IAR_STM32F429II_SK specific data and code.
*
*   FUNCTIONS
*
*       None
*
*   DEPENDENCIES
*
*       nucleus.h                       
*       nu_kernel.h
*       nu_services.h
*
***********************************************************************/

/* Include required header files */
#include            "nucleus.h"
#include            "kernel/nu_kernel.h"
#include            "services/nu_services.h"

/* PLL_VCO = (HSE_VALUE or HSI_VALUE / PLL_M) * PLL_N */
#define CPU_RCC_BASE                0x40023800
#define PWR_CR                      0x40007000
#define PWR_CSR                     0x40007004
#define PLL_M                       8
#define PLL_N                       360
#define PLL_P                       2
#define PLL_Q                       7
#define CPU_RCC_CR                  0x00
#define CPU_RCC_PLLCFGR             0x04
#define CPU_RCC_CFGR                0x08
#define CPU_RCC_CIR                 0x0C
#define CPU_RCC_APB1ENR             0x40

#define BIT_FLD_SET(bit_pos,value)  (value << bit_pos)

#define AFC                         0xC
#define AFM                         0x02
#define SPEED_50_MHz                0x02
#define FSMC_BASE                   0xA0000010
#define FSMC_BCR_OFFSET             0x00 /* Accessing Region 3 of Bank1 */
#define FSMC_BTR_OFFSET             0x04 /* Accessing Region 3 of Bank1 */

/* Flash access control register */
#define FLASH_ACR                   0x40023C00

/* SDRAM availability flag. */
BOOLEAN  SDRAM_Available = NU_FALSE;

#define REFRESH_COUNTER             683

/* Initialize to number of memory regions present on the target */
UINT32  ESAL_DP_MEM_Num_Regions = ESAL_DP_MEM_NUM_REGIONS;

void          SDRAM_Initialization(void);
void          SDRAM_GPIO_Configuration(void);
void          SDRAM_Init_Sequence(void);
extern VOID   RCC_AHB3_Periph_Clock_Enable (UINT32 pher, FunctionalState option);

/* Platform memory map used by MMU set-up routines and memory pool creation routines */
const ESAL_GE_MEM_REGION      ESAL_DP_MEM_Region_Data[ESAL_DP_MEM_NUM_REGIONS] =
{
            /* MEMORY REGION 1 - CODE (INTERNAL FLASH)          */
            {(VOID *)0x08000000,            /* Physical Start   */
             (VOID *)0x08000000,            /* Virtual Start    */
             ESAL_GE_MEM_2M,                /* Size             */
             ESAL_NOCACHE,                  /* Cache type       */
             ESAL_ROM,                      /* Memory type      */
             ESAL_INST_AND_DATA},           /* Memory access    */

             /* MEMORY REGION 2 - Core Coupled SRAM (INTERNAL)   */
             {(VOID *)0x10000000,            /* Physical Start   */
              (VOID *)0x10000000,            /* Virtual Start    */
              ESAL_GE_MEM_1K * 64,           /* Size             */
              ESAL_NOCACHE,                  /* Cache type       */
              ESAL_RAM,                      /* Memory type      */
              ESAL_INST_AND_DATA},           /* Memory access    */

             /* MEMORY REGION 3 - SRAM (INTERNAL)                */
            {(VOID *)0x20000000,            /* Physical Start   */
             (VOID *)0x20000000,            /* Virtual Start    */
             ESAL_GE_MEM_1K * 192,          /* Size             */
             ESAL_NOCACHE,                  /* Cache type       */
             ESAL_RAM,                      /* Memory type      */
             ESAL_INST_AND_DATA},           /* Memory access    */

            /* MEMORY REGION 4 - PERIPHERAL REGISTERS           */
            {(VOID *)ESAL_DP_PERIPH_BASE,   /* Physical Start   */
             (VOID *)ESAL_DP_PERIPH_BASE,   /* Virtual Start    */
             ESAL_GE_MEM_64M,               /* Size             */
             ESAL_NOCACHE,                  /* Cache type       */
             ESAL_MEM_MAPPED,               /* Memory type      */
             ESAL_DATA},                    /* Memory access    */

            /* MEMORY REGION 5 - SDRAM (EXTERNAL)               */
            {(VOID *)0xD0000000,            /* Physical Start   */
             (VOID *)0xD0000000,            /* Virtual Start    */
             ESAL_GE_MEM_4M,                /* Size             */
             ESAL_NOCACHE,                  /* Cache type       */
             ESAL_RAM,                      /* Memory type      */
             ESAL_INST_AND_DATA},           /* Memory access    */
};

/*************************************************************************
* FUNCTION
*
*       Board_Lowlevel_Init
*
* DESCRIPTION
*
*       This functions initializes board minimum items (clocks, etc)
*
* INPUTS
*
*       NONE
*
* OUTPUTS
*
*       NONE
*
*************************************************************************/
VOID Board_Lowlevel_Init(VOID)
{
    /* Enable Internal High Speed Clock (HSI Oscilator will be ON) */
    ESAL_GE_MEM_WRITE32(CPU_RCC_BASE + CPU_RCC_CR, 
        ESAL_GE_MEM_READ32(CPU_RCC_BASE + CPU_RCC_CR) | 0x00000001);
        
    /* Reset CFGR register */
    ESAL_GE_MEM_WRITE32(CPU_RCC_BASE + CPU_RCC_CFGR, 0x00000000);
    
    /* Clear (Reset) HSEON, CSSON and PLLON bits */
    ESAL_GE_MEM_WRITE32(CPU_RCC_BASE + CPU_RCC_CR, 
        ESAL_GE_MEM_READ32(CPU_RCC_BASE + CPU_RCC_CR) & 0xFEF6FFFF);

    /* Reset PLLCFGR register by its reset value */
    ESAL_GE_MEM_WRITE32(CPU_RCC_BASE + CPU_RCC_PLLCFGR, 0x24003010);

    /* Clear (Reset) HSEBYP bit */
    ESAL_GE_MEM_WRITE32(CPU_RCC_BASE + CPU_RCC_CR, 
        ESAL_GE_MEM_READ32(CPU_RCC_BASE + CPU_RCC_CR) & 0xFFFBFFFF);

    /* Disable all interrupts */
    ESAL_GE_MEM_WRITE32(CPU_RCC_BASE + CPU_RCC_CIR, 0x00000000);
    
    /* Configure Sysclock */

    /* Enable HSE (HSEON bit)  */
    ESAL_GE_MEM_WRITE32(CPU_RCC_BASE + CPU_RCC_CR, 
        ESAL_GE_MEM_READ32(CPU_RCC_BASE + CPU_RCC_CR) | 0x00010000);
    
    /* Wait until HSE Oscilator is ready */
    while (ESAL_GE_MEM_READ32(CPU_RCC_BASE + CPU_RCC_CR) & 0x00020000);

    /* Enable Power Interface Clock (APB1 Power) */
    ESAL_GE_MEM_WRITE32(CPU_RCC_BASE + CPU_RCC_APB1ENR, 0x10000000);

    /* Select Regular Voltage Scaling Output (Scale 1 Mode) */
    ESAL_GE_MEM_WRITE32(PWR_CR, ESAL_GE_MEM_READ32(PWR_CR) | 0x0000C000);
    
    /* Set Internal Hight Speed Clock Calibration [HCLK=SYSCLK, PCLK1=HCLK/8, PCLK2=HCLK/2] */
    ESAL_GE_MEM_WRITE32(CPU_RCC_BASE + CPU_RCC_CFGR, 0x00009800);

    ESAL_GE_MEM_WRITE32(CPU_RCC_BASE + CPU_RCC_PLLCFGR, 
            (PLL_M                      | /* Division factor for the main PLL (PLL) and audio PLL (PLLI2S) input clock */
            (PLL_N << 6)                | /* Main PLL (PLL) multiplication factor for VCO */
            (((PLL_P >> 1) -1) << 16)   | /* Main PLL (PLL) division factor for main system clock (PLLP = 2) */
            (0x00400000)                | /* HSE oscillator clock selected as PLL and PLLI2S clock entry */
            (PLL_Q << 24))                /* Main PLL (PLL) division factor for USB OTG FS, SDIO and random number generator clocks */
            );         

    /* Enable PLL */
    ESAL_GE_MEM_WRITE32(CPU_RCC_BASE + CPU_RCC_CR, 
        ESAL_GE_MEM_READ32(CPU_RCC_BASE + CPU_RCC_CR) | 0x01000000);

    /* Wait while PLL is set (LOCKED). */
    while (ESAL_GE_MEM_READ32(CPU_RCC_BASE + CPU_RCC_CR) & 0x02000000);

    /* Enable the Over-drive to extend the clock frequency to 180 Mhz */
    ESAL_GE_MEM_WRITE32(PWR_CR, ESAL_GE_MEM_READ32(PWR_CR) | 0x00010000);

    /* Wait until it is stable */
    while((ESAL_GE_MEM_READ32(PWR_CSR) & 0x00010000) == 0)
    {

    }

    /* Over drive switching is enabled */
    ESAL_GE_MEM_WRITE32(PWR_CR, ESAL_GE_MEM_READ32(PWR_CR) | 0x00020000);
    
    /* Wait until it is stable */
    while((ESAL_GE_MEM_READ32(PWR_CSR) & 0x00020000) == 0)
    {

    }

    /* Wait 5 Flash states. Also enable Instruction and Data Caches. Instruction Cache is also reset */
    ESAL_GE_MEM_WRITE32(FLASH_ACR, 0x00000705);

    /* Clear and then Select PLL as system clock source */
    ESAL_GE_MEM_WRITE32(CPU_RCC_BASE + CPU_RCC_CFGR, 
        ESAL_GE_MEM_READ32(CPU_RCC_BASE + CPU_RCC_CFGR) & ~(0x00000003));

    ESAL_GE_MEM_WRITE32(CPU_RCC_BASE + CPU_RCC_CFGR, 
        ESAL_GE_MEM_READ32(CPU_RCC_BASE + CPU_RCC_CFGR) | 0x00000002);
    
    /* Wait until PLL is set as the system clock. */
    while ((ESAL_GE_MEM_READ32(CPU_RCC_BASE + CPU_RCC_CFGR) & 0x0000000C) != 0x00000008);
}

/***********************************************************************
*
*   FUNCTION
*
*       nu_bsp_iar_stm32f429ii_sk_init
*
*   DESCRIPTION
*
*       This function initialize SDRAM. 
*
*   INPUTS
*
*       key                                 Path to registry
*       startstop                           Option to Register or Unregister
*
*   OUTPUTS
*
*       NU_SUCCESS
*
***********************************************************************/
STATUS  nu_bsp_iar_stm32f429ii_sk_init(const CHAR *key, INT startorstop)
{
    UINT32           sdram_start;
    UINT32           sdram_size;
    NU_MEMORY_POOL   *sys_pool_ptr;
    STATUS           status = ~NU_SUCCESS;

    /* Dont init sdram as it's not going to be used.  
     * The setup code needs to be reviewed for the IAR board
     */
    return NU_SUCCESS;
    
    /* Configure the FMC Parallel interface */
    SDRAM_Initialization();      
    
    if (key != NU_NULL)
    {
        if (startorstop)
        {
            /* Retrieve SDRAM start address from registry. */
            status = REG_Get_UINT32_Value (key, "/sdram/start", &sdram_start);

            if(status == NU_SUCCESS)
            {
                /* Retrieve SDRAM size from registry. */
                status = REG_Get_UINT32_Value (key, "/sdram/size", &sdram_size);
            }

            /* In case, valid SDRAM address and size have been provided. */
            if ((status == NU_SUCCESS) && (sdram_start != 0xFFFFFFFF) && (sdram_size != 0))
            {
                /* Get system memory pool */
                status = NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);

                if(status == NU_SUCCESS)
                {
                    /* Add SDRAM memory to system memory pool pointer. */
                    status = NU_Add_Memory(sys_pool_ptr, (VOID *)sdram_start, sdram_size);
                }

                if(status == NU_SUCCESS)
                {
                    /* Zero out SDRAM */
                    (VOID)memset((VOID *)sdram_start, 0, sdram_size);
    
                    /* Set SDRAM available flag as TRUE. */
                    SDRAM_Available = NU_TRUE;
                }
            }            
        }
    }
    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       SDRAM_Initialization
*
*   DESCRIPTION
*
*       This function initialize SDRAM       
*
*   INPUT
*
*       None  
*
*   OUTPUT
*
*       None  
*
*************************************************************************/
VOID SDRAM_Initialization()
{
    FMC_SDRAM_Init  fmc_sdram_init;
    FMC_SDRAM_Timing_Init  fmc_sdram_timing; 

    /* GPIO configuration for FMC SDRAM bank */
    SDRAM_GPIO_Configuration();

    /* Enable FMC clock */
    RCC_AHB3_Periph_Clock_Enable (RCC_AHB3Periph_FMC, ENABLE);

    /* FMC SDRAM Bank configuration */   

    /* TMRD: 2 Clock cycles */
    fmc_sdram_timing.FMC_LoadToActiveDelay = 2;   
    
    /* TXSR: min=70ns (6x11.90ns) */
    fmc_sdram_timing.FMC_ExitSelfRefreshDelay = 7;
    
    /* TRAS: min=42ns (4x11.90ns) max=120k (ns) */
    fmc_sdram_timing.FMC_SelfRefreshTime = 4;
    
    /* TRC:  min=63 (6x11.90ns) */        
    fmc_sdram_timing.FMC_RowCycleDelay = 7;   
    
    /* TWR:  2 Clock cycles */
    fmc_sdram_timing.FMC_WriteRecoveryTime = 2;  
    
    /* TRP:  15ns => 2x11.90ns */
    fmc_sdram_timing.FMC_RPDelay = 2;  
    
    /* TRCD: 15ns => 2x11.90ns */
    fmc_sdram_timing.FMC_RCDDelay = 2;

    /* FMC SDRAM control configuration */
    fmc_sdram_init.FMC_Bank = FMC_Bank2_SDRAM;
    
    /* Row addressing: [7:0] */
    fmc_sdram_init.FMC_ColumnBitsNumber = FMC_ColumnBits_Number_8b;
    
    /* Column addressing: [10:0] */
    fmc_sdram_init.FMC_RowBitsNumber = FMC_RowBits_Number_12b;
    fmc_sdram_init.FMC_SDMemoryDataWidth = SDRAM_MEMORY_WIDTH;
    fmc_sdram_init.FMC_InternalBankNumber = FMC_InternalBank_Number_4;
    fmc_sdram_init.FMC_CASLatency = SDRAM_CAS_LATENCY; 
    fmc_sdram_init.FMC_WriteProtection = FMC_Write_Protection_Disable;
    fmc_sdram_init.FMC_SDClockPeriod = SDCLOCK_PERIOD;  
    fmc_sdram_init.FMC_ReadBurst = SDRAM_READBURST;
    fmc_sdram_init.FMC_ReadPipeDelay = FMC_ReadPipe_Delay_1;
    fmc_sdram_init.FMC_SDRAMTimingStruct = &fmc_sdram_timing;

    /* FMC SDRAM bank initialization */
    FMC_SDRAM_Initialization(&fmc_sdram_init); 

    /* FMC SDRAM device initialization sequence */
    SDRAM_Init_Sequence(); 
}

/*************************************************************************
*
*   FUNCTION
*
*       SDRAM_GPIO_Configuration
*
*   DESCRIPTION
*
*       This function configure SDRAM GPIO's pins 
*
*   INPUT
*
*       None  
*
*   OUTPUT
*
*       None  
*
*************************************************************************/
VOID SDRAM_GPIO_Configuration()
{
    GPIO_Init gpio;

    /* Enable GPIOs clock */
    RCC_AHB1_Periph_Clock_Enable(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD |
                         RCC_AHB1Periph_GPIOE | RCC_AHB1Periph_GPIOF | RCC_AHB1Periph_GPIOG, ENABLE);
                            
    /* Common GPIO configuration */
    gpio.GPIO_Mode  = GPIO_Mode_AF;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_PuPd  = GPIO_PuPd_NOPULL;

    /* GPIOB configuration */
    GPIO_Pin_Configuration(GPIOB_BASE_ADDRESS, GPIO_PinSource5 , GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOB_BASE_ADDRESS, GPIO_PinSource6 , GPIO_AF_FMC);
    gpio.GPIO_Pin = GPIO_Pin_5  | GPIO_Pin_6;      
    GPIO_Initialization(GPIOB_BASE_ADDRESS, &gpio);    

    /* GPIOC configuration */
    GPIO_Pin_Configuration(GPIOC_BASE_ADDRESS, GPIO_PinSource0 , GPIO_AF_FMC);
    gpio.GPIO_Pin = GPIO_Pin_0;      
    GPIO_Initialization(GPIOC_BASE_ADDRESS, &gpio);    

    /* GPIOD configuration */
    GPIO_Pin_Configuration(GPIOD_BASE_ADDRESS, GPIO_PinSource0, GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOD_BASE_ADDRESS, GPIO_PinSource1, GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOD_BASE_ADDRESS, GPIO_PinSource8, GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOD_BASE_ADDRESS, GPIO_PinSource9, GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOD_BASE_ADDRESS, GPIO_PinSource10, GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOD_BASE_ADDRESS, GPIO_PinSource14, GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOD_BASE_ADDRESS, GPIO_PinSource15, GPIO_AF_FMC);
    gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1  | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_Initialization(GPIOD_BASE_ADDRESS, &gpio);

    /* GPIOE configuration */
    GPIO_Pin_Configuration(GPIOE_BASE_ADDRESS, GPIO_PinSource0 , GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOE_BASE_ADDRESS, GPIO_PinSource1 , GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOE_BASE_ADDRESS, GPIO_PinSource7 , GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOE_BASE_ADDRESS, GPIO_PinSource8 , GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOE_BASE_ADDRESS, GPIO_PinSource9 , GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOE_BASE_ADDRESS, GPIO_PinSource10 , GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOE_BASE_ADDRESS, GPIO_PinSource11 , GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOE_BASE_ADDRESS, GPIO_PinSource12 , GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOE_BASE_ADDRESS, GPIO_PinSource13 , GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOE_BASE_ADDRESS, GPIO_PinSource14 , GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOE_BASE_ADDRESS, GPIO_PinSource15 , GPIO_AF_FMC);
    gpio.GPIO_Pin = GPIO_Pin_0  | GPIO_Pin_1  | GPIO_Pin_7 | GPIO_Pin_8  | GPIO_Pin_9  | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_Initialization(GPIOE_BASE_ADDRESS, &gpio);

    /* GPIOF configuration */
    GPIO_Pin_Configuration(GPIOF_BASE_ADDRESS, GPIO_PinSource0 , GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOF_BASE_ADDRESS, GPIO_PinSource1 , GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOF_BASE_ADDRESS, GPIO_PinSource2 , GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOF_BASE_ADDRESS, GPIO_PinSource3 , GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOF_BASE_ADDRESS, GPIO_PinSource4 , GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOF_BASE_ADDRESS, GPIO_PinSource5 , GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOF_BASE_ADDRESS, GPIO_PinSource11 , GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOF_BASE_ADDRESS, GPIO_PinSource12 , GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOF_BASE_ADDRESS, GPIO_PinSource13 , GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOF_BASE_ADDRESS, GPIO_PinSource14 , GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOF_BASE_ADDRESS, GPIO_PinSource15 , GPIO_AF_FMC);
    gpio.GPIO_Pin = GPIO_Pin_0  | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3  | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;      
    GPIO_Initialization(GPIOF_BASE_ADDRESS, &gpio);

    /* GPIOG configuration */
    GPIO_Pin_Configuration(GPIOG_BASE_ADDRESS, GPIO_PinSource0 , GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOG_BASE_ADDRESS, GPIO_PinSource1 , GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOG_BASE_ADDRESS, GPIO_PinSource4 , GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOG_BASE_ADDRESS, GPIO_PinSource5 , GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOG_BASE_ADDRESS, GPIO_PinSource8 , GPIO_AF_FMC);
    GPIO_Pin_Configuration(GPIOG_BASE_ADDRESS, GPIO_PinSource15 , GPIO_AF_FMC);
    gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_8 | GPIO_Pin_15; 
    GPIO_Initialization(GPIOG_BASE_ADDRESS, &gpio);    
}

/*************************************************************************
*
*   FUNCTION
*
*       SDRAM_Init_Sequence
*
*   DESCRIPTION
*
*       This function does SDRAM Init sequence 
*
*   INPUT
*
*       None  
*
*   OUTPUT
*
*       None  
*
*************************************************************************/
VOID SDRAM_Init_Sequence()
{
    FMC_SDRAM_Command_Type fmc_sdram_command;
    UINT32 temp = 0;

    /* Configure a clock configuration enable command */
    fmc_sdram_command.FMC_CommandMode = FMC_Command_Mode_CLK_Enabled;
    fmc_sdram_command.FMC_CommandTarget = FMC_Command_Target_bank2;
    fmc_sdram_command.FMC_AutoRefreshNumber = 1;
    fmc_sdram_command.FMC_ModeRegisterDefinition = 0;
    /* Wait until the SDRAM controller is ready */ 
    while(FMC_Get_Flag_Status(FMC_Bank2_SDRAM, FMC_FLAG_Busy) != RESET)
    {
    
    }
    /* Send the command */
    FMC_SDRAM_Command_Configuratoin(&fmc_sdram_command);  

    /* Insert 100 ms delay */
    ESAL_PR_Delay_USec(100000);

    /* Configure a PALL (precharge all) command */ 
    fmc_sdram_command.FMC_CommandMode = FMC_Command_Mode_PALL;
    fmc_sdram_command.FMC_CommandTarget = FMC_Command_Target_bank2;
    fmc_sdram_command.FMC_AutoRefreshNumber = 1;
    fmc_sdram_command.FMC_ModeRegisterDefinition = 0;

    /* Wait until the SDRAM controller is ready */ 
    while(FMC_Get_Flag_Status(FMC_Bank2_SDRAM, FMC_FLAG_Busy) != RESET)
    {

    }

    /* Send the command */
    FMC_SDRAM_Command_Configuratoin(&fmc_sdram_command);

    /* Configure a Auto-Refresh command */ 
    fmc_sdram_command.FMC_CommandMode = FMC_Command_Mode_AutoRefresh;
    fmc_sdram_command.FMC_CommandTarget = FMC_Command_Target_bank2;
    fmc_sdram_command.FMC_AutoRefreshNumber = 4;
    fmc_sdram_command.FMC_ModeRegisterDefinition = 0;
    /* Wait until the SDRAM controller is ready */ 
    while(FMC_Get_Flag_Status(FMC_Bank2_SDRAM, FMC_FLAG_Busy) != RESET)
    {

    }

    /* Send the  first command */
    FMC_SDRAM_Command_Configuratoin(&fmc_sdram_command);

    /* Wait until the SDRAM controller is ready */ 
    while(FMC_Get_Flag_Status(FMC_Bank2_SDRAM, FMC_FLAG_Busy) != RESET)
    {

    }

    /* Send the second command */
    FMC_SDRAM_Command_Configuratoin(&fmc_sdram_command);

    /* Program the external memory mode register */
    temp = (UINT32)SDRAM_MODEREG_BURST_LENGTH_2          |
                   SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL   |
                   SDRAM_MODEREG_CAS_LATENCY_3           |
                   SDRAM_MODEREG_OPERATING_MODE_STANDARD |
                   SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

    /* Configure a load Mode register command*/ 
    fmc_sdram_command.FMC_CommandMode = FMC_Command_Mode_LoadMode;
    fmc_sdram_command.FMC_CommandTarget = FMC_Command_Target_bank2;
    fmc_sdram_command.FMC_AutoRefreshNumber = 1;
    fmc_sdram_command.FMC_ModeRegisterDefinition = temp;

    /* Wait until the SDRAM controller is ready */ 
    while(FMC_Get_Flag_Status(FMC_Bank2_SDRAM, FMC_FLAG_Busy) != RESET)
    {

    }

    /* Send the command */
    FMC_SDRAM_Command_Configuratoin(&fmc_sdram_command);

    /* Set the device refresh counter */
    FMC_Set_Refresh_Count(REFRESH_COUNTER);

    /* Wait until the SDRAM controller is ready */ 
    while(FMC_Get_Flag_Status(FMC_Bank2_SDRAM, FMC_FLAG_Busy) != RESET)
    {

    }
}

