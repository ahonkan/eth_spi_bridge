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
*       uni_util.c
*                                                                       
* COMPONENT                                                              
*                                                                       
*       UNICODE
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Standard Unicode utility routines.
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       None.                                                           
*                                                                       
* FUNCTIONS                                                             
*       
*       NUF_Uni_Get_Char            Gets a requested character in a UTF8 encoded
*                                   string.
*       NUF_Next_Char               Acquires the next character in a UTF8 encoded
*                                   string.
*       NUF_Is_Equal                Compares two UTF8 encoded characters.
*       NUF_Valid_UTF8_Encoding     Verifies that a character is encoded in UTF8.
*       NUF_Copybuff                Copy one buffer to another.
*       NUF_Ncpbuf                  Copy up to size, stop at end of "from".
*       NUF_Get_CP_Loaded           Used to get a list of the current codepages loaded.
*       unicode_to_utf8             Decodes a utf8 character into a 2byte unicode value.
*       utf8_to_unicode             Encodes a 2 byte unicode character into utf8. 
*       uni_setup_cp                Sets up the conversion function pointers.
*       
*                                                                       
*************************************************************************/
#include "storage/pcdisk.h"
#include "storage/uni_util_extr.h"
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CP_SUPPORT == 1)

/* Fill codepage table. */
CPTE_S CP_Table[CP_MAX_TABLE_SIZE] =
{
#if (ENABLE_ASCII == NU_TRUE)
    {CP_ASCII,{NU_NULL,NU_NULL}},
#endif
#if (ENABLE_W_JAPANESE_SHIFT_JIS == NU_TRUE)
    {CP_JAPANESE_SHIFT_JIS,{NU_NULL,NU_NULL}},
#endif
#if (ENABLE_W_SIMPLIFIED_CHINESE == NU_TRUE)
    {CP_SIMPLIFIED_CHINESE,{NU_NULL,NU_NULL}},
#endif
#if (ENABLE_W_KOREAN == NU_TRUE)
    {CP_KOREAN,{NU_NULL,NU_NULL}},
#endif
#if (ENABLE_W_TRADITIONAL_CHINESE == NU_TRUE)
    {CP_TRADITIONAL_CHINESE,{NU_NULL,NU_NULL}},
#endif
};

/* Verify that they have the required files. */
#include "storage/trie_extr.h"

