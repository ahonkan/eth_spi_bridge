/*
 * Avinash Honkan   Terabit Radios
 * 23 Jul 2014
 *
 * DMA setup code for DMA transfer specific parameters
 * Combined DMA configuration in one file for tracking.
 */


/* Include required header files */
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "drivers/dma.h"
#include "drivers/dma_common.h"
#include "bsp/drivers/dma/dma_tgt.h"

#if defined(CFG_IAR_STM32F429II_SK_DMA0_ENABLE)

/***********************************************************************
*
*   FUNCTION
*
*       iar_stm32f429ii_sk_dma0_setup
*
*   DESCRIPTION
*
*       This function sets-up the DMA transfer parameters
*
*   CALLED BY
*
*       Device manager thread
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       DMA_TGT_HANDLE            Parameters specified in this structure
*                       
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
ESAL_TS_WEAK_DEF(VOID iar_stm32f429ii_sk_dma0_setup (DMA_TGT_HANDLE  *tgt_ptr))
{

    tgt_ptr->PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    tgt_ptr->MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    tgt_ptr->Mode = DMA_NORMAL;
    tgt_ptr->Priority = DMA_PRIORITY_LOW;

}

/***********************************************************************
*
*   FUNCTION
*
*       iar_stm32f429ii_sk_dma0_cleanup
*
*   DESCRIPTION
*
*       This function cleans up the target for DMA driver access.
*
*   CALLED BY
*
*       SPI_Driver_Register
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
ESAL_TS_WEAK_DEF(VOID iar_stm32f429ii_sk_dma0_cleanup (VOID))
{
}

#endif

#if defined(CFG_IAR_STM32F429II_SK_DMA1_ENABLE)

/***********************************************************************
*
*   FUNCTION
*
*       iar_stm32f429ii_sk_dma1_setup
*
*   DESCRIPTION
*
*       This function sets-up the DMA transfer parameters
*
*   CALLED BY
*
*       Device manager thread
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       DMA_TGT_HANDLE            Parameters specified in this structure
*                       
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
ESAL_TS_WEAK_DEF(VOID iar_stm32f429ii_sk_dma1_setup (DMA_TGT_HANDLE  *tgt_ptr))
{

    tgt_ptr->PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    tgt_ptr->MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    tgt_ptr->Mode = DMA_NORMAL;
    tgt_ptr->Priority = DMA_PRIORITY_LOW;

}

/***********************************************************************
*
*   FUNCTION
*
*       iar_stm32f429ii_sk_dma1_cleanup
*
*   DESCRIPTION
*
*       This function cleans up the target for DMA driver access.
*
*   CALLED BY
*
*       SPI_Driver_Register
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
ESAL_TS_WEAK_DEF(VOID iar_stm32f429ii_sk_dma1_cleanup (VOID))
{
}

#endif

#if defined(CFG_IAR_STM32F429II_SK_DMA2_ENABLE)


/***********************************************************************
*
*   FUNCTION
*
*       iar_stm32f429ii_sk_dma2_setup
*
*   DESCRIPTION
*
*       This function sets-up the DMA transfer parameters
*
*   CALLED BY
*
*       Device manager thread
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       DMA_TGT_HANDLE            Parameters specified in this structure
*                       
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
ESAL_TS_WEAK_DEF(VOID iar_stm32f429ii_sk_dma2_setup (DMA_TGT_HANDLE  *tgt_ptr))
{

    tgt_ptr->PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    tgt_ptr->MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    tgt_ptr->Mode = DMA_NORMAL;
    tgt_ptr->Priority = DMA_PRIORITY_LOW;

}

/***********************************************************************
*
*   FUNCTION
*
*       iar_stm32f429ii_sk_dma2_cleanup
*
*   DESCRIPTION
*
*       This function cleans up the target for DMA driver access.
*
*   CALLED BY
*
*       SPI_Driver_Register
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
ESAL_TS_WEAK_DEF(VOID iar_stm32f429ii_sk_dma2_cleanup (VOID))
{
}

#endif

#if defined(CFG_IAR_STM32F429II_SK_DMA3_ENABLE)



/***********************************************************************
*
*   FUNCTION
*
*       iar_stm32f429ii_sk_dma3_setup
*
*   DESCRIPTION
*
*       This function sets-up the DMA transfer parameters
*
*   CALLED BY
*
*       Device manager thread
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       DMA_TGT_HANDLE            Parameters specified in this structure
*                       
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
ESAL_TS_WEAK_DEF(VOID iar_stm32f429ii_sk_dma3_setup (DMA_TGT_HANDLE  *tgt_ptr))
{

    tgt_ptr->PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    tgt_ptr->MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    tgt_ptr->Mode = DMA_NORMAL;
    tgt_ptr->Priority = DMA_PRIORITY_LOW;

}

/***********************************************************************
*
*   FUNCTION
*
*       iar_stm32f429ii_sk_dma3_cleanup
*
*   DESCRIPTION
*
*       This function cleans up the target for DMA driver access.
*
*   CALLED BY
*
*       SPI_Driver_Register
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
ESAL_TS_WEAK_DEF(VOID iar_stm32f429ii_sk_dma3_cleanup (VOID))
{
}

#endif

#if defined(CFG_IAR_STM32F429II_SK_DMA4_ENABLE)
/***********************************************************************
*
*   FUNCTION
*
*       iar_stm32f429ii_sk_dma4_setup
*
*   DESCRIPTION
*
*       This function sets-up the DMA transfer parameters
*
*   CALLED BY
*
*       Device manager thread
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       DMA_TGT_HANDLE            Parameters specified in this structure
*                       
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
ESAL_TS_WEAK_DEF(VOID iar_stm32f429ii_sk_dma4_setup (DMA_TGT_HANDLE  *tgt_ptr))
{

    tgt_ptr->PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    tgt_ptr->MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    tgt_ptr->Mode = DMA_NORMAL;
    tgt_ptr->Priority = DMA_PRIORITY_LOW;

}

/***********************************************************************
*
*   FUNCTION
*
*       iar_stm32f429ii_sk_dma4_cleanup
*
*   DESCRIPTION
*
*       This function cleans up the target for DMA driver access.
*
*   CALLED BY
*
*       SPI_Driver_Register
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
ESAL_TS_WEAK_DEF(VOID iar_stm32f429ii_sk_dma4_cleanup (VOID))
{
}

#endif


#if defined(CFG_IAR_STM32F429II_SK_DMA5_ENABLE)
/***********************************************************************
*
*   FUNCTION
*
*       iar_stm32f429ii_sk_dma5_setup
*
*   DESCRIPTION
*
*       This function sets-up the DMA transfer parameters
*
*   CALLED BY
*
*       Device manager thread
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       DMA_TGT_HANDLE            Parameters specified in this structure
*                       
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
ESAL_TS_WEAK_DEF(VOID iar_stm32f429ii_sk_dma5_setup (DMA_TGT_HANDLE  *tgt_ptr))
{

    tgt_ptr->PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    tgt_ptr->MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    tgt_ptr->Mode = DMA_NORMAL;
    tgt_ptr->Priority = DMA_PRIORITY_LOW;

}

/***********************************************************************
*
*   FUNCTION
*
*       iar_stm32f429ii_sk_dma5_cleanup
*
*   DESCRIPTION
*
*       This function cleans up the target for DMA driver access.
*
*   CALLED BY
*
*       SPI_Driver_Register
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
ESAL_TS_WEAK_DEF(VOID iar_stm32f429ii_sk_dma5_cleanup (VOID))
{
}

#endif

/*----------------------------------- EOF------------------------------------*/

