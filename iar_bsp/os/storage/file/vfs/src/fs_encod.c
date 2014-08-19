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
*       fs_encod.c
*
* COMPONENT
*
*       Encode
*
* DESCRIPTION
*
*       Manages the encoding routines.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       cp_2_utf8
*       utf8_2_cp
*       utf8_to_unicode
*       unicode_to_utf8
*
*************************************************************************/
#include "nucleus.h"
#include "storage/encod_extr.h"
#include "storage/util_extr.h"
#include "storage/pcdisk.h"

#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CP_SUPPORT == 1)
#include "storage/uni_util_extr.h"
#endif

#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CP_SUPPORT == 0)
    CPTE_S CP_Table[CP_MAX_TABLE_SIZE] = {{CP_ASCII,{NU_NULL,NU_NULL}}};
#endif


/************************************************************************
* FUNCTION
*
*       cp_2_utf8
*
* DESCRIPTION
*
*       Each long filename entry has Unicode character strings. This
*       routine converts ASCII to Unicode.
*
*
* INPUTS
*
*       utf8 (out)                  UTF8 encoded character.
*       cp   (in)                   ASCII character.
*
* OUTPUTS
*
*       Number of bytes used to represent the character in UTF8.
*
*************************************************************************/
UINT8 cp_2_utf8(UINT8 *utf8,UINT8 *cp)
{
    *utf8 = *(cp+1);
    return 1;
}

/************************************************************************
* FUNCTION
*
*       utf8_2_cp
*
* DESCRIPTION
*
*       Each long filename entry has Unicode character strings. This
*       routine converts Unicode to ASCII.
*
*
* INPUTS
*
*       cp   (out)                  ASCII character.
*       utf8 (in)                   UTF8 encoded character.
*
* OUTPUTS
*
*       Number of bytes used to represent the character in codepage 
*       format.
*
*************************************************************************/
UINT8 utf8_2_cp(UINT8 *cp, UINT8 *utf8)
{

    *cp = *utf8;
    return 1;
}
/************************************************************************
* FUNCTION                                                              
*                                                                       
*       ascii_to_cp_format                                                         
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Converts an ASCII character into a two byte codepage 
*       codepoint value for internal use. The reason this is 
*       done is because the parser requires all codepage codepoints
*       to be two bytes.
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*       *cp_format(out)                     ASCII codepage codepoint
*                                           padded with a 0x00.
*       *ascii_cp(in)                       ASCII codepage value
*                                           that is only one byte
*                                           long.
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS                          ASCII codepoint was padded
*                                           0x00
*       NUF_BADPARAM                        ASCII character wasn't passed
*                                           in.
*                                                                       
*************************************************************************/
STATUS  ascii_to_cp_format(UINT8 *cp_format,UINT8 *ascii_cp)
{
    STATUS ret_val = NU_SUCCESS;
    /* Verify that codepage value passed in is ASCII. */
    if(*ascii_cp <= 0x7E)
    {
        /* Pad with 0x00 because ASCII is only 1 byte. */
        cp_format[0] = 0x00;   
        cp_format[1] = *ascii_cp;
    }
    else
        ret_val = NUF_BADPARM;
        
    return ret_val;
}
/************************************************************************
* FUNCTION                                                              
*                                                                       
*       ascii_cp_format_to_ascii                                                         
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Converts and ASCII character that is in the codepage format
*       of 0x00 0xASCII_VALUE to just a plan ASCII character.
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*       ascii_cp_format(out)                 Gets converted to 
*                                            plan ASCII character.
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS                          ASCII codepage codepoint
*                                           was converted.
*       NUF_BADPARAM                        Parameter wasn't a valid
*                                           ASCII codepage codepoint.
*                                           
*                                                                       
*************************************************************************/
STATUS  ascii_cp_format_to_ascii(UINT8 *ascii_cp_format)
{
    STATUS ret_val = NU_SUCCESS;
    
    /* Verify that codepage value passed in is ASCII. */
    if(ascii_cp_format[0] == 0x00 && ascii_cp_format[1] <= 0x7E)
    {
        /* Change to valid Microsoft codepage value. */
        ascii_cp_format[0] = ascii_cp_format[1];
        ascii_cp_format[1] = 0x00;
    }
    else
        ret_val = NUF_BADPARM;
    
    return ret_val;
    
}

#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CP_SUPPORT == 0)
/************************************************************************
* FUNCTION                                                              
*                                                                       
*       utf8_to_unicode                                                         
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Decodes a UTF8 character into a Unicode character. UTF8 is 
*       assumed to be in Little-Endian. Should always return two 
*       because Unicode only uses two bytes.
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*       unicode(out)                        Buff to copy the decoded UTF8 
*                                           character to.
*       utf8(in)                            UTF8 encoded character
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       Number of bytes used to represent the character in unicode.
*                                                                       
*************************************************************************/
UINT8 utf8_to_unicode(UINT8 *unicode,UINT8 *utf8)
{
    UINT8 bytes_in_unicode;

    *unicode = *utf8;
    *(unicode + 1) = 0x00;
    bytes_in_unicode = 2;

    return bytes_in_unicode;

}

/************************************************************************
* FUNCTION                                                              
*                                                                       
*       unicode_to_utf8                                                         
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Encodes a Unicode character into UTF8. Unicode is assumed to be
*       in Little-Endian. Should always return two because this is 
*       used when using the ASCII character set.
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*       utf8(out)                          Buff to copy the encoded 
*                                          UNICODE character to.
*       unicode(in)                        UNICODE character.
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       Number of bytes used to represent the unicode character in UTF8.
*                                                                       
*************************************************************************/
UINT8 unicode_to_utf8(UINT8 *utf8,UINT8 *unicode)
{
    *utf8 = unicode[0];    
    return 1;
}


#endif /* CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CP_SUPPORT == 0 */
