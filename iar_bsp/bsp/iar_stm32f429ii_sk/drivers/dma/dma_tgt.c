/*
 * Avinash Honkan   Terabit Radios
 * 17 Jul 2014
 *
 * Modified code from Mentor FAE (Stuart) to fit the STM device.
 * Utilizing code from STM Firmware SDK.
 *
 */
/* 
 * TODO:
 * Figure out what the pmi_dev does and how power management works with
 * the dma code. It may be better to remove all PMI related code and then
 * rebuild it up.  Right now the codes is ifdef-ed out.
 *
 * Also figure out what open_mode means and how it's used.  The code
 * base received from Stuart (Mentor FAE) has it defined to 1.
 *
 */

/*************************************************************************
*
*               Copyright 2012 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   FILE NAME
*
*       dma_tgt.c
*
*   COMPONENT
*
*       EDMA                             - EDMA3 controller driver
*
*   DESCRIPTION
*
*       This file contains the DMA Driver specific functions.
*
*   FUNCTIONS
*
*       nu_bsp_drvr_dma_edma_init
*       DMA_Tgt_Register
*       DMA_Tgt_Unregister
*       DMA_Tgt_Open
*       DMA_Tgt_Close
*       DMA_Tgt_Ioctl
*       DMA_Tgt_Get_Target_Info
*       DMA_Tgt_Setup
*       DMA_Tgt_Enable
*       DMA_Tgt_Disable
*       DMA_Tgt_Data_Trans
*       DMA_Tgt_Configure_Chan
*       DMA_Tgt_LISR
*       DMA_Tgt_HISR
*
*       
*
*   DEPENDENCIES
*
*       string.h
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       nu_drivers.h
*       dma_common.h
*       dma_tgt.h
*
*************************************************************************/

/*********************************/
/* INCLUDE FILES                 */
/*********************************/
#include <string.h>
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "drivers/dma_common.h"
#include "bsp/drivers/dma/dma_tgt.h"


#define DMA_TGT_LISR_ENTRY            //DEBUG_F6_SET
#define DMA_TGT_LISR_EXIT             //DEBUG_F6_CLEAR
#define DMA_TGT_HISR_ENTRY            //DEBUG_F7_SET
#define DMA_TGT_HISR_EXIT             //DEBUG_F7_CLEAR




#define HAL_TIMEOUT_DMA_ABORT    ((UINT32)CFG_NU_OS_KERN_PLUS_CORE_TICKS_PER_SEC * 1)


/* Stream and channel constant definitions for DMA */
const UINT16 DMA_Stream_Map[DMA_STREAM_COUNT][DMA_CHANNEL_COUNT] = 
{
/* DMA 1 */        
/* Stream 0  */  { CHAN_0 | DMA_SPI3_RX,  CHAN_1 | DMA_I2C1_RX,   CHAN_2 | DMA_TIM4_CH1,    CHAN_3 | DMA_I2S3_EXT_RX,
                   CHAN_4 | DMA_UART5_RX, CHAN_5 | DMA_UART8_TX,  CHAN_6 | DMA_TIM5_CH3,    CHAN_6 | DMA_TIM5_UP},

/* Stream 1  */  { CHAN_3 | DMA_TIM2_UP,  CHAN_3 | DMA_TIM2_CH3,  CHAN_4 | DMA_USART3_RX,   CHAN_5 | DMA_UART7_TX,
                   CHAN_6 | DMA_TIM5_CH4, CHAN_6 | DMA_TIM5_TRIG, CHAN_7 | DMA_TIM6_UP},

/* Stream 2  */  { CHAN_0 | DMA_SPI3_RX,  CHAN_1 | DMA_TIM7_UP,   CHAN_2 | DMA_I2S3_EXT_RX, CHAN_3 | DMA_I2C3_RX,
                   CHAN_4 | DMA_UART4_RX, CHAN_5 | DMA_TIM3_CH4,  CHAN_5 | DMA_TIM3_UP,     CHAN_6 | DMA_TIM5_CH1,
                   CHAN_7 | DMA_I2C2_RX},

/* Stream 3  */  { CHAN_0 | DMA_SPI2_RX,  CHAN_2 | DMA_TIM4_CH2,  CHAN_3 | DMA_I2S2_EXT_RX, CHAN_4 | DMA_USART3_TX,
                   CHAN_5 | DMA_UART7_RX, CHAN_6 | DMA_TIM5_CH4,  CHAN_6 | DMA_TIM5_TRIG,   CHAN_7 | DMA_I2C2_RX},

/* Stream 4  */  { CHAN_0 | DMA_SPI2_TX,  CHAN_1 | DMA_TIM7_UP,   CHAN_2 | DMA_I2S2_EXT_TX, CHAN_3 | DMA_I2C3_TX,
                   CHAN_4 | DMA_UART4_TX, CHAN_5 | DMA_TIM3_CH1,  CHAN_5 | DMA_TIM3_TRIG,   CHAN_6 | DMA_TIM5_CH2,
                   CHAN_7 | DMA_USART3_TX},

/* Stream 5  */  { CHAN_0 | DMA_SPI3_TX,   CHAN_1 | DMA_I2C1_RX,   CHAN_2 | DMA_I2S3_EXT_TX, CHAN_3 | DMA_TIM2_CH1,
                   CHAN_4 | DMA_USART2_RX, CHAN_5 | DMA_TIM3_CH2,  CHAN_7 | DMA_DAC1},

/* Stream 6  */  { CHAN_1 | DMA_I2C1_TX,   CHAN_2 | DMA_TIM4_UP,   CHAN_3 | DMA_TIM2_CH2, CHAN_3 | DMA_TIM2_CH4,
                   CHAN_4 | DMA_USART2_TX, CHAN_5 | DMA_UART8_RX,  CHAN_6 | DMA_TIM5_UP,  CHAN_7 | DMA_DAC2},

/* Stream 7  */  { CHAN_0 | DMA_SPI3_TX,   CHAN_1 | DMA_I2C1_TX,   CHAN_2 | DMA_TIM4_CH3, CHAN_3 | DMA_TIM2_UP, 
                   CHAN_3 | DMA_TIM2_CH4,  CHAN_4 | DMA_UART5_TX,  CHAN_5 | DMA_TIM3_CH3, CHAN_7 | DMA_I2C2_TX},


/* DMA 2 */        
/* Stream 0  */  { CHAN_0 | DMA_ADC1,       CHAN_2 | DMA_ADC2,      CHAN_3 | DMA_SPI1_RX,   CHAN_4 | DMA_SPI4_RX,
                   CHAN_6 | DMA_TIM1_TRIG},

/* Stream 1  */  { CHAN_0 | DMA_SAI1_A,     CHAN_1 | DMA_DCMI,      CHAN_2 | DMA_ADC3,      CHAN_4 | DMA_SPI4_TX,
                   CHAN_5 | DMA_USART6_RX,  CHAN_6 | DMA_TIM1_CH1,  CHAN_7 | DMA_TIM8_UP},

/* Stream 2  */  { CHAN_0 | DMA_TIM8_CH1,   CHAN_0 | DMA_TIM8_CH2,  CHAN_0 | DMA_TIM8_CH3,  CHAN_1 | DMA_ADC2,
                   CHAN_3 | DMA_SPI1_RX,    CHAN_4 | DMA_USART1_RX, CHAN_5 | DMA_USART6_RX, CHAN_6 | DMA_TIM1_CH2,
                   CHAN_7 | DMA_TIM8_CH1},

/* Stream 3  */  { CHAN_0 | DMA_SAI1_A,     CHAN_1 | DMA_ADC2,      CHAN_2 | DMA_SPI5_RX,   CHAN_3 | DMA_SPI1_TX,
                   CHAN_4 | DMA_SDIO,       CHAN_5 | DMA_SPI4_RX,   CHAN_6 | DMA_TIM1_CH1,  CHAN_7 | DMA_TIM8_CH2},

/* Stream 4  */  { CHAN_0 | DMA_ADC1,       CHAN_1 | DMA_SAI1_B,    CHAN_2 | DMA_SPI5_TX,   CHAN_5 | DMA_SPI4_TX,
                   CHAN_6 | DMA_TIM1_CH4,   CHAN_6 | DMA_TIM1_TRIG, CHAN_6 | DMA_TIM1_COM,  CHAN_7 | DMA_TIM8_CH3},

/* Stream 5  */  { CHAN_0 | DMA_SAI1_B,     CHAN_1 | DMA_SPI6_TX,   CHAN_2 | DMA_CRYP_OUT,  CHAN_3 | DMA_SPI1_TX,
                   CHAN_4 | DMA_USART1_RX,  CHAN_6 | DMA_TIM1_UP,   CHAN_6 | DMA_TIM1_COM,  CHAN_7 | DMA_SPI5_RX},

/* Stream 6  */  { CHAN_0 | DMA_TIM1_CH1,   CHAN_0 | DMA_TIM1_CH2,  CHAN_0 | DMA_TIM1_CH3,  CHAN_1 | DMA_SPI6_RX,
                   CHAN_2 | DMA_CRYP_IN,    CHAN_4 | DMA_SDIO,      CHAN_5 | DMA_USART6_TX, CHAN_6 | DMA_TIM1_CH3,
                   CHAN_7 | DMA_SPI5_TX},

/* Stream 7  */  { CHAN_1 | DMA_DCMI,       CHAN_2 | DMA_HASH_IN,   CHAN_4 | DMA_USART1_TX, CHAN_5 | DMA_USART6_TX,  
                   CHAN_7 | DMA_TIM8_CH4,   CHAN_7 | DMA_TIM8_TRIG, CHAN_7 | DMA_TIM8_COM},

};


