/*************************************************************************
*
*                  Copyright 2012 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
* FILE NAME
*
*       lwspi_defs.h
*
* COMPONENT
*
*       Nucleus lightweight SPI
*
* DESCRIPTION
*
*       This file contains data structures, functions and definitions 
*       for internal use of lightweight SPI component.
*
* DATA STRUCTURES
*
*       None.
*
* DEPENDENCIES
*
*       None.
*
*************************************************************************/
#ifndef _LWSPI_DEFS_H
#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++. */
#endif

#define _LWSPI_DEFS_H

/* Miscellaneous defines for encoding/decoding handle and finding bus and 
 * slave slots.
 */
#define NU_SPI_HANDLE_MAGIC_NUM     0xA5000000
#define NU_SPI_VALIDATE_HANDLE(n)   ((n&0xFF000000) == NU_SPI_HANDLE_MAGIC_NUM)
#define NU_SPI_ENCODE_HANDLE(b,d)   (NU_SPI_HANDLE_MAGIC_NUM | (b << 16) | (d << 8))
#define NU_SPI_DECODE_HANDLE(h,b,d) {   \
                                        (*(UINT8*)(b)) = ((h & 0x00FF0000) >> 16);   \
                                        (*(UINT8*)(d)) = ((h & 0x0000FF00) >> 8);    \
                                    }
                                    
/* Internal functions. */
static STATUS LWSPI_Bus_Register(DV_DEV_ID, VOID*);
static STATUS LWSPI_Bus_Unregister(DV_DEV_ID, VOID*);
static STATUS LWSPI_Find_Vacant_Bus_Slot(NU_SPI_BUS**);
static STATUS LWSPI_Find_Vacant_Device_Slot(NU_SPI_BUS*, NU_SPI_DEVICE**);
static STATUS LWSPI_Find_Bus_Slot_By_Name(CHAR*, NU_SPI_BUS**);
static STATUS LWSPI_Find_Bus_Slot_By_ID(DV_DEV_ID, NU_SPI_BUS**);
static STATUS LWSPI_Get_Params_From_Handle(NU_SPI_HANDLE, NU_SPI_BUS**, NU_SPI_DEVICE**);

#ifdef      __cplusplus
}
#endif
#endif  /* #ifndef _LWSPI_DEFS_H */
