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
*       util.c
*                                                                       
* COMPONENT                                                             
*                                                                       
*       Util
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Standard utility routines
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       None.                                                           
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       NUF_Copybuff                Copy one buffer to another
*       NUF_Memfill                 Fill memory with value
*       NUF_Strncmp                 Compare two byte arrays (strncmp)
*       NUF_Ncpbuf                  Copy up to size, stop at end of "from"
*       NUF_Get_Str_Len             Counts number of characters in the string.
*       NUF_Fswap16                 Convert big to little 16 bit preserving
*                                   little endian
*       NUF_Fswap32                 Convert big to little 32 bit preserving
*                                   little endian
*       NUF_Swap16                  Swap 16 bit byte ordering
*       NUF_Swap32                  Swap 32 bit byte ordering
*       NUF_Through16               16 bit data byte through                                         
*       NUF_Through32               32 bit data byte through
*       NUF_Alloc                   File version of NU_Allocate_Memory                                         
*                                                                
*************************************************************************/
#include "nucleus.h"
#include "storage/pcdisk.h"
#include "storage/util_extr.h"

extern NU_MEMORY_POOL *FILE_Alloc_Pool;
/************************************************************************
* FUNCTION                                                              
*                                                                       
*       NUF_Copybuff                                                         
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Copies one buffer to another. Similar to memcpy and strncpy.
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*       vto                                 Copy to data buffer         
*       vfrom                               Copy from data buffer       
*       size                                Size                        
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None.                                                           
*                                                                       
*************************************************************************/
VOID NUF_Copybuff(VOID *vto, VOID *vfrom, INT size)
{
UINT8       *to   = (UINT8 *) vto;
UINT8       *from = (UINT8 *) vfrom;


    while (size--)
        *to++ = *from++;
}
/************************************************************************
* FUNCTION                                                              
*                                                                       
*       NUF_Memfill                                                      
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Fill "vto" with "size" instances of "c"                                
*       Fill a buffer with a character                                  
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*       vto                                 Copy to data buffer         
*       size                                Size                        
*       c                                   Character                   
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None.                                                           
*                                                                       
*************************************************************************/
VOID NUF_Memfill(VOID *vto, INT size, UINT8 c)
{
UINT8       *to = (UINT8 *) vto;
    
    /* Do not attempt using a pointer to 0x0 */
    if((vto != NU_NULL) && (size >= 0))
    {
        while (size--)
            *to++ = c;
    }

}


/*************************************************************************
*
*   FUNCTION
*
*       NUF_Strncmp
*
*   DESCRIPTION
*
*       Lexigraphically compare at most n characters from the incoming
*       2 arguments and return an integer greater than, equal to, or
*       less than 0 according to whether s1 is greater than, equal to,
*       or less than s2.
*
*   INPUTS
*
*       s1                  String to compare
*       s2                  String to compare
*       n                   The most chars that will be compared
*
*   OUTPUTS
*
*       int                 0 means s1 == s2;
*                           -1 means s1 < s2;
*                           1 means s1 > s2
*
*************************************************************************/
INT NUF_Strncmp(const CHAR *s1, const CHAR *s2, INT n)
{
    if (s1 == NU_NULL)
        return -1;
    else if (s2 == NU_NULL)
        return 1;
    else
    {
        while ((n > 0) && (*s1 == *s2) && (*s1))
            ++s1, ++s2, --n;

        if (n == 0)
        {
            return 0;
        }
    }

    return ((INT)(CHAR)*s1) - ((INT)(CHAR)*s2);
}

