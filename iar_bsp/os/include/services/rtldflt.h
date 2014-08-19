/************************************************************************
*
*                  Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
*************************************************************************

*************************************************************************
* FILE NAME               
*
*		rtldflt.h              
*
* COMPONENT
*
*		RTL - RunTime Library.
*
* DESCRIPTION
*
*		This file contains the various internal routines used by POSIX
*		RunTime Library.
*
* DATA STRUCTURES
*
*		None
*
* DEPENDENCIES
*
*		None
*
************************************************************************/
#ifndef __RTLDFLT_H_
#define __RTLDFLT_H_

/*  Definitions related to the time related functions   */
/* The default synchronization for time() is:
Thursday, 00:00.00 January 1, 1970 GMT.
__tstlpyr(a) returns nonzero if 'a' is a leap year.
_TIMEZONE holds the index to the timezone name for strftime().
_SPNY holds the seconds per normal year
_SPLY holds the seconds per leap year
*/

#define _BASEYEAR 1970
#define _BASEDOFW 4
#define __tstlpyr(a) (a%4 == 0 && !(a%100 == 0 && a%400 != 0))
#define _TIMEZONE 3
#define _SPNY (365L*24L*60L*60L)
#define _SPLY (366L*24L*60L*60L)


/* These functions are required by printf and scanf */
#define UPPER 2
#define LONG 4
#define PREC 8

int i_isxdigit(int c);
int i_isdigit(int c);
int i_isodigit(int c);
int xecvt(double val, int ndig, char *buf);
int xsfff(int *inp, int (*getcfn)(char *), char *param, char *pc, int width);
void xftof(char *str, char *buf, int expt, int sig, int prec, int flags);
int ss_getc(char **ptr);
void xftoe(char *str, char *buf, int expt, int sig, int prec, int flags);
int xpfefg(int ch, char *str, double *nump, int prec, int flags);
void xsfef(char *pvar, char *str, int isdouble);
double square(double d);

#endif  /*  __RTLDFLT_H_    */




