/***********************************************************************
*
*            Copyright 2011 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
************************************************************************

************************************************************************
* 
*  DESCRIPTION
*  
*       This file contains DDL constants needed by the
*       application to access the actual Nucleus Services.
*
***********************************************************************/

/* Check to see if this file has been included already.  */

#ifndef         NU_DDL_APP
#ifdef          __cplusplus
/* C declarations in C++     */
extern          "C" {
#endif
#define         NU_DDL_APP


/**********************************************************************/
/*                           DDL Services                             */
/**********************************************************************/
#ifdef CFG_NU_OS_SVCS_DDL_ENABLE

#if (defined(CFG_NU_OS_SVCS_DDL_LOADER_ENABLE) && defined(CFG_NU_OS_SVCS_DDL_RUNTIME_ENABLE)) 
#include        "services/ddl.h"
#include        "services/ddl_elf.h"
#include        "services/ddl_extr.h"
#include        "services/ddl_runtime.h"
#include        "services/ddl_exports.h"
#endif 
#endif /* CFG_NU_OS_SVCS_DDL_ENABLE */


#ifdef          __cplusplus

/* End of C declarations */
}

#endif  /* __cplusplus */

#endif  /* !NU_DDL_APP */

