/*************************************************************************
*
*            Copyright 2012 Mentor Graphics Corporation
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
*        hibernate_dv_interface.h
*
*   COMPONENT
*
*        Hibernate driver
*
*   DESCRIPTION
*
*        Contains data structures and prototypes of the Hibernate driver
*
*************************************************************************/
#ifndef HIBERNATE_DV_INTERFACE_H
#define HIBERNATE_DV_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Class Label */
#define HIBERNATE_LABEL                 {0xc2,0x99,0x30,0x4f,0xb9,0xb6,0xa8,0x49,0xc7,0xe3,0x3c,0x36,0x0b,0xf9,0xd1,0x25}

/* Hibernate IOCTL offset codes */
#define HIBERNATE_IOCTL_INIT_SHUTDOWN   0     /* Initiate system shutdown */
#define HIBERNATE_IOCTL_INIT_STANDBY    1     /* Initiate system standby */
#define HIBERNATE_IOCTL_EXIT_STANDBY    2     /* Exit system standby */
#define HIBERNATE_IOCTL_TGT_EXIT        3     /* Target specific operations on hibernate exit  */

/* Hibernate function prototypes codes */
STATUS Hibernate_Dv_Register(const CHAR *key, VOID *instance_handle);
STATUS Hibernate_Dv_Unregister(const CHAR *key, INT startstop, DV_DEV_ID dev_id);
STATUS Hibernate_Dv_Open(VOID *instance_handle, DV_DEV_LABEL label_list[], INT label_cnt, VOID**session_handle);
STATUS Hibernate_Dv_Close(VOID *session_handle);
STATUS Hibernate_Dv_Ioctl(VOID *session_handle, INT ioctl_cmd, VOID *data, INT length);

STATUS Hibernate_Tgt_Shutdown(VOID);
UINT32 Hibernate_Tgt_Get_Region_Count(VOID);
VOID Hibernate_Tgt_Get_Region_Info(UINT32 region_index, VOID **start, UINT32 *size);
VOID Hibernate_Tgt_Initialize(VOID);
VOID Hibernate_Tgt_Pre_Save(VOID);
VOID Hibernate_Tgt_Standby_Enter(VOID);
VOID Hibernate_Tgt_Standby_Exit(VOID);
VOID Hibernate_Tgt_Exit(VOID);
STATUS Hibernate_Tgt_MMU_Cache_Enable(UINT32    hib_translation_table);

typedef struct HIBERNATE_REGION_STRUCT
{
    VOID   *hib_start;
    VOID   *hib_end;
} HIBERNATE_REGION;

typedef struct HIBERNATE_SAVE_POOL_INFO_STRUCT
{
    VOID   *hib_pool_start_addr;
    UINT32  hib_pool_size;
    VOID   *hib_pool_footer_addr;
    UINT32  hib_pool_footer_size;
} HIBERNATE_SAVE_POOL_INFO;

#define HIBERNATE_IOCTL_BASE            (DV_IOCTL0 + 1)

#ifdef __cplusplus
}
#endif

#endif /* HIBERNATE_DV_INTERFACE_H */
