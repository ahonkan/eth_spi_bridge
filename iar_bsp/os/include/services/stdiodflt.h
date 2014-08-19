/*************************************************************************
*
*                  Copyright 2003 Mentor Graphics Corporation
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
*       stdiodflt.h
*
* COMPONENT
*
*       RTL - Runtime Library.
*
* DESCRIPTION
*
*       This file contains the internal routines used by STDIO.
*
* DATA STRUCTURES
*
*       size_t              An unsigned integer type.
*
* DEPENDENCIES
*
*       "stddef.h"                          STDDEF related definitions.
*
*************************************************************************/
#ifndef __STDIODFLT_H_
#define __STDIODFLT_H_

#include "services/stddef.h"

#define PRINTFBUFSIZ    257                 /*  Max length + 1 of printf
                                                output */
#define PF_BUF_OVERFLOW -256                /*  Return value when printf
                                                output buffer is
                                                exceeded */

int  eprintf(const char *fmt, ...);
int  xprintf1(char *str, const char *fmt, va_list args, int (*hook)());
int  xprintf(char *str, char *fmt, va_list args, int (*hook)());
void xftof(char *str, char *buf, int expt, int sig, int prec, int flags);
void xftoe(char *str, char *buf, int expt, int sig, int prec, int flags);
int  xpfefg(int ch, char *str, double *nump, int prec, int flags);
int  xscanf(int (*getcfn)(void *),int (*ungetcfn)(int, void *),
            void *param, const char *fmt, va_list parglist);
void brev(char *buf,size_t len);
void xsfef(char *pvar, char *str, int isdouble);
int  xpfr(int val,char *buf);
int xpfneg(va_list  *args, double *dnum);

#endif /*   __STDIODFLT_H_  */




