/***************************************************************************
*
*               Copyright 2004 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
****************************************************************************

****************************************************************************
*
* FILE NAME                                             
*
*  input_config.h                                                      
*
* DESCRIPTION
*
*  This file contains defines for input configuration.
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
#ifndef _INPUT_CONFIG_H_
#define _INPUT_CONFIG_H_

/* Only one Mouse type can be used at a time but if you want */
/* to use more than one then you can define two and initialize */
/* them when you would be using them */

#define INCLUDE_INPUT_DEVICE

#ifndef INCLUDE_INPUT_DEVICE
#define CYCLE
#endif

#endif /* _INPUT_CONFIG_H_ */

