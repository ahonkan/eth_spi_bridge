/***********************************************************************
*
*            Copyright 2012 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       This file contains private data structure definitions,  
*       constants and functions of SELF REFRESH subcomponent.
*
***********************************************************************/
#ifndef PMS_SELFREFRESH_H
#define PMS_SELFREFRESH_H

#ifdef __cplusplus
extern "C" {
#endif

STATUS PMS_SelfRefresh_Enter(VOID);
STATUS PMS_SelfRefresh_Exit(VOID);
VOID   PMS_SelfRefresh_Initialize(NU_MEMORY_POOL* mem_pool);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef PMS_SELFREFRESH_H */