/************************************************************************
* FUNCTION
*
*       uni_setup_cp
*
* DESCRIPTION
*
*       Sets up the conversion function pointers for drive that
*       was just mounted.
*
* INPUTS
*
*       *mte                                Pointer to a mount table
*                                           entry.
*
* OUTPUTS
*
*       NU_SUCCESS                          Function pointers where setup.
*       NUF_BADPARM                         Codepage conversion functions
*                                           aren't supported or mte is
*                                           invalid.
*
*************************************************************************/
STATUS uni_setup_cp(MTE_S *mte)
{
    STATUS ret_val = NU_SUCCESS;

    if(mte)
    {
        switch(mte->mte_cp->cp)
        {
           
#if (ENABLE_W_JAPANESE_SHIFT_JIS == NU_TRUE)
            case CP_JAPANESE_SHIFT_JIS:
                mte->mte_cp->cp_op.cp_to_utf8 = Trie_CP932_Codepage_To_UTF8;
                mte->mte_cp->cp_op.utf8_to_cp = Trie_CP932_UTF8_To_Codepage;

                break;
#endif

#if (ENABLE_W_SIMPLIFIED_CHINESE == NU_TRUE)
            case CP_SIMPLIFIED_CHINESE:
                mte->mte_cp->cp_op.cp_to_utf8 = Trie_CP936_Codepage_To_UTF8;
                mte->mte_cp->cp_op.utf8_to_cp = Trie_CP936_UTF8_To_Codepage;

                break;
#endif
#if (ENABLE_W_KOREAN == NU_TRUE)
            case CP_KOREAN:
                mte->mte_cp->cp_op.cp_to_utf8 = Trie_CP949_Codepage_To_UTF8;
                mte->mte_cp->cp_op.utf8_to_cp = Trie_CP949_UTF8_To_Codepage;

                break;
#endif
#if (ENABLE_W_TRADITIONAL_CHINESE == NU_TRUE)
            case CP_TRADITIONAL_CHINESE:
                mte->mte_cp->cp_op.cp_to_utf8 = Trie_CP950_Codepage_To_UTF8;
                mte->mte_cp->cp_op.utf8_to_cp = Trie_CP950_UTF8_To_Codepage;

                break;
#endif

            default:
                ret_val = NUF_BADPARM;
                break;
        }/* switch */
      
    } /* mte is valid */
    else
        ret_val = NUF_BADPARM;

    return ret_val;
}
/************************************************************************
* FUNCTION                                                              
*                                                                       
*       NUF_Uni_Prev_Char                                                         
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Will get the previous character in a UTF8 encoded string.
*       This function is purely for speed it doesn't test for any 
*       possible errors.
*                                                                       
*                                                                       
* INPUTS                                                                
*       
*       **str(out)                          The address of the pointer to 
*                                           of the current character in a
*                                           string. The function will move this
*                                           pointer back one character.
*
* OUTPUTS                                                               
*                                                                       
*       None.  
*                                                                       
*                                                                     
*************************************************************************/    
VOID    NUF_Uni_Prev_Char(CHAR** str)
{
    
    while( ((UINT8)(*(--(*str))) < 0xc0) && ((UINT8) **str > 0x7f));
   
}
/************************************************************************
* FUNCTION                                                              
*                                                                       
*       NUF_Uni_Get_Char                                                         
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Sets pos_req to the char_req index in the string s. This 
*       function is essentially an overloaded array operator([]).
*                                                                       
*                                                                       
* INPUTS                                                                
*       
*       **pos_req(out)                      Pointer to the position
*                                           requested in the string s.
*       *s                                  A pointer to a string.
*       char_req                            The index of the character
*                                           requested. 
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       NUF_BADPARM                         If s isn't encoded in UTF8 
*                                           correctly.
*       NU_SUCCESS                          If the encoding is valid and
*                                           the assignment was successful.
*                                                                     
*************************************************************************/    
STATUS NUF_Uni_Get_Char(CHAR** pos_req,CHAR *s,INT char_req)
{
    STATUS ret_val = NU_SUCCESS;
    UNSIGNED_CHAR *trav_ptr = (UNSIGNED_CHAR*)s;
    INT8 num_bytes = 0;
    INT i;

    for(i = 1; i <= char_req && trav_ptr != NU_NULL; ++i)
    {
        /* This means character is represented in 1 byte. */
        if(*trav_ptr <= 0x7F)
        {
            num_bytes = 1;         
        }
        
        /* Character is represented using 2 bytes. */
        if(*trav_ptr >= 0xC2 && *trav_ptr <= 0xDF)
        {
            num_bytes = 2;            
        }
        
        /* Character is represented using 3 bytes. */
        if(*trav_ptr >= 0xE0 && *trav_ptr <= 0xEF)
        {
            num_bytes = 3;           
        }

        /* Character is represented using 4 bytes. */
        if(*trav_ptr >= 0xF0 && *trav_ptr <= 0xF4)
        {
            num_bytes = 4;            
        }
        
        ret_val = NUF_Valid_UTF8_Encoding((CHAR *)trav_ptr,num_bytes);
        if(ret_val != NU_SUCCESS)
        {
            trav_ptr = NU_NULL;
            ret_val = NUF_BADPARM;
        }

        else
        {
            trav_ptr += num_bytes;
        } 
        
    }  

   if(ret_val == NU_SUCCESS)
        *pos_req = (CHAR*)trav_ptr;
    
     return ret_val;
    
}
/************************************************************************
* FUNCTION                                                              
*                                                                       
*       NUF_Uni_Is_Equal                                                         
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Compares character s1 to see if it is equal to s2.
*       This function assumes that characters are being compared.                                                                        
*                                                                       
* INPUTS                                                                
*                                                                       
*       *s1                                 String to compare.
*       *s2                                 String to compare.
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       INT                                 0 if the s1 != s2
*                                           1 if s1 == s2
*
*                                                                       
*************************************************************************/    
INT     NUF_Uni_Is_Equal(const CHAR *s1,const CHAR *s2)
{
    STATUS ret_val;
    INT n = get_char_length(s1);

    if(n != get_char_length(s2))
        return 0;
           
    ret_val = NUF_Valid_UTF8_Encoding(s1,n);
    if(ret_val == NU_SUCCESS)
    {
        ret_val = NUF_Valid_UTF8_Encoding(s2,n);
        if(ret_val == NU_SUCCESS)
        {
            /* If n is less than or equal to zero than no characters where compared, so
               the return value is NU_SUCCESS. */
            while((n > 0))
            {   
                if((*s1) && (*s1 != *s2))
                    return 0;
                                
                ++s1,++s2,--n;
            }
        }
        else
            return 0;

    }

    return 1;

}
/************************************************************************
* FUNCTION                                                              
*                                                                       
*       NUF_Valid_UTF8_Encoding                                                         
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Verifies that encode_seq is a valid UTF8 character.
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*       *encode_seq                          Pointer to a character encoded 
*                                            in UTF8.
*       len                                  Number of bytes that are 
*                                            used in the encoding.
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       NUF_SUCCESS                         encode_seq is a valid UTF8 character.
*       NUF_BADPARM                         encode_seq isn't encoded correctly or
*                                           invalid.
*
*                                                                       
*************************************************************************/ 
STATUS  NUF_Valid_UTF8_Encoding(const CHAR *encode_seq,INT len)
{
    UNSIGNED_CHAR *str = (UNSIGNED_CHAR *)encode_seq;
    STATUS ret_val = NU_SUCCESS;
    UNSIGNED_CHAR next_char = *(str + 1);
    UNSIGNED_CHAR utf8_len;
    
    /* Validate parameters. */
    if(encode_seq == NU_NULL)
    {
        ret_val = NUF_BADPARM;
    }
    
    /* Check length versus the 1st byte. */
    switch(len)
    {
        case 1:
            /* First byte should be: 0xxxxxxx. */
            utf8_len = (UNSIGNED_CHAR)(str[0]>>7);
            if(utf8_len != 0x00)
                ret_val = NUF_BADPARM;

            break;
        
        case 2:
            /* First byte should be: 110xxxxx. */
            utf8_len = (UNSIGNED_CHAR)(str[0]>>6);
            if(utf8_len != 0x03)
                ret_val = NUF_BADPARM;

            break;

        case 3:
            /* First byte should be: 1110xxxx. */
            utf8_len = (UNSIGNED_CHAR)(str[0]>>5);
            if(utf8_len != 0x07)
                ret_val = NUF_BADPARM;

            break;

        case 4:
            /* First byte should be: 11110xxx. */
            utf8_len = (UNSIGNED_CHAR)(str[0]>>4);
            if(utf8_len != 0x0F)
                ret_val = NUF_BADPARM;

            break;

        default:
            ret_val = NUF_BADPARM;
            break;
    }

    if(ret_val == NU_SUCCESS)
    {
        if(len == 4)
        {
            if( *(str + 3) < 0x80 || *(str + 3) > 0xBF ) 
                ret_val = NUF_BADPARM;
        }
        if(ret_val != NUF_BADPARM && len >= 3)
        {
            if( *(str + 2) < 0x80 || *(str + 2) > 0xBF ) 
                ret_val = NUF_BADPARM;
        }
        
        if(ret_val != NUF_BADPARM && len >= 2)
        {
            /* Handle special cases for the 2nd byte. */
            switch(*str)
            {
           
            case 0xE0:
                if(*(str + 1) < 0xA0 || *(str + 1) > 0xBF)
                    ret_val = NUF_BADPARM;
                break;

            case 0xED:
                if( *(str + 1) < 0x80 || *(str + 1) > 0x9F)
                    ret_val = NUF_BADPARM;
                break;

            case 0xF0:
                if( *(str + 1) < 0x90 || *(str + 1) > 0xBF)
                    ret_val = NUF_BADPARM;
                break;

            case 0xF4:
                if( *(str + 1) < 0x80 || *(str + 1) > 0x8F)
                    ret_val = NUF_BADPARM;
                break;

            default:
                if(next_char < 0x80 || next_char > 0xBF)
                    ret_val = NUF_BADPARM;
                
                break;

            }
        }
        
        if(ret_val != NUF_BADPARM && len == 1)
        {
            if( *str >= 0x80 && *str <= 0xC1 )
               ret_val = NUF_BADPARM;
        }
        
    }/* Close if */

    return ret_val;
}
/************************************************************************
* FUNCTION                                                              
*                                                                       
*       get_char_length                                                         
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Determines the number of bytes used to represent this character.
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*       *utf8_char                          A pointer to a character 
*                                           encoded using UTF-8.
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       INT                                 Number of bytes used to represent
*                                           this character.
*
*                                                                       
*************************************************************************/ 
INT get_char_length(const CHAR *utf8_char)
{    
    INT num_of_bytes = 0;
    
    if((UNSIGNED_CHAR)*utf8_char <= 0x7F)
        num_of_bytes = 1;
    else
    {   
        if((UNSIGNED_CHAR)*utf8_char >= 0xC2 && (UNSIGNED_CHAR)*utf8_char < 0xE0)
            num_of_bytes = 2;
        else
        {
            if((UNSIGNED_CHAR)*utf8_char >= 0xE0 && (UNSIGNED_CHAR)*utf8_char < 0xF0)
            {
                /* Special case for 0xE5. */   
                if(NUF_Valid_UTF8_Encoding(utf8_char,3)!= NU_SUCCESS)
                {
                    num_of_bytes = 1;     
                }
                else
                    num_of_bytes = 3;
            }
            else
            {
                if((UNSIGNED_CHAR)*utf8_char >= 0xF0 && (UNSIGNED_CHAR)*utf8_char < 0xF4)
                     num_of_bytes = 4;
            }
        }
    }
    return num_of_bytes;
}

