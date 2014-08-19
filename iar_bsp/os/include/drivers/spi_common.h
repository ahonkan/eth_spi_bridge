/**************************************************************************
*            Copyright 2007 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
***************************************************************************

**************************************************************************
*
*   FILE NAME
*
*       spi_common.h
*
*   COMPONENT
*
*       SPI                              - SPI Library
*
*   DESCRIPTION
*
*       This file contains constant definitions and function declarations
*       for the SPI Library Driver module.
*
*************************************************************************/
#ifndef SPI_COMMON_H
#define SPI_COMMON_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

STATUS  SPI_Get_Target_Info(const CHAR * key, SPI_INSTANCE_HANDLE* spi_inst_ptr);
STATUS  SPI_PR_Intr_Enable(SPI_DRV_SESSION* spi_ses_ptr);
STATUS  SPI_Call_Setup_Func(const CHAR * key, SPI_INSTANCE_HANDLE *spi_inst_ptr);
STATUS  SPI_Set_Reg_Path(const CHAR * key, SPI_INSTANCE_HANDLE *spi_inst_ptr);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* !SPI_COMMON_H */