#define DMA_GET_FLAGS(_DMA_DEV_ID, _FLAGS)                        \
    do                                                            \
    {                                                             \
      switch(_DMA_DEV_ID)                                         \
      {                                                           \
        case  0:                                                  \
        case  8: _FLAGS = (dma->LISR >> 0 ) & 0xFF; break;        \
        case  1:                                                  \
        case  9: _FLAGS = (dma->LISR >> 6 ) & 0xFF; break;        \
        case  2:                                                  \
        case 10: _FLAGS = (dma->LISR >> 16) & 0xFF; break;        \
        case  3:                                                  \
        case 11: _FLAGS = (dma->LISR >> 22) & 0xFF; break;        \
        case  4:                                                  \
        case 12: _FLAGS = (dma->HISR >> 0 ) & 0xFF; break;        \
        case  5:                                                  \
        case 13: _FLAGS = (dma->HISR >> 6 ) & 0xFF; break;        \
        case  6:                                                  \
        case 14: _FLAGS = (dma->HISR >> 16) & 0xFF; break;        \
        case  7:                                                  \
        case 15: _FLAGS = (dma->HISR >> 22) & 0xFF; break;        \
                                                                  \
        default:  while(1);                                       \
      }                                                           \
    } while(0)
      


#define DMA_CLEAR_FLAGS(_DMA_DEV_ID, _FLAGS)                      \
    do                                                            \
    {                                                             \
      switch(_DMA_DEV_ID)                                         \
      {                                                           \
        case  0:                                                  \
        case  8: dma->LIFCR = (_FLAGS << 0 ); break;              \
        case  1:                                                  \
        case  9: dma->LIFCR = (_FLAGS << 6 ); break;              \
        case  2:                                                  \
        case 10: dma->LIFCR = (_FLAGS << 16); break;              \
        case  3:                                                  \
        case 11: dma->LIFCR = (_FLAGS << 22); break;              \
        case  4:                                                  \
        case 12: dma->HIFCR = (_FLAGS << 0 ); break;              \
        case  5:                                                  \
        case 13: dma->HIFCR = (_FLAGS << 6 ); break;              \
        case  6:                                                  \
        case 14: dma->HIFCR = (_FLAGS << 16); break;              \
        case  7:                                                  \
        case 15: dma->HIFCR = (_FLAGS << 22); break;              \
                                                                  \
        default:  while(1);                                       \
      }                                                           \
    } while (0)

static STATUS DMA_Tgt_Get_Target_Info(const CHAR * key, DMA_INSTANCE_HANDLE *inst_info);

/*********************************/
/* FUNCTION PROTOTYPES           */
/*********************************/
static STATUS  DMA_Tgt_Register (const CHAR *key, DMA_INSTANCE_HANDLE *instance_handle);
static STATUS  DMA_Tgt_Unregister (const CHAR *key, DV_DEV_ID dev_id);
static STATUS  DMA_Tgt_Open(VOID *instance_handle, DV_DEV_LABEL label_list[],
                       INT label_cnt, VOID* *session_handle);
static STATUS  DMA_Tgt_Close(VOID *sess_handle);
static STATUS  DMA_Tgt_Ioctl(VOID *session_ptr, INT ioctl_cmd, VOID *data, INT length);

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
extern VOID     DMA_Tgt_Pwr_Default_State (DMA_INSTANCE_HANDLE *inst_handle);
extern STATUS   DMA_Tgt_Pwr_Set_State (VOID *inst_handle, PM_STATE_ID *state);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)
extern STATUS   DMA_Tgt_Pwr_Min_OP_Pt_Calc (DMA_INSTANCE_HANDLE *inst_handle, UINT8* min_op_pt);
extern STATUS   DMA_Tgt_Pwr_Notify_Park1(VOID *instance_handle);
extern STATUS   DMA_Tgt_Pwr_Notify_Park2(VOID *instance_handle);
extern STATUS   DMA_Tgt_Pwr_Notify_Resume1(VOID *instance_handle);
extern STATUS   DMA_Tgt_Pwr_Notify_Resume2(VOID *instance_handle);
extern STATUS   DMA_Tgt_Pwr_Notify_Resume3(VOID *instance_handle);
#endif
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