/************************************************************************
* FUNCTION                                                              
*                                                                       
*       NUF_Uni_Ncpbuf                                                       
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Copy up to size characters(encoded in UTF8) from "from" to "to".  
*       Stop copy when at end of "from" or size is 0.                                                                
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*       *to                                 Copy to data buffer.        
*       *from                               Copy from data buffer.      
*       size                                Number of characters.             
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None.                                                           
*                                                                       
*************************************************************************/
VOID    NUF_Uni_Ncpbuf(UINT8 *to,UINT8 *from,INT size)
{
    INT utf8_char_len;
    UINT8 *orig_to = to;
    STATUS ret_val;    
    
    /* Do not attempt using a pointer to 0x0 */
    if((to != NU_NULL) && (from != NU_NULL) && (size >= 0))
    {
        while (size--)
        {
            if (*from)
            {
               /* Determine the number of bytes used to represent this character. */
               utf8_char_len = get_char_length((CHAR*)from);
               
               /* Verify that the character is encoded correctly. */
               ret_val = NUF_Valid_UTF8_Encoding((CHAR*)from,utf8_char_len);
               /* If 'from' is not encoded correctly do not copy it to 'to'.*/
               if(ret_val != NU_SUCCESS)
               {
                   to = orig_to;
                   *to = '\0';
                   break;
               }
               while(utf8_char_len--)
               {
                    *to++ = *from++;
               }
            }                
            else
            {
                *to = '\0';
                break;
            }
        }
    }
    else if(to != NU_NULL)
        *to = '\0';
}
/************************************************************************
* FUNCTION                                                              
*                                                                       
*       NUF_Copybuff                                                         
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Copies n characters from vfrom to vto. Similar to memcpy 
*       and strncpy. Assume that vfrom is encoded correctly in UTF-8.
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*       *vto                                Copy to data buffer         
*       *vfrom                              Copy from data buffer       
*       n                                   Number of characters.                  
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None.                                                           
*                                                                       
*************************************************************************/
VOID    NUF_Uni_Copybuff(VOID *vto,VOID *vfrom, INT n)
{
    UINT8 *to            = (UINT8 *)vto;
    UINT8 *from          = (UINT8 *)vfrom;
    INT  utf8_char_len;
    
    while (n--)
    {
        /* Determine the number of bytes used to represent this character. */
        utf8_char_len = get_char_length((CHAR*)from);

        while(utf8_char_len--)
        {
            *to++ = *from++;
        }
    
    }
    
}
/************************************************************************
* FUNCTION                                                              
*                                                                       
*       NUF_Get_CP_Loaded                                                         
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Fills the array cp_loaded with the available codepages.
*       The number of codepages can be determined at runtime using
*       CP_MAX_TABLE_SIZE.
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*       *cp_loaded(out)                     Pointer to an array to 
*                                           be filled with available 
*                                           codepages that the system
*                                           can use.
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None.
*                                                                       
*************************************************************************/
VOID    NUF_Get_CP_Loaded(INT *cp_loaded)
{
    INT *cp_loaded_ptr = cp_loaded;
    
#if (ENABLE_ASCII == NU_TRUE)
    *cp_loaded_ptr = CP_ASCII;
    ++cp_loaded_ptr;
#endif

#if (ENABLE_W_JAPANESE_SHIFT_JIS == NU_TRUE)
    *cp_loaded_ptr = CP_JAPANESE_SHIFT_JIS;
    ++cp_loaded_ptr;
#endif

#if (ENABLE_W_SIMPLIFIED_CHINESE == NU_TRUE)
    *cp_loaded_ptr = CP_SIMPLIFIED_CHINESE;
    ++cp_loaded_ptr;
#endif

#if (ENABLE_W_KOREAN == NU_TRUE)
    *cp_loaded_ptr = CP_KOREAN;
    ++cp_loaded_ptr;
#endif

#if (ENABLE_W_TRADITIONAL_CHINESE == NU_TRUE)
    *cp_loaded_ptr = CP_TRADITIONAL_CHINESE;    
#endif   


}
/************************************************************************
* FUNCTION                                                              
*                                                                       
*       utf8_to_unicode                                                         
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Decodes a UTF-8 character and stores its Unicode representation into
*       the buffer being pointed to by unicode. UTF8 is assumed to be in 
*       Little-Endian.
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*       *unicode(out)                       Buffer to copy the decoded 
*                                           UTF-8 character to.
*       *utf8(in)                           Pointer to a UTF-8 encoded 
*                                           character.
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       Number of bytes in unicode.
*                                                                       
*************************************************************************/
UINT8 utf8_to_unicode(UINT8 *unicode,UINT8 *utf8)
{
    UINT8 num_of_bytes = (UINT8)get_char_length((CHAR*)utf8);
    UINT8 byte1;
    UINT8 byte2;
    UINT8 temp;
    UINT8 bytes_in_unicode = 0;
    
    switch(num_of_bytes) {

    /* ASCII support */
    case 1:
        *unicode = *utf8;
        *(unicode + 1) = 0x00;

        bytes_in_unicode = 2;

        break;
        /* Unicode encoded character takes 2 bytes. */
    case 2:     
        byte2 = utf8[1];
        byte2 <<= 2;
        byte2 >>=2;
        
        temp = *utf8;
        byte1 = temp;

        /*byte 2*/
        temp <<= 6;
        byte2 |= temp;

        /* Get byte 1*/
        byte1 &= 0x1c;
        byte1>>=2;

        *unicode = byte2;
        *(unicode + 1) = byte1;
        bytes_in_unicode = 2;

        break;

        /* Unicode encoded character takes 2 bytes. */
    case 3:

        /* 3rd byte*/
        byte2 = utf8[2];
        byte2 <<= 2;
        byte2 >>=2;

        temp = utf8[1];
        temp <<= 6;
        byte2 |= temp;

        temp = utf8[1];
        /* Zero out bit 7 & 6. */
        temp &= 0x3f;
        /* Get ride of bit 0 and 1 because they are in byte 2. */
        temp >>=2;

        byte1 = *utf8;
        /* Move bits 7-4 to correct position. */
        byte1 <<= 4;

        byte1 |= temp;

        *unicode = byte2;
        *(unicode + 1) =  byte1;

        bytes_in_unicode = 2;

        break;
    
    default:
        break;
    }

    return bytes_in_unicode;

}
/************************************************************************
* FUNCTION                                                              
*                                                                       
*       unicode_to_utf8                                                         
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Takes the a Unicode represented character being pointed to by unicode 
*       and stores its UTF-8 representation in the buffer being pointed to by 
*       utf8. Unicode is assumed to be in Little-Endian.
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*       *utf8(out)                         Pointer to a buffer to
*                                          be filled with the UTF-8
*                                          representation of unicode.
*       *unicode(in)                       A pointer to a unicode 
*                                          character.
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       Number of bytes of utf8.
*                                                                       
*************************************************************************/
UINT8 unicode_to_utf8(UINT8 *utf8,UINT8 *unicode)
{
    UINT8 byte1;
    UINT8 byte2;
    UINT8 byte3;
    UINT8 temp;
    UINT8 ret_val;
    
    /* ASCII SUPPORT 2 bytes in UTF8 */
    if (*(unicode+1) == (UINT8) 0x00 && *(unicode) <= (UINT8) 0x7F)
    {
        *utf8 = unicode[0];
        ret_val = 1;
    }
    else
    {  
        /* Will require 3 bytes to be encoded in UTF-8. */
        /* Range 0x0800-0xFFFF*/     
        if(*(unicode+1) >= 0x08)
        {
            /* The encoding in UTF8 should result in the following: */
            /* Let b# represent bit number in the 2 byte unicode value passed in. */
            /* Byte1:1110 b7 b6 b5 b4 */
            /* Byte2:10 b3 b2 b1 b0 b15 b14 */
            /* Byte3:10 b13 b12 b11 b10 b9 b8 */
            
            /* Format the 3rd UTF8 byte. */
            byte3 = *unicode;
            /* Get rid of the first 2 bits. */
            byte3 &= 0x3f;
            /* Add the utf8 specific encoding bytes to bit 7 and 8. */
            byte3 |= 0x80;
            
            /* Format the 2nd UTF8 byte.*/
            byte2 = *unicode;
            /* Shift bits 7 and 8 to the right because the other bits are stored in byte 3. */
            byte2 >>=6;
            /* Add the UTF8 specific encoding bits to bit 7 and 8. */
            byte2 |= 0x80;

            /* Get the next Unicode byte so that bits 0-3 can be stored in byte2. */
            temp = unicode[1];            
            /* Get just bits 0-3 inclusive. */
            temp <<=4;
            temp >>= 2;
            temp |= 0x80;

            /* Form byte 2 in our UTF8 sequence. */
            byte2 |= temp;
            
            /* Store bits 7-4 in the leading byte of our UTF8 sequence. */
            byte1 = unicode[1];
            byte1 >>= 4;
            byte1 |= 0xE0;

            *utf8 = byte1;
            *(utf8 + 1) = byte2;
            *(utf8 + 2) = byte3;
            ret_val = 3;
        }
        /* Only requires 2 bytes to be encoded in utf8. */
        /* UNICODE range: 0x0080 - 0x07FF*/
        else
        {
            /* The encoding in UTF8 should result in the following: */
            /* Let b# represent bit number in the 2 byte unicode value passed in. */
            /* Byte1:110 b10 b9 b8 b7 b6 */
            /* Byte2:10 b5 b4 b3 b2 b1 b0 */
           
            /* Setup the second byte. */
            byte2 = *unicode;
            byte2 &= 0x3f;
            byte2 |= 0x80;

            /* Setup the first byte. */
            temp = *unicode;
            temp >>= 6;
            
            byte1 = unicode[1];
            byte1 <<= 2;
            byte1 |= 0xC0;
            byte1 |= temp;

            *utf8 = byte1;
            *(utf8+1) = byte2;

            ret_val = 2;

        }
    }
     
    return ret_val;
}

