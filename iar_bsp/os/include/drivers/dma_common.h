/*************************************************************************
*
*            Copyright 2012 Mentor Graphics Corporation
*                      All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       dma_common.h
*
* COMPONENT
*
*       DMA Driver - Nucleus DMA Driver
*
* DESCRIPTION
*
*       This file contains common code for Nucleus DMA driver.
*
*************************************************************************/

/* Check to avoid multiple file inclusion. */
#ifndef     DMA_COMMON_H
#define     DMA_COMMON_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

        
/* DMA device instance handler. */
typedef struct  _dma_instance_handle_struct
{
    DV_DEV_ID               dev_id;                 /* Device ID. */
    UINT32                  dma_io_addr;            /* DMA controller base address. */
    INT                     dma_vector;             /* DMA interrupt vector ID. */
    UINT32                  dma_irq_priority;       /* DMA interrupt priority. */
    ESAL_GE_INT_TRIG_TYPE   dma_irq_type;           /* DMA interrupt type. */
    NU_HISR                 dma_hisr;               /* DMA HISR controller block. */
    VOID                    *dma_hisr_stk_ptr;      /* DMA HISR stack pointer. */
    VOID                    (*cmp_callback)(DMA_CHANNEL *, STATUS );    /* DMA completion callback. */
    DMA_CHANNEL             *chan_req_first;        /* Transfer request channel list first element pointer. */
    DMA_CHANNEL             *chan_req_last;         /* Transfer request channel list last element pointer. */
    BOOLEAN                 device_in_use;          /* Flag to indicate device status. */
    VOID                    *dma_tgt_handle;        /* Pointer for target specific controller block pointer. */
    UINT8                   dma_dev_id;             /* DMA device ID as mentioned in the platform file. */


} DMA_INSTANCE_HANDLE;


/* DMA Hisr stack size */
#define DMA_HISR_STK_SIZE    CFG_NU_OS_DRVR_DMA_HISR_STACK_SIZE

/* Function prototypes for DMA target specific driver. */
STATUS    DMA_PR_Int_Enable(DMA_INSTANCE_HANDLE  *inst_ptr);
STATUS    DMA_PR_Int_Disable(DMA_INSTANCE_HANDLE  *inst_ptr);
VOID      DMA_Add_Chan_Req(DMA_INSTANCE_HANDLE  *inst_ptr, DMA_CHANNEL * chan_req);
DMA_CHANNEL * DMA_Remove_Chan_Req(DMA_INSTANCE_HANDLE  *inst_ptr, UINT8 hw_chan_id);
VOID      DMA_Trans_Complete(DMA_INSTANCE_HANDLE  *inst_ptr, UINT8 hw_chan_id, STATUS status);
VOID      DMA_Tgt_LISR (INT vector);

#ifdef          __cplusplus
}
#endif /* _cplusplus */


#endif      /* !DMA_COMMON_H */

