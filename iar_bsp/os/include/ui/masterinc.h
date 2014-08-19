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
*  masterinc.h                                                  
*
* DESCRIPTION
*
*  This file is the master include file.
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
#ifndef _MASTERINC_H_
#define _MASTERINC_H_

/* OFFSET size */
#define TYPEOFF SIGNED  

/*   bytes */
#define SIZEOFF 4       

/* NATIVE cpu register size */
#define TYPENAT SIGNED  

/*   bytes */
#define SIZENAT 4       

/*   bits */
#define BITSNAT 32      

/*   divide/multiply by SIZENAT shift count */
#define SSHFNAT 2       

/*   divide/multiply by BITSNAT shift count */
#define BSHFNAT 4       

#define DEFN TYPENAT

/* M_PAUSE - lock grafMap */
#define M_PAUSE(gmapptr) (gmapptr->mapLock--)   

#ifdef  GFX_VERBOSE_ERROR

VOID nuGrafErrInternal(INT16 grafErrValue, INT16 lineNum, CHAR *fileName);
#define nuGrafErr(a, b, c)  nuGrafErrInternal(a, b, c)

#else

VOID nuGrafErrInternal(INT16 grafErrValue);
#define nuGrafErr(a, b, c)  nuGrafErrInternal(a)

#endif  /* GFX_VERBOSE_ERROR */

VOID nuClearErrList(VOID);


#endif /* _MASTERINC_H_ */