/************************************************************************
* FUNCTION                                                              
*                                                                       
*       NUF_Codepage_To_UTF8                                                         
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Takes a null terminated codepage string or character and 
*       converts it to its UTF-8 representation.
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*       *utf8_buff(out)                 Pointer to a buffer to
*                                       be filled with the UTF-8
*                                       representation of cp.
*       *cp(in)                         A pointer to a codepage
*                                       string or character.
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       NUF_BADPARM                     Invalid parameter specified.
*       NUF_SUCCESS                     Conversion was successful.
*                                                                       
*************************************************************************/
STATUS NUF_Codepage_To_UTF8(UINT8 *utf8_buff, UINT8 *cp,UINT8 (*cp_to_utf8_func)(UINT8*,UINT8*) )
{
    UINT16 len;
    STATUS ret_val = NU_SUCCESS;
    UINT8 *utf8_buff_trav = utf8_buff;

    /* Make sure we have a valid cp string. */
    if(cp[0] != 0x00 || cp[1] != 0x00)
    {
        /* Keep converting the cp string until we reach a null terminated character. 
           A null terminated character in codepage format would be two bytes both
           containing a 0x00 value. */
        for(len = 0; cp[len] != 0x00 || cp[len + 1] != 0x00;len+=2)
        {
            cp_to_utf8_func(utf8_buff_trav,(UINT8*)&cp[len]);        
            NUF_NEXT_CHAR(utf8_buff_trav);
        }
    }
    else
        ret_val = NUF_BADPARM;

    /* Add NULL terminating character to UTF8 string. */
    *utf8_buff_trav = 0x00;

    return ret_val;    
}

