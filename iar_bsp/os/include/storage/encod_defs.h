/*************************************************************************/
/*                                                                       */
/*               Copyright 1993 Mentor Graphics Corporation              */
/*                         All Rights Reserved.                          */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   */
/* SUBJECT TO LICENSE TERMS.                                             */
/*                                                                       */
/*************************************************************************/
/*************************************************************************
* FILE NAME
*
*       encod_defs.h
*
* COMPONENT
*
*       Encode
*
* DESCRIPTION
*
*       Contains the structure definition for encode services.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       None.
*
*************************************************************************/
#ifndef ENCOD_DEFS_H
#define ENCOD_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

typedef struct codepage_op_struct
{
    UINT8         (*cp_to_utf8)(UINT8 *, UINT8 *);
    UINT8         (*utf8_to_cp)(UINT8 *, UINT8 *);

}CP_OP;

typedef struct codepage
{
    /* Set this to the codepage that you want. */
    UINT16 cp;
    CP_OP cp_op;

}CPTE_S;

#ifdef          __cplusplus
}

#endif /* _cplusplus */


#endif

