/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2013
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
*       linkload.h
*
*   COMPONENT
*
*       Nucleus Processes - Linker / Loader
*
*   DESCRIPTION
*
*       Linker / Loader component internal API.
*
*   DATA STRUCTURES
*
*       None
*
*************************************************************************/

#ifndef LINKLOAD_H
#define LINKLOAD_H

/* Symbols API */
STATUS PROC_Get_Exported_Symbol_Address(PROC_CB *            process,
                                        CHAR *               sym_name,
                                        VOID **              sym_addr);

STATUS PROC_Validate_Symbols(NU_SYMBOL_ENTRY * sym_table);

#endif /* LINKLOAD_H */
