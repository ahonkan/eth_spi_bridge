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
*       vfs_exp.c
*
*   COMPONENT
*
*       Exported Symbols
*
*   DESCRIPTION
*
*       Export symbols for Nucleus Virtual File System Interface.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       None
*
*************************************************************************/

#include "storage/nu_storage.h"

#if (CFG_NU_OS_STOR_FILE_VFS_EXPORT_SYMBOLS == NU_TRUE)

#include "kernel/proc_extern.h"

/* Define component name for these symbols */
NU_SYMBOL_COMPONENT(NU_OS_STOR_FILE_VFS);

/* Export file system APIs */
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CHECK_DISK == NU_TRUE)
NU_EXPORT_SYMBOL(NU_Check_Disk);
#endif
NU_EXPORT_SYMBOL(NU_Close);
NU_EXPORT_SYMBOL(NU_Create_Partition);
NU_EXPORT_SYMBOL(NU_Delete);
NU_EXPORT_SYMBOL(NU_Delete_Partition);
NU_EXPORT_SYMBOL(NU_Disk_Abort);
NU_EXPORT_SYMBOL(NU_Done);
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE == NU_TRUE)
NU_EXPORT_SYMBOL(NU_FILE_Cache_Create);
NU_EXPORT_SYMBOL(NU_FILE_Cache_Destroy);
NU_EXPORT_SYMBOL(NU_FILE_Cache_Flush);
NU_EXPORT_SYMBOL(NU_FILE_Cache_Get_Config);
NU_EXPORT_SYMBOL(NU_FILE_Cache_Set_Config);
#endif
NU_EXPORT_SYMBOL(NU_Flush);
NU_EXPORT_SYMBOL(NU_Format);
NU_EXPORT_SYMBOL(NU_Free_List);
NU_EXPORT_SYMBOL(NU_Free_Partition_List);
NU_EXPORT_SYMBOL(NU_FreeSpace);
NU_EXPORT_SYMBOL(NU_Get_Attributes);
NU_EXPORT_SYMBOL(NU_Get_First);
NU_EXPORT_SYMBOL(NU_Get_Format_Info);
NU_EXPORT_SYMBOL(NU_Get_Next);
NU_EXPORT_SYMBOL(NU_Get_Partition_Info);
NU_EXPORT_SYMBOL(NU_List_Device);
NU_EXPORT_SYMBOL(NU_List_File_System);
NU_EXPORT_SYMBOL(NU_List_Mount);
NU_EXPORT_SYMBOL(NU_List_Partitions);
NU_EXPORT_SYMBOL(NU_Make_Dir);
NU_EXPORT_SYMBOL(NU_Mount_File_System);
NU_EXPORT_SYMBOL(NU_Open);
NU_EXPORT_SYMBOL(NU_Read);
NU_EXPORT_SYMBOL(NU_Register_File_System);
NU_EXPORT_SYMBOL(NU_Remove_Dir);
NU_EXPORT_SYMBOL(NU_Rename);
NU_EXPORT_SYMBOL(NU_Seek);
NU_EXPORT_SYMBOL(NU_Set_Attributes);
NU_EXPORT_SYMBOL(NU_Storage_Device_Wait);
NU_EXPORT_SYMBOL(NU_Truncate);
NU_EXPORT_SYMBOL(NU_Unmount_File_System);
NU_EXPORT_SYMBOL(NU_Unregister_File_System);
NU_EXPORT_SYMBOL(NU_Utime);
NU_EXPORT_SYMBOL(NU_Write);

#endif /* CFG_NU_OS_STOR_FILE_VFS_EXPORT_SYMBOLS == NU_TRUE */