/************************************************************************
* FUNCTION                                                              
*                                                                       
*       NUF_UTF8_To_Codepage                                                         
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Takes a null terminated UTF-8 string or character and 
*       converts it to its codepage representation.
*                                                                       
*                                                                       
* INPUTS                                                                
*                                                                       
*       *cp_buff(out)                   Pointer to a buffer to
*                                       be filled with the UTF-8
*                                       representation of utf8.
*       *utf8(in)                       A pointer to a unicode 
*                                       character.
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       NUF_BADPARM                     Invalid parameter specified.
*       NUF_SUCCESS                     Conversion was successful.
*                                                                       
*************************************************************************/
STATUS NUF_UTF8_To_Codepage(UINT8 *cp_buff, UINT8 *utf8,UINT8 (*utf8_to_cp_func)(UINT8*,UINT8*) )
{    
    STATUS ret_val = NU_SUCCESS;
    UINT8 *cp_buff_trav = cp_buff;
    

    if(*utf8 != 0x00 && utf8 != NU_NULL)
    {
        while(*utf8 != 0x00)
        {
            cp_buff_trav += utf8_to_cp_func(cp_buff_trav,utf8);        
            NUF_NEXT_CHAR(utf8);
        }
    }
    else
        ret_val = NUF_BADPARM;

    /* Add NULL terminating character to codepage string. Two null terminated characters
       are added because one codepage character always takes up two bytes. */
    *cp_buff_trav++ = 0x00;
    *cp_buff_trav = 0x00;


    return ret_val;
}


#endif /* CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CP_SUPPORT == 1 */