/************************************************************************
* FUNCTION                                                              
*                                                                       
*       NUF_Ncpbuf                                                       
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Copy up to size characters from "from" to "to".  Stop copy                 
*       when at end of "from"                                                                
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*       to                                  Copy to data buffer.        
*       from                                Copy from data buffer.      
*       size                                Size of buffer.             
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None.                                                           
*                                                                       
*************************************************************************/
VOID NUF_Ncpbuf(UINT8 *to, UINT8 *from, INT size)
{

    /* Do not attempt using a pointer to 0x0 */
    if((to != NU_NULL) && (from != NU_NULL) && (size >= 0))
    {
        while (size--)
        {
            if (*from)
                *to++ = *from++;
            else
            {
                *to = '\0';
                break;
            }
        }
    }
    else if	(to != NU_NULL)
        *to = '\0';
}

/************************************************************************
* FUNCTION
*
*   NUF_Get_Str_Len
*
* DESCRIPTION
*
*   Used to count the number of characters in a string.    
*          
*
* INPUTS
*  
*   str                         Pointer to the string
*                               whose characters are to be
*                               counted.                               
*
* OUTPUTS
*
*   UINT32                      Number of characters in str.
*
*************************************************************************/
UINT32  NUF_Get_Str_Len(const CHAR *str)
{
    UINT32 char_cnt = 0;    

    if(str != NU_NULL)
    {
        do 
        {
            ++char_cnt;
            ++str;
    	    
        } while(*str != '\0');
    }
    
    return char_cnt;  
}
/************************************************************************
* FUNCTION                                                              
*                                                                       
*       NUF_Fswap16                                                         
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Swap the byte order of a Big Endian 2-byte value.               
*       Preserve the byte order of a Little Endian 2-byte value. 
*       This also does a byte to byte transfer to assure 
*       correct alignment.
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*       *to                                 After Converted data.         
*       *from                               Before Converted data.        
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None.                                                           
*                                                                       
*************************************************************************/
VOID NUF_Fswap16(UINT16 *to, UINT16 *from)
{

    UINT8       *fptr;
    UINT8       *tptr;

    UINT16  num;

    fptr = (UINT8 *)from;

    num = (UINT16)((fptr[1] << 8) + fptr[0]);

    fptr = (UINT8 *)&num;
    tptr = (UINT8 *)to;
    *tptr = *fptr;
    *(tptr+1) = *(fptr+1);

}

/************************************************************************
* FUNCTION                                                              
*                                                                       
*       NUF_Fswap32                                                         
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Swap the byte order of a Big Endian 4-byte value.               
*       Preserve the byte order of a Little Endian 4-byte value. 
*       This also does a byte to byte transfer to assure 
*       correct alignment.
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*       *to                                 After Converted data.         
*       *from                               Before Converted data.        
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None.                                                           
*                                                                       
*************************************************************************/
VOID NUF_Fswap32(UINT32 *to, UINT32 *from)
{

    UINT8       *fptr;
    UINT8       *tptr;

    UINT32  num;

    fptr = (UINT8 *)from;
    num = ((UINT32)fptr[3] << 24) + 
          ((UINT32)fptr[2] << 16) + 
          ((UINT32)fptr[1] << 8) + 
          ((UINT32)fptr[0]);
    fptr = (UINT8 *)&num;
    tptr = (UINT8 *)to;
    *tptr = *fptr;
    *(tptr+1) = *(fptr+1);
    *(tptr+2) = *(fptr+2);
    *(tptr+3) = *(fptr+3);


}

/************************************************************************
* FUNCTION                                                              
*                                                                       
*       NUF_Swap16                                                         
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Swap the byte ordering for a 16-bit value.               
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*       *to                                 After Convert data.         
*       *from                               Before Convert data.        
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None.                                                           
*                                                                       
*************************************************************************/
VOID NUF_Swap16(UINT16 *to, UINT16 *from)
{
UINT8       *fptr;
UINT8       *tptr;


    fptr = (UINT8 *)from;
    tptr = (UINT8 *)to;

    *tptr = *(fptr + 1);
    *(tptr + 1) = *(fptr);
}


