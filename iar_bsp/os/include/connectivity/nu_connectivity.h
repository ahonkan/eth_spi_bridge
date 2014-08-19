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
*       nu_connectivity.h
*
* PACKAGE
*
*       Connectivity - Application include file.
*
* COMPONENT
*
*       CAN
*       SPI
*       USB
*       I2C
*
* DESCRIPTION
*
*       This file includes all required header files to allow
*       access to the API for all connectivity components.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*		nucleus_gen_cfg.h
*       can.h
*       spi.h
*       spic_extr.h
*       i2c.h
*       i2c_extr.h
*       nu_usb.h
*************************************************************************/


#ifndef     NU_CONNECTIVITY_H
#define     NU_CONNECTIVITY_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#include "nucleus_gen_cfg.h"

#ifdef CFG_NU_OS_CONN_CAN_ENABLE
/* nu.os.conn.can component include file. */
#include    "connectivity/can_extr.h"
#endif

#ifdef CFG_NU_OS_CONN_SPI_ENABLE
/* nu.os.conn.spi component include files. */
#include    "connectivity/spi.h"
#include    "connectivity/spic_extr.h"
#endif

#ifdef CFG_NU_OS_CONN_I2C_ENABLE
/* nu.os.conn.i2c component include file. */
#include    "connectivity/i2c.h"
#include    "connectivity/i2c_extr.h"
#include    "connectivity/i2c_extern.h"
#endif

#ifdef CFG_NU_OS_CONN_USB_ENABLE
/* nu.os.conn.usb component include file. */
#include "connectivity/nu_usb.h"
#endif

#ifdef CFG_NU_OS_CONN_LWSPI_ENABLE
/* nu.os.conn.lwspi component include fule. */
#include "connectivity/lwspi.h"
#endif

#ifdef          __cplusplus
}
#endif /* _cplusplus */


#endif      /* NU_CONNECTIVITY_H */