/***********************************************************************
*
*   FUNCTION
*
*       nu_bsp_drvr_dma_init
*
*   DESCRIPTION
*
*       EDMA entry function
*
*   CALLED BY
*
*       System Registry
*
*   CALLS
*
*       DMA_Tgt_Register
*       DMA_Tgt_Unregister
*
*   INPUTS
*
*       CHAR    *key                        - Path to registry
*       INT     startstop                   - Option to Register or Unregister
*
*
*   OUTPUTS
*
*       STATUS
*
***********************************************************************/
VOID nu_bsp_drvr_dma_init(const CHAR * key, INT startstop)
{
    static DV_DEV_ID            dev_id;
    VOID                        (*setup_fn)(VOID  *) = NU_NULL;
    VOID                        (*cleanup_fn)(VOID) = NU_NULL;
    STATUS                      status;
    STATUS                      reg_status;
    DMA_INSTANCE_HANDLE         *inst_handle;
    VOID                        *pointer;
    NU_MEMORY_POOL              *sys_pool_ptr;
    DMA_TGT_HANDLE              *tgt_handle;

    if (key != NU_NULL)
    {
        if (startstop)
        {
            /* Get system memory pool */
            status = NU_System_Memory_Get( &sys_pool_ptr, NU_NULL);

            if(status == NU_SUCCESS)
            {
                /* Allocate a new instance */
                status = NU_Allocate_Memory (sys_pool_ptr, &pointer,
                                             sizeof(DMA_INSTANCE_HANDLE), NU_NO_SUSPEND);
            }
            
            if (status == NU_SUCCESS)
            {
                /* Clear memory block */
                ESAL_GE_MEM_Clear (pointer, sizeof(DMA_INSTANCE_HANDLE));
                inst_handle = (DMA_INSTANCE_HANDLE*)pointer;

                /* Allocate memory for target specific instance */
                status = NU_Allocate_Memory (sys_pool_ptr, &pointer,
                                             sizeof(DMA_TGT_HANDLE), NU_NO_SUSPEND);
            }

            if (status == NU_SUCCESS)
            {
                /* Clear memory block */
                ESAL_GE_MEM_Clear (pointer, sizeof(DMA_TGT_HANDLE));
                tgt_handle = (DMA_TGT_HANDLE*)pointer;
                inst_handle->dma_tgt_handle = tgt_handle;

                /* Get target info */
                status = DMA_Tgt_Get_Target_Info(key, inst_handle);
            }
             
            if (status == NU_SUCCESS)
            {
               /* Get setup function */
               /* If there is a setup function, save it */
               reg_status = REG_Get_UINT32_Value(key, "/setup", (UINT32 *)&setup_fn);

               if (reg_status == NU_SUCCESS && setup_fn != NU_NULL)
               {
                   tgt_handle->setup_func = setup_fn;
               }

               /* Get cleanup function */
               /* If there is a cleanup function, save it */
               reg_status = REG_Get_UINT32_Value(key, "/cleanup", (UINT32 *)&cleanup_fn);

               if (reg_status == NU_SUCCESS && cleanup_fn != NU_NULL)
               {
                   tgt_handle->cleanup_func = cleanup_fn;

                   /* Call the cleanup function */
                   cleanup_fn();
               }
            }

            if (status == NU_SUCCESS)
            {
              status = DMA_Tgt_Register (key, inst_handle);
            }
            else
            {
              /* De-allocate memory. */
              NU_Deallocate_Memory(tgt_handle);
              NU_Deallocate_Memory(inst_handle);
            }
        }
        else
        {
            /* Call the unregister function */
            DMA_Tgt_Unregister (key, dev_id);
        }
    }

}


