/***************************************************************************
*
*             Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
****************************************************************************

****************************************************************************
*
* FILE NAME
*
*  screen_inc.h
*
* DESCRIPTION
*
*  This file contains the screen includes.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  None.
*
* DEPENDENCIES
*
*  None.
*
***************************************************************************/
#ifndef _SCREEN_INC_H_
#define _SCREEN_INC_H_

#ifdef INCLUDE_1_BIT
#include "drivers/m1b_fil.h"
#include "drivers/m1b_drv.h"
#include "drivers/m1b_lin.h"
#endif

#ifdef INCLUDE_2_4_BIT
#include "drivers/m2b4_fil.h"
#include "drivers/m2b4_drv.h"
#include "drivers/m2b4_lin.h"
#endif

#ifdef INCLUDE_8_BIT
#include "drivers/m8b_fil.h"
#include "drivers/m8b_drv.h"
#include "drivers/m8b_lin.h"
#endif

#ifdef INCLUDE_16_BIT
#include "drivers/m16b_fil.h"
#include "drivers/m16b_drv.h"
#include "drivers/m16b_lin.h"
#endif

#ifdef INCLUDE_24_BIT
#include "drivers/m24b_lin.h"
#include "drivers/m24b_drv.h"
#include "drivers/m24b_fil.h"
#endif

#ifdef INCLUDE_32_BIT
#include "drivers/m32b_lin.h"
#include "drivers/m32b_drv.h"
#include "drivers/m32b_fil.h"
#endif

#endif



