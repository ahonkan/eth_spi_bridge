/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILENAME                                               
*
*       snmp_file.h                                              
*
*   DESCRIPTION
*
*       This file contains structure and function declarations for the
*       file related functionality for SNMP.
*
*   DATA STRUCTURES
*
*       SNMP_INDEX_COMPARISON
*       SNMP_ADD_MIB_ENTRY
*       SNMP_NODE
*       SNMP_READ_FILE
*
*   DEPENDENCIES
*
*       None.
*
************************************************************************/
#ifndef SNMP_FILE_H
#define SNMP_FILE_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

#include "storage/pcdisk.h"

/* This macros is used to calculate the offsets for variables. */
#define SNMP_MEMBER_OFFSET(TYPE, MEMBER) \
        ((INT32) &((TYPE *)0)->MEMBER)

/* Function pointer type used to compare indices of two entries. */
typedef INT32 (*SNMP_INDEX_COMPARISON) (VOID *left_side,
                                        VOID *right_side);

/* Function pointer type used to add an entry to a table. */
typedef INT32 (*SNMP_ADD_MIB_ENTRY) (VOID *new_entry);


/* This structure is used to traverse the list of an unknown type.
 * All the structures have a front-link and back-link pointers
 * as the first two members of the structure. This is what makes
 * the traversal possible without knowing the type of the structure.
 */
typedef struct snmp_node
{
    VOID        *snmp_flink;
    VOID        *snmp_blink;
}SNMP_NODE;

/* Structure used to provide information to the file component about
 * how to retrieve data for a particular table during initialization.
 */
typedef struct snmp_read_file
{
    /* Name of file from which we will read data. */
    CHAR                    *snmp_file_name;

    /* Pointer to a location, which we will use
     * to read data to.
     */
    VOID                    *snmp_read_pointer;

    /* Insert function, used to insert the read entries to
     * a list.
     */
    SNMP_ADD_MIB_ENTRY      snmp_insert_func;

    /* Size of the structure. */
    UINT16                  snmp_sizeof_struct;

    /* Make the structure word-aligned. */
    UINT8                   snmp_pad[2];
}SNMP_READ_FILE;

/* Function prototypes. */
STATUS SNMP_Save(VOID *list_header, VOID *updated_entry, VOID *read_entry,
                 INT32 storage_offset, INT32 status_offset,
                 UINT16 sizeof_struct, CHAR *file_name,
                 SNMP_INDEX_COMPARISON comparison_func,
                 UINT8 mib_enable);
STATUS SNMP_Create_File(CHAR *file_name);
STATUS SNMP_Read_File(SNMP_READ_FILE *table_info);
STATUS SNMP_Wait_For_FS(CHAR *mount_point, UNSIGNED timeout);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif /* SNMP_FILE_H */

