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
*       pc_error.c
*                                                                       
* COMPONENT                                                             
*                                                                       
*       Error
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Report internal error function.                                 
*                                                                       
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       pc_error_strings                    Error strings list.         
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       va_error_print                      Error code print.           
*       pc_report_error                     Error report.               
*                                                                       
*************************************************************************/
#include "nucleus.h"
#ifdef DEBUG
#include        <stdarg.h>
#endif

#if (CFG_NU_OS_STOR_FILE_VFS_ERROR_SUPPORT == 1)
volatile CHAR  *pcerr_unused_ptr;

/* Error strings list. */
static UINT8 *pc_error_strings[] =
{
    /* PCERR_FAT_FLUSH      0 */    (UINT8 *)"Cant flush FAT\n",
    /* PCERR_FAT_NULLP      1 */    (UINT8 *)"flushfat called with null pointer",
    /* PCERR_NOFAT          2 */    (UINT8 *)"No FAT type in this partition., Can't Format",
    /* PCERR_FMTCSIZE       3 */    (UINT8 *)"Too many cluster for this partition., Can't Format",
    /* PCERR_FMTFSIZE       4 */    (UINT8 *)"File allocation Table Too Small, Can't Format",
    /* PCERR_FMTRSIZE       5 */    (UINT8 *)"Numroot must be an even multiple of INOPBLOCK",
    /* PCERR_FMTWBZERO      6 */    (UINT8 *)"Failed writing block 0",
    /* PCERR_FMTWFAT        7 */    (UINT8 *)"Failed writing FAT block",
    /* PCERR_FMTWROOT       8 */    (UINT8 *)"Failed writing root block",
    /* PCERR_FMT2BIG        9 */    (UINT8 *)"Total sectors may not exceed 64k.",
    /* PCERR_FSTOPEN        10 */   (UINT8 *)"pc_free_all freeing a file",
    /* PCERR_INITMEDI       11 */   (UINT8 *)"Not a DOS disk:pc_dskinit",
    /* PCERR_INITDRNO       12 */   (UINT8 *)"Invalid driveno to pc_dskinit",
    /* PCERR_INITCORE       13 */   (UINT8 *)"Out of core:pc_dskinit",
    /* PCERR_INITDEV        14 */   (UINT8 *)"Can't initialize device:pc_dskinit",
    /* PCERR_INITREAD       15 */   (UINT8 *)"Can't read block 0:pc_dskinit",
    /* PCERR_BLOCKCLAIM     16 */   (UINT8 *)"There is no block buffer",
    /* PCERR_INITALLOC      17 */   (UINT8 *)"Fatal error: Not enough core at startup",
    /* PCERR_BLOCKLOCK      18 */   (UINT8 *)"Warning: freeing a locked buffer",
    /* PCERR_FREEINODE      19 */   (UINT8 *)"Bad free call to freei",
    /* PCERR_FREEDROBJ      20 */   (UINT8 *)"Bad free call to freeobj",
    /* PCERR_FATCORE        21 */   (UINT8 *)"Not Enough Core To Load FAT",
    /* PCERR_FATREAD        22 */   (UINT8 *)"IO Error While Failed Reading FAT",
    /* PCERR_BLOCKALLOC     23 */   (UINT8 *)"Block Alloc Failure Memory Not Initialized",
    /* PCERR_DROBJALLOC     24 */   (UINT8 *)"Memory Failure: Out of DROBJ Structures",
    /* PCERR_FINODEALLOC    25 */   (UINT8 *)"Memory Failure: Out of FINODE Structures",
    /* PCERR_USERS          26 */   (UINT8 *)"Out of User Structures",
    /* PCERR_BAD_USER       27 */   (UINT8 *)"Unregistered User",
    /* PCERR_NO_DISK        28 */   (UINT8 *)"No disk",
    /* PCERR_DISK_CHANGED   39 */   (UINT8 *)"Disk changed",
    /* PCERR_DRVALLOC       30 */   (UINT8 *)"Memory Failure: Out of DDRIVE Structures",
    /* PCERR_PATHL          31 */   (UINT8 *)"Path too long."
    };                       


/************************************************************************
* FUNCTION                                                              
*                                                                       
*       va_error_print                                                  
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Error print function.                                           
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*       INT8 *format, ...                                               
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None.                                                           
*                                                                       
*************************************************************************/
VOID va_error_print(CHAR *format, ...)
{
    /* Eliminate compiler warnings */
	pcerr_unused_ptr = format;

#ifdef DEBUG
va_list     argptr;
char        msg[512];


    va_start(argptr, format);
    vsprintf(msg, (char *)format, argptr);
    va_end(argptr);

    PRINT_STRING(msg);
#endif
}


/************************************************************************
* FUNCTION                                                              
*                                                                       
*       pc_report_error                                                 
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       When the system detects an error needs it calls this routine    
*       with PCERR_????? as an argument. In the reference port we call  
*       printf to report the error. You may do anything you like with   
*       these errors.                                                   
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*       error_number                        Error number.               
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None.                                                           
*                                                                       
*************************************************************************/
VOID pc_report_error(INT error_number)
{
    va_error_print("Error %d: %s\r\n", error_number, 
                    pc_error_strings[error_number]);
}
#endif  /* CFG_NU_OS_STOR_FILE_VFS_ERROR_SUPPORT == 1 */
