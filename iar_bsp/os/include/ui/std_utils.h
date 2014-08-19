/***************************************************************************
*
*           Copyright 1998 Mentor Graphics Corporation
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
*  std_utils.h                                                  
*
* DESCRIPTION
*
*  This file contains prototypes and externs for std_utils.c
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
#ifndef _STD_UTILS_H_
#define _STD_UTILS_H_


#ifdef __cplusplus      /* If C++, disable "mangling" */
extern "C" {
#endif

INT32   STD_a_toi(const UNICHAR *nptr);
UNICHAR *STD_i_toa(INT32 val, UNICHAR *str, INT32 radix);
VOID    STD_number_to_a_worker(UINT32, UNICHAR *, INT32, BOOLEAN);
UNICHAR *STD_l_toa(INT32 value, UNICHAR* destination, INT32 radix);
INT32   STD_is_digit(INT32 c);
INT32   STD_is_alpha(INT32 c);
INT32   STD_is_alnum(INT32 c);
INT32   STD_is_space(INT32 c);
INT32   STD_abs_val(INT32 val);

#ifdef __cplusplus
}
#endif

#endif /* _STD_UTILS_H_ */
