/*************************************************************************
*
*               Copyright 2011 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   DESCRIPTION
*
*       This file contains generic interface specifications for Power -
*       aware device drivers
*
*************************************************************************/
#ifndef CPU_INTERFACE_H
#define CPU_INTERFACE_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/**********************************/
/* LOCAL FUNCTION PROTOTYPES      */
/**********************************/
STATUS CPU_Get_Device_Frequency(UINT8 *op_id, CHAR *ref_clock, UINT32 *ref_freq);

#ifdef __cplusplus
}
#endif

#endif /* !CPU_INTERFACE_H */
