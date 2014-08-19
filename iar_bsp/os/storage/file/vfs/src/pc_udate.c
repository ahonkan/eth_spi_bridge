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
*       pc_udate.c
*                                                                       
* COMPONENT                                                             
*                                                                       
*       Date and Time
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       When the system needs to date stamp a file it will call this    
*       routine to get the current time and date. YOU must modify the   
*       shipped routine to support your hardware's time and date        
*       routines. If you don't modify this routine the file date on all 
*       files will be the same.                                         
*                                                                       
*       The source for this routine is in file pc_udate.c and is self   
*       explanatory.                                                    
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       None.                                                           
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       pc_getsysdate                       Get date and time from the  
*                                            host system.               
*                                                                       
*************************************************************************/
#include        "storage/pcdisk.h"
#include        "nucleus.h"
#include        "storage/date_extr.h"


/************************************************************************
* FUNCTION                                                              
*                                                                       
*       pc_getsysdate                                                   
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Get date and time from the host system.                         
*       
*                                                                       
* INPUTS                                                                
*                                                                       
*       pd                                  Date stamping buffer.       
*                                                                       
* OUTPUTS                                                               
*                                                                       
*      Pointer of the buffer of date structure.                         
*                                                                       
*************************************************************************/
DATESTR *pc_getsysdate(DATESTR *pd)
{
UINT16      year;                           /* relative to 1980 */ 
UINT16      month;                          /* 1 - 12 */ 
UINT16      day;                            /* 1 - 31 */ 
UINT16      hour;
UINT16      minute;
UINT16      sec;                            /* Note: seconds are 2 second/per. ie 3 == 6 seconds */
UINT8       cenmsec;

    /* Generic */
    /* Hardwired for now */
    /* 7:37:28 PM */
    hour = 19;
    minute = 37;
    sec = 14;
    /* 3-28-88 */
    year = 8;                               /* relative to 1980 */ 
    month = 3;                              /* 1 - 12 */ 
    day = 28;                               /* 1 - 31 */
    /* Centesimal Milliseconds (1sec / 100) */
    cenmsec = (UINT8)0;    /* 0 - 199 */

    pd->cmsec = cenmsec;
    pd->time = (UINT16) ( (hour << 11) | (minute << 5) | sec );
    pd->date = (UINT16) ( (year << 9) | (month << 5) | day );

    return(pd);
}