/*************************************************************************
*
*   FUNCTION
*
*       DMA_Tgt_Register
*
*   DESCRIPTION
*
*       This function registers the hardware and places it in a
*       known, low-power state
*
*   INPUTS
*
*       key                                 - Path to registry
*       instance_handle                     - DMA instance structure
*
*   OUTPUTS
*
*       status                              - NU_SUCCESS or
*                                             DMA_NO_INSTANCE_AVAILABLE or
*                                             DMA_REGISTRY_ERROR or
*                                             DMA_TOO_MANY_LABELS
*
*************************************************************************/
static STATUS  DMA_Tgt_Register (const CHAR *key, DMA_INSTANCE_HANDLE *instance_handle)
{
    INT             status = NU_SUCCESS;
    DV_DEV_LABEL    dma_labels[DMA_TOTAL_LABELS] = {{DMA_LABEL}};
    INT             all_labels_cnt = 1;

    /* DVR function pointers */
    DV_DRV_FUNCTIONS dma_drv_funcs =
    {
        DMA_Tgt_Open,
        DMA_Tgt_Close,
        NU_NULL,
        NU_NULL,
        DMA_Tgt_Ioctl
    };

    /* Build device label using  dma device id. */
    /* this allows us to link the actual device 
     */
    dma_labels[0].data[0] = instance_handle->dma_dev_id;
   
#if 0    
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    
        /******************************************/
        /* PLACE DEVICE IN KNOWN, LOW-POWER STATE */
        /******************************************/
    
        /* Default state */
        DMA_Tgt_Pwr_Default_State (instance_handle);
    
        /********************************/
        /* INITIALIZE AS POWER DEVICE    */
        /********************************/
        status = PMI_Device_Initialize(&(instance_handle->pmi_dev), key, dma_labels,
                                       &all_labels_cnt, 0);
    
        if (status == NU_SUCCESS)
        {
            /* Setup the power device */
            PMI_Device_Setup(instance_handle->pmi_dev, &DMA_Tgt_Pwr_Set_State, DMA_POWER_BASE,
                             DMA_TOTAL_POWER_STATE_COUNT, &(instance_handle->dev_id), (VOID*)instance_handle);
    
#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)
    
            /* Perform DVFS related setup */
            status = PMI_DVFS_Setup(instance_handle->pmi_dev, key, (VOID*)instance_handle,
                                    &DMA_Tgt_Pwr_Notify_Park1, &DMA_Tgt_Pwr_Notify_Park2,
                                    &DMA_Tgt_Pwr_Notify_Resume1, &DMA_Tgt_Pwr_Notify_Resume2,
                                    &DMA_Tgt_Pwr_Notify_Resume3);
#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */
        }
    
#endif /* #ifdef CFG_NU_OS_SVCS_PWR_ENABLE */

#endif        
    /********************************/
    /* REGISTER WITH DM             */
    /********************************/
    status = DVC_Dev_Register((VOID*)instance_handle, dma_labels,
                              all_labels_cnt, &dma_drv_funcs, &(instance_handle->dev_id));

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       DMA_Tgt_Unregister
*
*   DESCRIPTION
*
*       This function unregisters the hardware
*
*   INPUTS
*
*       CHAR         *key                   - Path to registry
*       DV_DEV_ID    *dev_id                - Device ID
*
*   OUTPUTS
*
*       STATUS       status                 - NU_SUCCESS or error code
*
*************************************************************************/
static STATUS   DMA_Tgt_Unregister (const CHAR *key, DV_DEV_ID dev_id)
{
    STATUS               status = NU_SUCCESS;
    DMA_INSTANCE_HANDLE  *inst_handle;

    /* Suppress warnings */
    NU_UNUSED_PARAM(key);

    /*****************************************/
    /* UNREGISTER DEVICE WITH DEVICE MANAGER */
    /*****************************************/
    status = DVC_Dev_Unregister(dev_id, (VOID**)&inst_handle);

    if ((status == NU_SUCCESS) && (inst_handle != NU_NULL))
    {
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

        /* Place the device in low-power state */
        DMA_Tgt_Pwr_Default_State(inst_handle);

//        status = PMI_Device_Unregister(inst_handle->pmi_dev);

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */
        /***********************************/
        /* FREE THE INSTANCE HANDLE */
        /***********************************/
        NU_Deallocate_Memory ((VOID*) inst_handle);
    }

    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       DMA_Tgt_Open
*
*   DESCRIPTION
*
*       This function opens the device.
*
*   INPUTS
*
*       VOID            *instance_handle    - Device ID
*       DV_DEV_LABEL    labels_list[]       - Access mode (label) of open
*       UINT32          labels_cnt          - Number of labels
*       VOID            **session_handle    - Pointer to Pointer of session handle
*
*   OUTPUTS
*
*       STATUS          success             - 0 for success, negative for failure
*
*************************************************************************/
static STATUS DMA_Tgt_Open(VOID *dma_inst_ptr_void, DV_DEV_LABEL label_list[],
                    INT label_cnt, VOID* *session_handle)
{
    DMA_INSTANCE_HANDLE  *dma_inst_ptr = (DMA_INSTANCE_HANDLE*)dma_inst_ptr_void;
    VOID                 *pointer = NU_NULL;
    STATUS               status = NU_SUCCESS;
    INT                  int_level;
    NU_MEMORY_POOL       *sys_pool_ptr;
    UINT32               open_mode_requests = 0;
    DV_DEV_LABEL         dma_label = {DMA_LABEL};

    /* Check if the label list contains the DMA label */
    dma_label.data[0] = dma_inst_ptr->dma_dev_id;
    
    if (DVS_Label_List_Contains (label_list, label_cnt, dma_label) == NU_SUCCESS)
    {
        open_mode_requests |= DMA_OPEN_MODE;
    }
    
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
        /* Call the Power device open function */
        status = PMI_Device_Open (&open_mode_requests, label_list, label_cnt);
#endif

    /* If device is already open return a error. */
    if (!((dma_inst_ptr->device_in_use == NU_TRUE) && (open_mode_requests & DMA_OPEN_MODE)))
    {
        status = NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);

        /* If previous operation is successful */
        if (status == NU_SUCCESS)
        {
            /* Disable interrupts. */
            int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

            /* Allocate memory for DMA hisr stack. */
            status = NU_Allocate_Memory (sys_pool_ptr, &pointer, DMA_HISR_STK_SIZE, NU_NO_SUSPEND);
        }

        if(status == NU_SUCCESS)
        {
          /* Save the stack pointer */
          dma_inst_ptr->dma_hisr_stk_ptr = pointer;

          /* Clear the memory of DMA hist stack. */
          (VOID)memset((VOID*)pointer, 0, DMA_HISR_STK_SIZE);

          /* Create DMA HISR */
          status = NU_Create_HISR (&(dma_inst_ptr->dma_hisr), "DMA_HISR", DMA_Tgt_HISR,
                                   0, pointer, DMA_HISR_STK_SIZE);

          if (status != NU_SUCCESS)
          {
            (VOID)NU_Deallocate_Memory(pointer);
          }
        }

        if(status == NU_SUCCESS)
        {
          /* Store dma instance pointer. */
          dma_inst_ptr->dma_hisr.tc_app_reserved_1 = (UNSIGNED)dma_inst_ptr;

          dma_inst_ptr->device_in_use = NU_TRUE;

          /* Enable DMA controller. */
          DMA_Tgt_Enable(dma_inst_ptr);

          /* Configure the transfer properties of the device */
          DMA_Tgt_Setup(dma_inst_ptr);

          /* Enable processor level interrupts, register ISR */
          DMA_PR_Int_Enable(dma_inst_ptr);

          /* Update mode requests in instance handle */
//          dma_inst_ptr->open_modes |= open_mode_requests; 

          /* Place instance pointer in handle that can be returned */
          *session_handle = dma_inst_ptr;
        }

        /* Restore interrupts to previous level */
        NU_Local_Control_Interrupts(int_level);
    }
    else
    {
        /* Already opened. */
        status = NU_DMA_ALREADY_OPEN;
    }

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       DMA_Tgt_Close
*
*   DESCRIPTION
*
*       This function closes the device.
*
*   INPUTS
*
*       VOID    *sess_handle                - Session handle of the device
*
*   OUTPUTS
*
*       STATUS   status                     - NU_SUCCESS or error code
*
*************************************************************************/
static STATUS DMA_Tgt_Close(VOID *session_ptr_void)
{
    DMA_INSTANCE_HANDLE     *inst_ptr;
    STATUS                  status = NU_SUCCESS;
    INT                     int_level;

    /* If a valid session, then close it */
    if(session_ptr_void != NU_NULL)
    {
        /* Initialize local variables */
        inst_ptr = (DMA_INSTANCE_HANDLE*)session_ptr_void;

        if(inst_ptr->device_in_use != NU_FALSE)
        {
            /* Disable interrupts before clearing shared variable */
            int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

            /* If the open mode request was ethernet */
//            if (inst_ptr->open_modes & DMA_OPEN_MODE)
//            {
//                /* Set device is closed */
//                inst_ptr->device_in_use = NU_FALSE;
//            }

            /* Disable processor level interrupts */
            DMA_PR_Int_Disable(inst_ptr);

            /* Disable DMA controller. */
            DMA_Tgt_Disable(inst_ptr);

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
            
//            status = PMI_Device_Close((inst_ptr->pmi_dev));
            
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

            /* Delete the DMA HISR */
            NU_Delete_HISR(&(inst_ptr->dma_hisr));

            /* Deallocate memory for DMA HISR stack */
            NU_Deallocate_Memory(inst_ptr->dma_hisr_stk_ptr);

            /* Restore interrupts to previous level */
            NU_Local_Control_Interrupts(int_level);
        }

    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       DMA_Tgt_Ioctl
*
*   DESCRIPTION
*
*       This function provides IOCTL functionality.
*
*   INPUTS
*
*       VOID      *sess_handle_ptr          - Session handle of the driver
*       UINT32    ioctl_cmd                 - Ioctl command
*       VOID      *data                     - Ioctl data pointer
*       UINT32    length                    - Ioctl length
*
*   OUTPUTS
*
*       STATUS    status                    - NU_SUCCESS
*                                           - or error code
*
*************************************************************************/
static STATUS DMA_Tgt_Ioctl(VOID *sess_handle_ptr_void, INT ioctl_cmd, VOID *data, INT length)
{
    DMA_INSTANCE_HANDLE  *inst_ptr = (DMA_INSTANCE_HANDLE*)sess_handle_ptr_void;
    STATUS               status = NU_SUCCESS;
    DV_IOCTL0_STRUCT     *ioctl0;
    DV_DEV_LABEL         dma_label = {DMA_LABEL};

    dma_label.data[0] = inst_ptr->dma_dev_id;
    
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    
//    PMI_DEV_HANDLE            pmi_dev = inst_ptr->pmi_dev;
        
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

    /* Process command */
    switch (ioctl_cmd)
    {
        case DV_IOCTL0:

                if (length == sizeof(DV_IOCTL0_STRUCT))
                {
                    ioctl0 = data;
                    status = DV_IOCTL_INVALID_MODE;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
//                    status = PMI_Device_Ioctl(pmi_dev, ioctl_cmd, data, length, inst_ptr,
//                                              inst_ptr->open_modes);
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

                    if (status != NU_SUCCESS)
                    {
                        /* If the mode requested is supported and if the session was opened for that mode */
//                        if ((DV_COMPARE_LABELS (&(ioctl0->label), &dma_label)) &&
//                            (inst_ptr->open_modes & DMA_OPEN_MODE))
                        if (DV_COMPARE_LABELS (&(ioctl0->label), &dma_label))
                        {
                            ioctl0->base = IOCTL_DMA_BASE;
                            status = NU_SUCCESS;
                        }
                    }
                }

                else
                {
                    status = DV_IOCTL_INVALID_LENGTH;
                }

                break;
                
        case (DMA_ACQUIRE_CHANNEL):

            /* Configure DMA channel. */
            status = DMA_Tgt_Configure_Chan(inst_ptr, (DMA_CHANNEL *) data);

            break;

        case (DMA_RELEASE_CHANNEL):

            /* Reset DMA channel. */
            DMA_Tgt_Reset_Chan(inst_ptr, (DMA_CHANNEL *) data);

            break;

        case (DMA_RESET_CHANNEL):

            /* Reset DMA channel. */
            DMA_Tgt_Reset_Chan(inst_ptr, (DMA_CHANNEL *) data);

            break;

        case (DMA_SET_COMP_CALLBACK):

            /* Save callback function pointer. */
            inst_ptr->cmp_callback = data;

            break;

        case (DMA_DATA_TRANSFER):

            /* Add request in request queue. */
            DMA_Add_Chan_Req(inst_ptr, (DMA_CHANNEL *) data);

            /* Trigger Data transfer. */
            status = DMA_Tgt_Data_Trans(inst_ptr, (DMA_CHANNEL *) data);

            break;
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
                    
        default:

            /* Call the PMI IOCTL function for Power and UII IOCTLs */
//            status = PMI_Device_Ioctl((inst_ptr->pmi_dev), ioctl_cmd, data, length,
//                                      inst_ptr, inst_ptr->open_modes);

            break;
            
#else
            
        default:

            status = DV_INVALID_INPUT_PARAMS;

            break;
            
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       DMA_Tgt_Get_Target_Info
*
*   DESCRIPTION
*
*       This function gets target info from Registry for dma device.  
*       Further dma channel configuration information is provided by
*       specific device in the platform configuration.
*
*   INPUTS
*
*       key                                 - Registry path
*       inst_info                           - pointer to dma instance info structure
*                                             (populated by this function)
*
*   OUTPUTS
*
*       STATUS    status                    - NU_SUCCESS for success
*                                           - DMA_REGISTRY_ERROR for error
*
*************************************************************************/
STATUS DMA_Tgt_Get_Target_Info(const CHAR * key, DMA_INSTANCE_HANDLE *inst_info)
{
    STATUS     reg_status = NU_SUCCESS;
    STATUS     status;
    UINT32     temp32, temp32a;
    UINT8      temp8;
    DMA_TGT_HANDLE  *tgt_ptr = (DMA_TGT_HANDLE  *)inst_info->dma_tgt_handle;
    CHAR       *str_ptr;

    
    if(reg_status == NU_SUCCESS)
    {
        /* Get dma device id. */
        reg_status = REG_Get_UINT8_Value(key, "/tgt_settings/dma_dev_id", &temp8);
        if (reg_status == NU_SUCCESS)
        {
            inst_info->dma_dev_id = temp8;
        }
    }

    if(reg_status == NU_SUCCESS)
    {
        switch(inst_info->dma_dev_id)
        {
          case 0  : temp32 = (UINT32)DMA1_Stream0; temp32a = (UINT32)DMA1; break; 
          case 1  : temp32 = (UINT32)DMA1_Stream1; temp32a = (UINT32)DMA1; break; 
          case 2  : temp32 = (UINT32)DMA1_Stream2; temp32a = (UINT32)DMA1; break; 
          case 3  : temp32 = (UINT32)DMA1_Stream3; temp32a = (UINT32)DMA1; break; 
          case 4  : temp32 = (UINT32)DMA1_Stream4; temp32a = (UINT32)DMA1; break; 
          case 5  : temp32 = (UINT32)DMA1_Stream5; temp32a = (UINT32)DMA1; break; 
          case 6  : temp32 = (UINT32)DMA1_Stream6; temp32a = (UINT32)DMA1; break; 
          case 7  : temp32 = (UINT32)DMA1_Stream7; temp32a = (UINT32)DMA1; break;

          case 8  : temp32 = (UINT32)DMA2_Stream0; temp32a = (UINT32)DMA2; break; 
          case 9  : temp32 = (UINT32)DMA2_Stream1; temp32a = (UINT32)DMA2; break; 
          case 10 : temp32 = (UINT32)DMA2_Stream2; temp32a = (UINT32)DMA2; break; 
          case 11 : temp32 = (UINT32)DMA2_Stream3; temp32a = (UINT32)DMA2; break; 
          case 12 : temp32 = (UINT32)DMA2_Stream4; temp32a = (UINT32)DMA2; break; 
          case 13 : temp32 = (UINT32)DMA2_Stream5; temp32a = (UINT32)DMA2; break; 
          case 14 : temp32 = (UINT32)DMA2_Stream6; temp32a = (UINT32)DMA2; break; 
          case 15 : temp32 = (UINT32)DMA2_Stream7; temp32a = (UINT32)DMA2; break; 

          default : return NU_DMA_INVALID_PARAM;
        }
        tgt_ptr->dma_stream_io_addr = temp32;
        inst_info->dma_io_addr = temp32a;
    }

    if(reg_status == NU_SUCCESS)
    {
        switch(inst_info->dma_dev_id)
        {
          case 0  : temp32 = ESAL_PR_DMA1_CH0_INT_VECTOR_ID; break; 
          case 1  : temp32 = ESAL_PR_DMA1_CH1_INT_VECTOR_ID; break; 
          case 2  : temp32 = ESAL_PR_DMA1_CH2_INT_VECTOR_ID; break; 
          case 3  : temp32 = ESAL_PR_DMA1_CH3_INT_VECTOR_ID; break; 
          case 4  : temp32 = ESAL_PR_DMA1_CH4_INT_VECTOR_ID; break; 
          case 5  : temp32 = ESAL_PR_DMA1_CH5_INT_VECTOR_ID; break; 
          case 6  : temp32 = ESAL_PR_DMA1_CH6_INT_VECTOR_ID; break; 
          case 7  : temp32 = ESAL_PR_DMA1_CH7_INT_VECTOR_ID; break; 

          case 8  : temp32 = ESAL_PR_DMA2_CH0_INT_VECTOR_ID; break; 
          case 9  : temp32 = ESAL_PR_DMA2_CH1_INT_VECTOR_ID; break; 
          case 10 : temp32 = ESAL_PR_DMA2_CH2_INT_VECTOR_ID; break; 
          case 11 : temp32 = ESAL_PR_DMA2_CH3_INT_VECTOR_ID; break; 
          case 12 : temp32 = ESAL_PR_DMA2_CH4_INT_VECTOR_ID; break; 
          case 13 : temp32 = ESAL_PR_DMA2_CH5_INT_VECTOR_ID; break; 
          case 14 : temp32 = ESAL_PR_DMA2_CH6_INT_VECTOR_ID; break; 
          case 15 : temp32 = ESAL_PR_DMA2_CH7_INT_VECTOR_ID; break; 

          default : return NU_DMA_INVALID_PARAM;
        }
        inst_info->dma_vector = temp32;
    }

//    if (reg_status == NU_SUCCESS)
//    {
//            strncpy(inst_info->dma_ref_clock, CPU_AHBCLK_FREQ, NU_DRVR_REF_CLOCK_LEN);
//    }

    if(reg_status == NU_SUCCESS)
    {
        /* Get dma device id. */
        reg_status = REG_Get_UINT32_Value(key, "/tgt_settings/dma_intr_priority", &inst_info->dma_irq_priority);

    }

    /* Check if no error occurred. */
    if(reg_status == NU_SUCCESS)
    {
        status = NU_SUCCESS;
    }

    else
    {
        status = NU_DMA_REGISTRY_ERROR;
    }

    return status;
}

/**************************************************************************
*
* FUNCTION
*
*       DMA_Tgt_Setup
*
* DESCRIPTION
*
*       This function setups the DMA controller.
*
* INPUTS
*
*       inst_ptr                    - DMA instance handle
*
* OUTPUTS
*
*       None
*
**************************************************************************/
VOID DMA_Tgt_Setup(DMA_INSTANCE_HANDLE  *inst_ptr)
{
    DMA_TGT_HANDLE  *tgt_ptr = (DMA_TGT_HANDLE  *)inst_ptr->dma_tgt_handle;
    VOID            (*setup_fn)(VOID  *) = NU_NULL;
    UINT32          tmp;
    DMA_Stream_TypeDef  *dma_stream = (DMA_Stream_TypeDef*)tgt_ptr->dma_stream_io_addr;

    /* Call setup function if available. */
    setup_fn = tgt_ptr->setup_func;
    if(setup_fn != NU_NULL)
    {
       /* Call the setup function */
       setup_fn(tgt_ptr);
    }
    
    /* Insert dma setup code here */

  tmp = 0;

  /* Prepare the DMA Stream configuration */
  tmp |=  tgt_ptr->PeriphDataAlignment | tgt_ptr->MemDataAlignment |
          tgt_ptr->Mode                | tgt_ptr->Priority;

  /* Write to DMA Stream CR register */
  dma_stream->CR = tmp;  

  /* Implementation decision not to implement FIFO functionality.  To implement it, 
   * look at the stm32f4xx_hal_dma.h and add appropriate elements to the 
   * DMA_TGT_HANDLE
   */

  /* Get the FCR register value */
  tmp = dma_stream->FCR;

  /* Clear Direct mode and FIFO threshold bits */
  tmp &= (UINT32)~(DMA_SxFCR_DMDIS | DMA_SxFCR_FTH);

  /* Write to DMA Stream FCR */
  dma_stream->FCR = tmp;

}

/**************************************************************************
*
* FUNCTION
*
*       DMA_Tgt_Enable
*
* DESCRIPTION
*
*       This function enabled DMA controller
*
* INPUTS
*
*       inst_ptr                    - DMA instance handle
*
* OUTPUTS
*
*       None
*
**************************************************************************/
VOID      DMA_Tgt_Enable (DMA_INSTANCE_HANDLE *inst_ptr)
{
    NU_UNUSED_PARAM(inst_ptr);

    __DMA1_CLK_ENABLE();
    __DMA2_CLK_ENABLE();
}

/**************************************************************************
*
* FUNCTION
*
*       DMA_Tgt_Disable
*
* DESCRIPTION
*
*       This function disables DMA controller
*
* INPUTS
*
*       inst_ptr                    - DMA instance handle
*
* OUTPUTS
*
*       None
*
**************************************************************************/
VOID      DMA_Tgt_Disable (DMA_INSTANCE_HANDLE *inst_ptr)
{
    DMA_TGT_HANDLE      *tgt_handle = (DMA_TGT_HANDLE *) inst_ptr->dma_tgt_handle;
    VOID                (*cleanup_fn)(VOID) = NU_NULL;

    /* Call cleanup function if available. */
    cleanup_fn = tgt_handle->cleanup_func;
    if(cleanup_fn != NU_NULL)
    {
        /* Call the cleanup function */
        cleanup_fn();
    }
}

/*************************************************************************
*
* FUNCTION
*
*       DMA_Tgt_Data_Trans
*
* DESCRIPTION
*
*       This function triggers the data transfer request.
*
* INPUTS
*
*       inst_ptr                    - DMA instance handle
*       chan                        - DMA channel control block pointer
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS DMA_Tgt_Data_Trans(DMA_INSTANCE_HANDLE  *inst_ptr, DMA_CHANNEL * chan)
{
    STATUS          status = NU_SUCCESS;
    UINT32          tmp;
    UINT8           idx;
    UINT16          stm_dma_channel;
    DMA_TGT_HANDLE  *tgt_ptr = (DMA_TGT_HANDLE  *)inst_ptr->dma_tgt_handle;
    DMA_Stream_TypeDef  *dma_stream = (DMA_Stream_TypeDef*)tgt_ptr->dma_stream_io_addr;
    
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
//        PMI_DEV_HANDLE pmi_dev_entry = inst_ptr->pmi_dev;
    
        /* Check current state of the device. If the device is off, suspend on a
           semaphore until the device power state changes to ON */
//        if ((PMI_STATE_GET(pmi_dev_entry) == DMA_OFF)||(PMI_IS_PARKED(pmi_dev_entry) == NU_TRUE))
//        {
            /* Wait until the device is available for a write operation */
//            PMI_WAIT_CYCLE(pmi_dev_entry, status);
//        }
#endif

    /* Enter code to initiate transfer of data */        

    /* Configure  DMA parameters from DMA CHANNEL
     *  STM DMA Data request channel  : peri_id
     *  
     *  STM DMA Data direction    :
     *  If DMA_REQUEST_TYPE is *_SEND, the data flows from memory to periperhal.
     *  If DMA_REQUEST_TYPE is *_RECEIVE, the data flows from periperhal to memory.
     *  It is up to the caller to ensure that the correct data flow type matches
     *  with the peri_id.  Assume the channel (stream) is correctly chosen. 
     *
     *  Peripheral Increment  : DMA_REQ src_add_type or dst_add_type based on 
     *                          DMA_REQUEST_TYPE
     *
     *  Memory Increment      : DMA_REQ src_add_type or dst_add_type based on 
     *                          DMA_REQUEST_TYPE
     */

  /* Disable the peripheral */
  dma_stream->CR &= ~DMA_SxCR_EN;

  /* Get the CR register value */
  tmp = dma_stream->CR;

  /* Find the stm32F dma channel for the specified perip id */
  stm_dma_channel = 0xFFFF;
  for (idx = 0; idx < DMA_CHANNEL_COUNT; idx ++)
  {
    if ( (DMA_Stream_Map[inst_ptr->dma_dev_id][idx] & DMA_PERIPH_MASK) == 
          chan->peri_id)
    {            
      stm_dma_channel = DMA_Stream_Map[inst_ptr->dma_dev_id][idx] & CHAN_MASK;
      break;
    }
  }

  if (stm_dma_channel == 0xFFFF)
	  return NU_DMA_DEVICE_NOT_FOUND;

  switch (stm_dma_channel)
  {
    case CHAN_0: tmp |= DMA_CHANNEL_0; break;
    case CHAN_1: tmp |= DMA_CHANNEL_1; break;
    case CHAN_2: tmp |= DMA_CHANNEL_2; break;
    case CHAN_3: tmp |= DMA_CHANNEL_3; break;
    case CHAN_4: tmp |= DMA_CHANNEL_4; break;
    case CHAN_5: tmp |= DMA_CHANNEL_5; break;
    case CHAN_6: tmp |= DMA_CHANNEL_6; break;
    case CHAN_7: tmp |= DMA_CHANNEL_7; break;
    default:  return NU_DMA_INVALID_CHANNEL;              
  }            

  /* Set up the direction.  _SEND means from memory to periperhal.
   * _RECEIVE is from peripheral to memory
   */
  switch (chan->cur_req_type)
  {
    case DMA_SYNC_SEND:
    case DMA_ASYNC_SEND:  
            tmp |= DMA_MEMORY_TO_PERIPH; 

            if ((chan->cur_req_ptr->src_add_type & DMA_ADDRESS_MASK) == DMA_ADDRESS_INCR) tmp |= DMA_MINC_ENABLE;
            else  tmp |= DMA_MINC_DISABLE;

            if ((chan->cur_req_ptr->dst_add_type & DMA_ADDRESS_MASK) == DMA_ADDRESS_INCR) tmp |= DMA_PINC_ENABLE;
            else  tmp |= DMA_PINC_DISABLE;
            
            /* Configure DMA Stream destination address */
            dma_stream->PAR = (UINT32)chan->cur_req_ptr->dst_ptr;

            /* Configure DMA Stream source address */
            dma_stream->M0AR = (UINT32)chan->cur_req_ptr->src_ptr;

            /* Configure DMA Stream source address second buffer */
            if (chan->cur_req_ptr->src_add_type & DMA_ADDRESS_DOUBLE_BUFFER)
            {
              dma_stream->M1AR = &(chan->cur_req_ptr->src_ptr[chan->cur_req_ptr->length]);
              tmp |= DMA_SxCR_DBM;
            } 
            
            break;

    case DMA_SYNC_RECEIVE:
    case DMA_ASYNC_RECEIVE: 
            tmp |= DMA_PERIPH_TO_MEMORY; 

            if ((chan->cur_req_ptr->dst_add_type & DMA_ADDRESS_MASK) == DMA_ADDRESS_INCR) tmp |= DMA_MINC_ENABLE;
            else  tmp |= DMA_MINC_DISABLE;

            if ((chan->cur_req_ptr->src_add_type & DMA_ADDRESS_MASK) == DMA_ADDRESS_INCR) tmp |= DMA_PINC_ENABLE;
            else  tmp |= DMA_PINC_DISABLE;
            
            /* Configure DMA Stream source address */
            dma_stream->PAR = (UINT32)chan->cur_req_ptr->src_ptr;

            /* Configure DMA Stream destination address */
            dma_stream->M0AR = (UINT32)chan->cur_req_ptr->dst_ptr;
            
            /* Configure DMA Stream source address second buffer */
            if (chan->cur_req_ptr->dst_add_type & DMA_ADDRESS_DOUBLE_BUFFER)
            {                    
              dma_stream->M1AR = (UINT32 *)&(chan->cur_req_ptr->dst_ptr[chan->cur_req_ptr->length]);
              tmp |= DMA_SxCR_DBM;
            }

            break;

    /* Dont handle Mem to Mem transfers as this feature needs 
     * further investigation in driver code implementation
     */
    case DMA_SYNC_MEM_TRANS:
    case DMA_ASYNC_MEM_TRANS:
    default: return NU_DMA_INVALID_COMM_MODE;

  }

  /* Write to DMA Stream CR register */
  dma_stream->CR = tmp;  


    /* Enable interrupts and initiate
     * transfers
     */

  /* Configure the source, destination address and the data length */
  dma_stream->NDTR = chan->cur_req_ptr->length;

  /* Enable the transfer complete interrupt */
  dma_stream->CR |= DMA_IT_TC;

  /* Enable the transfer Error interrupt */
  dma_stream->CR |= DMA_IT_TE;

  /* Enable the direct mode Error interrupt */
  dma_stream->CR |= DMA_IT_DME;

   /* Enable the Peripheral */
  dma_stream->CR |= DMA_SxCR_EN;

  /* Let rest of software know dma is in use */
  tgt_ptr->dmaState = HAL_DMA_STATE_BUSY;

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*       DMA_Tgt_Configure_Chan
*
* DESCRIPTION
*
*       This function configures the specified channel.
*
* INPUTS
*
*       inst_ptr                    - DMA instance handle
*       chan                        - DMA channel control block pointer
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS DMA_Tgt_Configure_Chan(DMA_INSTANCE_HANDLE  *inst_ptr, DMA_CHANNEL * chan)
{
    STATUS status = NU_SUCCESS;
    UINT8   idx;
    DMA_TGT_HANDLE  *tgt_ptr = (DMA_TGT_HANDLE  *)inst_ptr->dma_tgt_handle;

    /* configure channel */

    /* Set hw_chan_id to dma_dev_id as both mean the same thing.
     * In header file, create perpheral id that matches to ref manual dma streams
     * Check if peripheral id matches with dma_dev_id.
     */

    chan->hw_chan_id = inst_ptr->dma_dev_id;

    for (idx = 0; idx < DMA_CHANNEL_COUNT; idx ++)
    {
      if ( (DMA_Stream_Map[inst_ptr->dma_dev_id][idx] & DMA_PERIPH_MASK) == 
           chan->peri_id)
      {
        status = NU_SUCCESS;    

        /* Let rest of software know dma is ready */
        tgt_ptr->dmaState = HAL_DMA_STATE_READY;
        
        break;
      }
      else
        status = NU_DMA_DEVICE_NOT_FOUND;              
    }

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*       DMA_Tgt_Reset_Chan
*
* DESCRIPTION
*
*       This function resets the specified channel.
*
* INPUTS
*
*       inst_ptr                    - DMA instance handle
*       chan                        - DMA channel control block pointer
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS DMA_Tgt_Reset_Chan(DMA_INSTANCE_HANDLE  *inst_ptr, DMA_CHANNEL * chan)
{
    STATUS          status = NU_SUCCESS;
    UINT32          timeout;
    DMA_TGT_HANDLE  *tgt_ptr = (DMA_TGT_HANDLE  *)inst_ptr->dma_tgt_handle;
    DMA_Stream_TypeDef  *dma_stream = (DMA_Stream_TypeDef*)tgt_ptr->dma_stream_io_addr;


  /* Disable the stream */
  dma_stream->CR &=  ~DMA_SxCR_EN;

  /* Get timeout */
  timeout = HAL_TIMEOUT_DMA_ABORT;

  /* Check if the DMA Stream is effectively disabled */
  while(( dma_stream->CR & DMA_SxCR_EN) != 0)
  {
    NU_Sleep(1);          

    /* Check for the Timeout */
    if(timeout == 0)
    {
      status = NU_DMA_RESET_FAIL;
      break;
    }
    timeout --;
  }

  /* Let rest of software know dma went through reset */
  tgt_ptr->dmaState = HAL_DMA_STATE_RESET;

  return status;
}

/**************************************************************************
*
* FUNCTION
*
*       DMA_Tgt_LISR
*
* DESCRIPTION
*
*       This is the LISR for DMA interrupt.
*
* INPUTS
*
*       vector                              Vector that has caused this
*                                           interrupt.
*
* OUTPUTS
*
*       NONE.
*
**************************************************************************/
VOID DMA_Tgt_LISR (INT vector)
{
    DMA_INSTANCE_HANDLE *dma_inst_ptr;
    DMA_TGT_HANDLE      *tgt_ptr;
    DMA_Stream_TypeDef  *dma_stream;

    DMA_TGT_LISR_ENTRY;

    /* Get a pointer to the DMA structure for this vector */
    dma_inst_ptr = (DMA_INSTANCE_HANDLE*) ESAL_GE_ISR_VECTOR_DATA_GET (vector);

    /* Get DMA Target Handle */
    tgt_ptr = (DMA_TGT_HANDLE  *)dma_inst_ptr->dma_tgt_handle;
    dma_stream = (DMA_Stream_TypeDef*)tgt_ptr->dma_stream_io_addr;

    
    /* Check if valid instance pointer. */
    if (dma_inst_ptr != NU_NULL)
    {
        /* Make a copy of the cr register for interrupt handling */            
        tgt_ptr->DMA_SxCR_copy = dma_stream->CR; 

        /* Disable DMA interrupt sources */
        dma_stream->CR &= ~(DMA_IT_TE | DMA_IT_DME | DMA_IT_HT | DMA_IT_TC)  ;
        dma_stream->FCR &= ~DMA_IT_FE;
            
        /* Activate Hisr. */
        NU_Activate_HISR(&dma_inst_ptr->dma_hisr);
    }

    DMA_TGT_LISR_EXIT;

}

/**************************************************************************
*
* FUNCTION
*
*       DMA_Tgt_HISR
*
* DESCRIPTION
*
*       This is the HISR entry function for DMA interrupt.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       None
*
**************************************************************************/
VOID DMA_Tgt_HISR (VOID)
{
    DMA_INSTANCE_HANDLE *dma_inst_ptr;
    NU_HISR             *hcb;
    DMA_TGT_HANDLE      *tgt_ptr;
    DMA_Stream_TypeDef  *dma_stream;
    DMA_TypeDef         *dma;
    UINT32              dma_intr_status = 0;
    STATUS              status = NU_SUCCESS;
    UINT32              cur_req_length_copy;

    DMA_TGT_HISR_ENTRY;
    
    /* Get the HISR control block */
    hcb = (NU_HISR*)NU_Current_HISR_Pointer();

    /* Get the DMA instance pointer from HISR control block */
    dma_inst_ptr = (DMA_INSTANCE_HANDLE *)hcb->tc_app_reserved_1;

    /* Get DMA Target Handle */
    tgt_ptr = (DMA_TGT_HANDLE  *)dma_inst_ptr->dma_tgt_handle;
    dma = (DMA_TypeDef*)dma_inst_ptr->dma_io_addr;
    dma_stream = (DMA_Stream_TypeDef*)tgt_ptr->dma_stream_io_addr;
    

    /* Transfer Error Interrupt management ***************************************/
    DMA_GET_FLAGS(dma_inst_ptr->dma_dev_id, dma_intr_status);              
    if (dma_intr_status & DMA_FLAG_TEIF0_4)
    {
      /*
       * TODO/ TBD:
       * Code to handle Transfer Error:
       */
      tgt_ptr->dmaState = HAL_DMA_STATE_ERROR;
    }

    /* FIFO Error Interrupt management ******************************************/
    DMA_GET_FLAGS(dma_inst_ptr->dma_dev_id, dma_intr_status);              
    if (dma_intr_status & DMA_FLAG_FEIF0_4)
    {
      /*
       * TODO/ TBD:
       * Code to handle FIFO Error:
       */
      tgt_ptr->dmaState = HAL_DMA_STATE_ERROR;
    }
    
    /* Direct Mode Error Interrupt management ***********************************/
    DMA_GET_FLAGS(dma_inst_ptr->dma_dev_id, dma_intr_status);              
    if (dma_intr_status & DMA_FLAG_DMEIF0_4)
    {
      /*
       * TODO/ TBD:
       * Code to handle Direct Mode Error:
       */
      tgt_ptr->dmaState = HAL_DMA_STATE_ERROR;
    }

    /* Half Transfer Complete Interrupt management ******************************/
    /* 
     * Half Transfer Complete use case isn't fully developed. The code here is
     * minimal and needs more to be fully functional.  The HT interrupt is not
     * enabled so this code won't be called.
     */ 
    DMA_GET_FLAGS(dma_inst_ptr->dma_dev_id, dma_intr_status);              
    if ((dma_intr_status & DMA_FLAG_HTIF0_4) && (tgt_ptr->DMA_SxCR_copy & DMA_IT_HT))
    {
        /* Multi_Buffering mode enabled */
        if(dma_stream->CR & DMA_SxCR_DBM)
        {
          /* Current memory buffer used is Memory 0 */
          if((dma_stream->CR & DMA_SxCR_CT) == 0)
          {
            /* Change DMA peripheral state */
            tgt_ptr->dmaState = HAL_DMA_STATE_READY_HALF_MEM0;
          }
          /* Current memory buffer used is Memory 1 */
          else
          {
            /* Change DMA peripheral state */
            tgt_ptr->dmaState = HAL_DMA_STATE_READY_HALF_MEM1;
          }
        }
        else
        {
          /* Disable the half transfer interrupt if the DMA mode is not CIRCULAR */
          /* Circular DMA mode means that data is contiuously read into the 
           * buffer and the pointer circles back to beginning after the specified
           * number of words have been transmitted.
           */

          /* Re-enable the half transfer interrupt if CIRC buffer is enabled*/
          if(dma_stream->CR & DMA_SxCR_CIRC)
            dma_stream->CR |= DMA_IT_HT;
    
          /* Change DMA peripheral state */
          tgt_ptr->dmaState = HAL_DMA_STATE_READY_HALF_MEM0;
        }

        /* Call some callback to indicate half transfer is complete
         * The DMA_Trans_Complete is for a full tranfer completion.
         * This function is kept here as placeholder for actual
         * half transfer function
         */
        /*
          DMA_Trans_Complete(dma_inst_ptr, dma_inst_ptr->dma_dev_id, NU_SUCCESS);
         */

    }



    /* Transfer Complete Interrupt management ***********************************/
    /* This is the first of a two part check for transfer complete.  This block
     * contains common code and code to handle last transfer completion
     */
    DMA_GET_FLAGS(dma_inst_ptr->dma_dev_id, dma_intr_status);              
    if ((dma_intr_status & DMA_FLAG_TCIF0_4) && (tgt_ptr->DMA_SxCR_copy & DMA_IT_TC))
    {
        /* Multi_Buffering mode enabled */
        if(dma_stream->CR & DMA_SxCR_DBM)
        {
          /* Last memory buffer transferred is Memory 1 
           * Current memory buffer  is Memory 0
           * Indicate that memory 1 is ready to be processed
           */
          if((dma_stream->CR & DMA_SxCR_CT) == 0)
          {
            /* Change DMA peripheral state */
            tgt_ptr->dmaState = HAL_DMA_STATE_READY_MEM1;
          }
          /* Current memory buffer transfer is Memory 1 */
          else
          {
            /* Change DMA peripheral state */
            tgt_ptr->dmaState = HAL_DMA_STATE_READY_MEM0;
          }
        }
        else
        {
          /* Disable the transfer interrupt if the DMA mode is not CIRCULAR */
          /* Circular DMA mode means that data is contiuously read into the 
           * buffer and the pointer circles back to beginning after the specified
           * number of words have been transmitted.
           */

          /* Keep the transfer interrupt enabled if CIRC buffer is enabled*/
          if((dma_stream->CR & DMA_SxCR_CIRC))
            dma_stream->CR |= DMA_IT_TC;
    
          /* Change DMA peripheral state */
          tgt_ptr->dmaState = HAL_DMA_STATE_READY_MEM0;
        }

        /* If there is another request queued in this channel, 
         * initiate the request.
         */

        /* Current request count decrements as this request is now completed
         */
        if (dma_inst_ptr->chan_req_first->cur_req_length != DMA_LENGTH_CONTINUOUS)
        	dma_inst_ptr->chan_req_first->cur_req_length --;
        
        /* Make a copy of the current channel request length as the
         * DMA_Trans_Complete function will remove the channel if there are no
         * more elements to transfer.
         *
         * The second part of the transfer complete process will use this local
         * copy to determine if there are additional requests to process.
         * I use the copy just in case the original record has been discarded
         * by the DMA_Trans_Complete function.
         */
        cur_req_length_copy = dma_inst_ptr->chan_req_first->cur_req_length;

        DMA_Trans_Complete(dma_inst_ptr, dma_inst_ptr->dma_dev_id, status);
    }
    
    DMA_CLEAR_FLAGS(dma_inst_ptr->dma_dev_id, dma_intr_status);

    /* Transfer Complete Interrupt management ***********************************/
    /* This is the second of a two part check for transfer complete.  This block
     * contains code to handle additional transfers
     *
     * If there is still additional requests to process, the very last
     * set of code in the HISR will be to re-enable the interrupts
     * and initiate another transfer.
     *
     * The _Trans function should be split up into two functions to
     * remove unnecessary hardware initialization. It doesn't hurt but
     * it will take up cycles if we need to optimize code.
     *
     * NOTE: The DMA_LENGTH_CONTINUOUS operation can be optimized by
     * enabling the circular buffer mode in the STM DMA engine.  This will have
     * the desired behavior of continuously sending data.  Using the "software"
     * mode of continuous operation, ie re-enabling and restarting transfers,
     * takes up cpu time.
     *
     */
   
    if ( (dma_intr_status & DMA_FLAG_TCIF0_4) &&
        (cur_req_length_copy > 0) )
    {

        /* Advance the request pointer to the next one if its not in
         * CONTINUOUS setting
         */
        if (cur_req_length_copy != DMA_LENGTH_CONTINUOUS)
        	dma_inst_ptr->chan_req_first->cur_req_ptr ++;

    	/* Initiate the transfer */
    	status = DMA_Tgt_Data_Trans(dma_inst_ptr, dma_inst_ptr->chan_req_first);

    	if (status != NU_SUCCESS)
    		tgt_ptr->dmaState = HAL_DMA_STATE_ERROR;

    }

    DMA_TGT_HISR_EXIT;
}

/*------------------------------------- EOF ---------------------------------*/


