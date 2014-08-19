/****************************************************************************
*
*                  Copyright 2002 Mentor Graphics Corporation
*                             All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
****************************************************************************/

/**************************************************************************
*
* FILE NAME
*
*       can_extr.h
*
* COMPONENT
*
*       Nucleus CAN
*
* DESCRIPTION
*
*       This file contains the function prototypes for Nucleus CAN
*       services.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       can.h                               Main definition and API file
*                                           for Nucleus CAN
*
*       cqm_extr.h                          Nucleus CAN queue management
*                                           functions.
*
*************************************************************************/
/* Check to see if the file has already been included. */
#ifndef     CAN_EXTR_H
#define     CAN_EXTR_H

#include    "can.h"
#include    "cqm_extr.h"


/* Function prototypes for the function to integrate Nucleus CAN
   with registry initialization. */

VOID        nu_os_conn_can_init     (const CHAR * key, INT);

/* Function prototypes for Nucleus CAN initialization. */
STATUS  CANC_Initialize                 (NU_MEMORY_POOL *mem_pool);
STATUS  CANC_Start                      (CAN_HANDLE *can_dev, CAN_INIT *can_init);

/* Core functions for implementing Nucleus CAN API. */
STATUS  CANC_Receive_Data               (CAN_PACKET *can_msg);
STATUS  CANC_Node_State                 (UINT8  can_port_id,
                                         CAN_HANDLE  can_dev_id);
STATUS  CANC_Sleep_Device               (UINT8  can_port_id,
                                         CAN_HANDLE  can_dev_id);
STATUS  CANC_Wakeup_Device              (UINT8  can_port_id,
                                         CAN_HANDLE  can_dev_id);
STATUS  CANC_Set_Baud_Rate              (UINT8  can_port_id,
                                         CAN_HANDLE  can_dev_id,
                                         UINT16 baud_rate);
UINT16  CANC_Get_Baud_Rate              (UINT8  can_port_id,
                                         CAN_HANDLE  can_dev_id);
STATUS  CANC_Close_Driver               (UINT8  can_port_id,
                                         CAN_HANDLE  can_dev_id);

/* Function prototypes for Nucleus CAN message reception filter
   configuration. */
STATUS  CANFC_Set_Mask                  (UINT8  can_port_id,
                                         CAN_HANDLE  can_dev_id,
                                         UINT8  buffer_no,
                                         UINT32 mask_value);

/* Function prototypes for Nucleus CAN support services. */
STATUS  CANS_Get_CAN_CB                 (UINT8  can_port_id,
                                         CAN_HANDLE  can_dev_id,
                                         CAN_CB **can_cb);
STATUS  CANS_Check_Init_Params          (CAN_INIT *can_init,
                                         CAN_CB **can_cb);

/* Function prototypes for allocating OS resources to Nucleus CAN. */
STATUS  CAN_OSAL_Allocate_Resources     (CAN_CB *can_cb,
                                         NU_MEMORY_POOL *mem);
STATUS  CAN_OSAL_Deallocate_Resources   (CAN_CB *can_cb);
STATUS  CAN_OSAL_Create_Handler         (NU_MEMORY_POOL *mem);

/* Function prototypes for Nucleus CAN handler. */
VOID    CAN_Handler                     (CAN_CB *can_cb);
STATUS  CANS_Get_Device_Index           (DV_DEV_ID dev_id, INT* index);

/* Perform optimization for speed based upon the optimization settings. */
#if         (!NU_CAN_OPTIMIZE_FOR_SPEED)

VOID        CANS_Copy_Msg(const CAN_PACKET *src_msg, CAN_PACKET *dest_msg);

#else

/* Macro to perform copy of a CAN message from one memory region to another.
   The CAN packet structure is fully aligned so only a simple while is
   used to copy the message contents. */

#define     CANS_Copy_Msg(src_msg, dest_msg)                            \
        if ((dest_msg != NU_NULL) && (src_msg  != NU_NULL))           \
        {                                                               \
            UNSIGNED_INT    count;                                      \
            UNSIGNED_INT    *dest;                                      \
            UNSIGNED_INT    *source;                                    \
            count  = sizeof(CAN_PACKET)/sizeof(UNSIGNED_INT);           \
            source = (UNSIGNED_INT *)src_msg;                           \
            dest   = (UNSIGNED_INT *)dest_msg;                          \
            while (count--)                                             \
            {                                                           \
                *(dest++) = *(source++);                                \
            }                                                           \
        }

#endif      /* !NU_CAN_OPTIMIZE_FOR_SPEED */

/* Check if multiple port support is enabled. */
#if         NU_CAN_MULTIPLE_PORTS_SUPPORT

extern      UINT8       CAN_Devs_In_Port[CAN_MAX_SUPPORTED_PORTS];

#if         NU_CAN_OPTIMIZE_FOR_SPEED

/* This macro sums up the number of total drivers in all the ports before
   the specified port. */

#define     CAN_Sum_Port_Drivers(port_id)                               \
            (port_id == CAN_PORT1  ? 0 :                                \
             port_id == CAN_PORT2  ? CAN_Devs_In_Port[CAN_PORT1] :   \
             port_id == CAN_PORT3  ? CAN_Devs_In_Port[CAN_PORT2] +   \
                                     CAN_Devs_In_Port[CAN_PORT1] :   \
             port_id == CAN_PORT4  ? CAN_Devs_In_Port[CAN_PORT3] +   \
                                     CAN_Devs_In_Port[CAN_PORT2] +   \
                                     CAN_Devs_In_Port[CAN_PORT1] :   \
            0)

/* Macros to calculate the internal driver index for the specified
   device of the specified port. Both the macros perform the same
   functionality but operate upon different parameters. */
#define     CAN_DRIVER_NUMBER(can_msg)                                  \
                              can_msg->can_dev_id +                     \
                             (CAN_Sum_Port_Drivers(can_msg->can_port_id))

#define     CAN_Calc_Driver_ID(port_id, dev_id)                         \
            dev_id + (CAN_Sum_Port_Drivers(port_id))

#else      /* Working without speed optimization. */

/* Prototype for function that will calculate driver ID for internal
   usage. */
INT         CANS_Calculate_Port_Driver_ID(INT port_id, INT dev_id);

/* Mapping of function call to macro to provide uniform interfaces for
   calculating internal driver ID. Both the macros perform the same
   functionality but operate on different parameters. */

#define     CAN_DRIVER_NUMBER(can_msg)                                  \
            CANS_Calculate_Port_Driver_ID(                               \
            can_msg->can_port_id, can_msg->can_dev_id)

#define     CAN_Calc_Driver_ID(port_id, dev_id)                         \
            CANS_Calculate_Port_Driver_ID(port_id, dev_id)

#endif      /* NU_CAN_OPTIMIZE_FOR_SPEED */

#endif      /* NU_CAN_MULTIPLE_PORTS_SUPPORT */

#endif      /* !CAN_EXTR_H */