/************************************************************************
* FUNCTION                                                              
*                                                                       
*       NUF_Swap32                                                         
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Swap the byte ordering of a 32-bit value.               
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*       *to                                 After Convert data.         
*       *from                               Before Convert data.        
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None.                                                           
*                                                                       
*************************************************************************/
VOID NUF_Swap32(UINT32 *to, UINT32 *from)
{
UINT8       *fptr;
UINT8       *tptr;


    fptr = (UINT8 *)from;
    tptr = (UINT8 *)to;

    *tptr = *(fptr + 3);
    *(tptr + 1) = *(fptr + 2);
    *(tptr + 2) = *(fptr + 1);
    *(tptr + 3) = *(fptr);

}


/************************************************************************
* FUNCTION                                                              
*                                                                       
*       NUF_Through16                                                      
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       16bit data byte through                                         
*                                                                       
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*       *to                                 After Convert data.         
*       *from                               Before Convert data.        
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None.                                                           
*                                                                       
*************************************************************************/
VOID NUF_Through16(UINT16 *to, UINT16 *from)
{
UINT8       *fptr;
UINT8       *tptr;

    fptr = (UINT8 *)from;
    tptr = (UINT8 *)to;

    *tptr = *fptr;
    *(tptr + 1) = *(fptr + 1);
}


/************************************************************************
* FUNCTION                                                              
*                                                                       
*       NUF_Through32                                                      
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       32bit data byte through.                                        
*                                                                       
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*       *to                                 After Convert data.         
*       *from                               Before Convert data.        
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None.                                                           
*                                                                       
*************************************************************************/
VOID NUF_Through32(UINT32 *to, UINT32 *from)
{
UINT8       *fptr;
UINT8       *tptr;

    fptr = (UINT8 *)from;
    tptr = (UINT8 *)to;

    *tptr = *fptr;
    *(tptr + 1) = *(fptr + 1);
    *(tptr + 2) = *(fptr + 2);
    *(tptr + 3) = *(fptr + 3);
}


/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NUF_Atoi                                                    
*                                                                       
*   DESCRIPTION  
*
*       Converts an ASCII number to the integer equivalent.
*                                                                       
*   INPUTS                                                                
*
*       *nptr                   Pointer to the ASCII number to convert
*                                                                       
*   OUTPUTS                                                               
*                                                     
*       The ASCII number in integer form.
*                                                                       
*************************************************************************/
INT NUF_Atoi(const char *nptr)
{
    register const unsigned char *ptr = (const unsigned char *)nptr;
    register       unsigned int   num = 0;
    register                int   c   = *ptr;
    register                int   neg = 0;

    while ( NUF_IS_SPACE(c) )  c = *++ptr;   /* skip over whitespace chars */

    if ( c == '-' )                     /* get an optional sign */
    {
      neg = 1;
      c = *++ptr;
    }
    else if ( c == '+' )  c = *++ptr;

    while ( NUF_IS_DIGIT(c) )
    {
      num = ( 10 * num ) + (unsigned int)( c - '0' );
      c = *++ptr;
    }

    if ( neg )  return ( (INT)NUF_SINEGATE(num) );

    return ( (INT) num );

} 

/************************************************************************
* FUNCTION                                                              
*                                                                       
*       NUF_Alloc                                                       
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Nucleus - NU_Allocate_Memory call function.                     
*       See Nucleus PLUS manual.                                        
*                                                                       
* INPUTS                                                                
*                                                                       
*       nbytes                  Allocate memory size(byte). 
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None.                                                           
*                                                                       
*************************************************************************/
VOID *NUF_Alloc(INT nbytes)
{
VOID        *return_ptr;
INT         alloc_status;


    /* Allocate memory. */
    alloc_status = NU_Allocate_Memory(FILE_Alloc_Pool, &return_ptr,
                      (UNSIGNED) nbytes,
                      NU_NO_SUSPEND);

    if (alloc_status != NU_SUCCESS)
    {
        return_ptr = NU_NULL;
    }
    return((void *) return_ptr);

}

