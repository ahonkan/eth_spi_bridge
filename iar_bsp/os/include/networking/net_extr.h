/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
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
*       net_extr.h
*
*   COMPONENT
*
*       Net
*
*   DESCRIPTION
*
*       Holds the defines for the compare functions INT32_CMP and
*       INT16_CMP.
*
*   DATA STRUCTURES
*
*       None.
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef NET_EXTR_H
#define NET_EXTR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/*************************************************************************
*  INT32_CMP
*
*  Compare two sequence numbers.  This macro handles will handle sequence
*  space wrap around.  Overflow/Underflow makes the results below
*  correct.
*  RESULTS:            result       implies
*                        -          a < b
*                        0          a = b
*                        +          a > b
*************************************************************************/
#define INT32_CMP(a, b)      ((INT32)((a)-(b)))
#define INT16_CMP(a, b)      ((INT16)((a)-(b)))

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif
