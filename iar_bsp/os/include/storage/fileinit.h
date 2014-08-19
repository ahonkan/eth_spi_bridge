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
*       fileinit.h
*
* COMPONENT
*
*       Initialization
*
* DESCRIPTION
*          This header file defines the drivers to be initialized by
*          the FILE library.
*
*
*************************************************************************/
#ifndef FILEINIT_H
#define FILEINIT_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/******************************  Externals  ******************************/
/* =========== File FileInit.C ================ */
STATUS  file_init(VOID *config_s);

#define FINIT_REG_OPT_AUTO_MNT_PT   "/auto_mnt_pt_start_loc"
#define FINIT_REG_OPT_GROUP_MNT     "/mw_mnt_info"
#define FINIT_REG_OPT_MNT_PT        "/pt"
#define FINIT_REG_OPT_MNT_FS        "/fs"
#define FINIT_REG_OPT_MNT_AUTO_FMT  "/auto_fmt"
#define FINIT_REG_OPT_MAX_OPEN_FILES "/max_open_files"

STATUS finit_volume_enumeration(CHAR* dev_name);
STATUS finit_remove_logical_devices(CHAR* dev_name);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* FILEINIT_H */


