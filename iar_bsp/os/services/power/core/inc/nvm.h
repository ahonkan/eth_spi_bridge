/*************************************************************************
*
*               Copyright 2012 Mentor Graphics Corporation
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
*       nvm.h
*
*   COMPONENT
*
*       Non-Volatile Memory
*
*   DESCRIPTION
*
*       This file contains function declarations
*       for the Non-Volatile Memory (NVM) Driver module.
*
*************************************************************************/
#ifndef NVM_H
#define NVM_H

#ifdef __cplusplus
extern  "C" {
#endif /* _cplusplus */

/* NVM Driver Target-specific function prototypes */
STATUS NVM_Tgt_Open(VOID);
STATUS NVM_Tgt_Close(VOID);
STATUS NVM_Tgt_Read(VOID *p_item, UINT *p_item_size, UINT index);
STATUS NVM_Tgt_Write(VOID *p_item, UINT item_size, UINT *p_index, BOOLEAN verify);
STATUS NVM_Tgt_Reset(VOID);
STATUS NVM_Tgt_Info(UINT *p_nvm_size, UINT *p_item_size_max);

#if (CFG_NU_OS_SVCS_PWR_CORE_NVM_AVAILABLE == NU_TRUE)

#define PM_NVM_OPEN()                           NVM_Tgt_Open()
#define PM_NVM_CLOSE()                          NVM_Tgt_Close()
#define PM_NVM_READ(item, size, index)          NVM_Tgt_Read(item, size, index)
#define PM_NVM_WRITE(item, size, index, verify) NVM_Tgt_Write(item, size, index, verify)
#define PM_NVM_RESET()                          NVM_Tgt_Reset()
#define PM_NVM_INFO(size, max)                  NVM_Tgt_Info(size, max)

#else

#define PM_NVM_OPEN()                           NU_INVALID_OPERATION
#define PM_NVM_CLOSE()                          NU_INVALID_OPERATION
#define PM_NVM_READ(item, size, index)          NU_INVALID_OPERATION
#define PM_NVM_WRITE(item, size, index, verify) NU_INVALID_OPERATION
#define PM_NVM_RESET()                          NU_INVALID_OPERATION
#define PM_NVM_INFO(size, max)                  NU_INVALID_OPERATION

#endif

#ifdef __cplusplus
}
#endif /* _cplusplus */

#endif /* NVM_H */
