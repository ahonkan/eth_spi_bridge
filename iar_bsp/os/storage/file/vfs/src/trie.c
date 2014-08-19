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
*       trie.c
*                                                                       
* COMPONENT                                                             
*                                                                       
*       TRIE
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Conversion functions used by FILE to support Unicode.
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       None.                                                           
*                                                                       
* FUNCTIONS                                                
*                                                                       
*       trie_get_val                        Gets a value from the block
*       Trie_UTF8_To_Unicode                Decodes a UTF8 character into 
*                                           a Unicode character.  
*       Trie_Unicode_To_UTF8                Encodes a Unicode character 
*                                           into UTF8.
*       Trie_Codepage_To_UTF8               Converts an encoded codepage
*                                           to its UTF8 format.
*       Trie_utf8_to_unicode                Big endian conversion from
*                                           utf8 to unicode.
*       Trie_UTF8_To_Codepage               Converts a UTF8 encoded 
*                                           character to its codepage
*                                           representation.
*       
*                                                                       
*************************************************************************/
#include "nucleus.h"
#include "storage/trie_extr.h"
#include "storage/uni_util_extr.h"
#include "storage/file_cfg.h"
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CP_SUPPORT == 1)

#define TRIE_HI_MASK 0xFF00
#define TRIE_LO_MASK 0x00FF

extern INT get_char_length(const CHAR *);

/************************************************************************
* FUNCTION
*
*       trie_get_val
*
* DESCRIPTION
*
*       Given a key and the address of the index, return the value
*       from the block
*
*
* INPUTS
*
*       key                            Pointer to the value trying 
*                                      to be converted.
*       *idx                           Pointer to the codepage 
*                                      a codepage conversion table.
*
* OUTPUTS
*
*       Value from the block.
*
*************************************************************************/
UINT16 trie_get_val(UINT16 key, VOID *idx)
{
UINT16 block_key;
UINT16 low_key;

UINT32 *u32_blockp;
UINT32 *u32_idxp;
UINT16 *u16_idxp;

UINT16 val = 0;


    if (idx)
    {

        /* Compare using matching data types */    
        u32_blockp = idx;
        block_key = (key&TRIE_HI_MASK);
        block_key >>= 8;
    
        u32_idxp = (UINT32*)*(u32_blockp + (block_key));
        
        if (u32_idxp)
        {

            low_key = (key & TRIE_LO_MASK);
            u16_idxp = (UINT16*)u32_idxp;
               val = *(u16_idxp + (low_key));
        }
        else
            val = key;        
        
    }

    return (val);
}

/************************************************************************
* FUNCTION
*
*       Trie_Codepage_To_UTF8
*
* DESCRIPTION
*
*       Takes the codepage character pointed to by cp and stores the 
*       encoded UTF-8 representation into the buffer being pointed to by
*       utf8. cp should be two bytes long.
*
*
* INPUTS
*
*       *utf8(out)                          Pointer to a buffer to
*                                           be filled with the UTF-8
*                                           representation of cp.
*       *cp(in)                             Pointer to a character
*                                           in codepage format.
*       *trie_idx                           Pointer to the index value 
*                                           for that codepage table.
*
* OUTPUTS
*
*       Number of bytes used to represent utf8.
*
*************************************************************************/
UINT8 Trie_Codepage_To_UTF8(UINT8 *utf8, UINT8 *cp, void *trie_idx)
{
    UINT8 uni[2];
    UINT8 utf8_length;
    UINT16 trie_val,us_tval;


    if (*(cp) == 0x00 && *(cp+1) <= 0x7e) /* Max ASCII value */
    {        
        us_tval = (UINT16)(cp[1]);
    }        
    else
    {           
        us_tval = (UINT16)(cp[0]<<8);
        us_tval += (UINT16)(cp[1]);
    }

    trie_val = trie_get_val(us_tval, trie_idx);
    
    if (trie_val & 0xff00)
    {
        uni[0] = (UINT8)(trie_val & 0x00FF);
        uni[1] = (UINT8)((trie_val & 0xFF00) >> 8);        
    }
    else 
    {
        uni[0] = (UINT8)(trie_val & 0x00FF);
        uni[1] = 0;        
    }

    utf8_length = unicode_to_utf8(utf8, &uni[0]);
    return(utf8_length); 
}

/************************************************************************
* FUNCTION
*
*       Trie_utf8_to_unicode
*
* DESCRIPTION
*
*       Decodes a UTF8 character being pointed to by utf8 and stores its 
*       Unicode representation into the buffer being pointed to by unicode.
*       Byte order is in big endian.
*
*
* INPUTS
*
*       *unicode(out)                        Pointer to a buffer 
*                                            where the UTF-8 decoded
*                                            character will be placed.
*       *utf8(in)                            Pointer to a UTF-8 character.
*
* OUTPUTS
*
*       Number of bytes used to represent unicode.
*
*************************************************************************/
static UINT8 Trie_utf8_to_unicode(UINT8 *unicode,UINT8 *utf8)
{
    UINT8 num_of_bytes = (UINT8)get_char_length((CHAR*)utf8);
    UINT8 byte1;
    UINT8 byte2;
    UINT8 temp;
    UINT8 bytes_in_unicode = 0;
    
    switch(num_of_bytes) {

    /* ASCII support */
    case 1:
        *(unicode + 1) = *utf8;
        *unicode = 0x00;

        bytes_in_unicode = 2;

        break;
        /* utf8 encoded character takes 2 bytes. */
    case 2:
        utf8++;
        byte2 = *utf8;
        byte2 <<= 2;
        byte2 >>=2;

        --utf8;
        temp = *utf8;
        byte1 = temp;

        /*byte 2*/
        temp <<= 6;
        byte2 |= temp;

        /* Get byte 1*/
        byte1 &= 0x1c;
        byte1>>=2;

        *unicode = byte1;
        *(unicode + 1) = byte2;
        bytes_in_unicode = 2;

        break;

        /* utf8 encoded character takes 3 bytes. */
    case 3:

        /* 3rd byte*/
        utf8 +=2;
        byte2 = *utf8;
        byte2 <<= 2;
        byte2 >>=2;

        --utf8;
        temp = *utf8;
        temp <<= 6;
        byte2 |= temp;

        temp = *utf8;
        /* Zero out bit 7 & 6. */
        temp &= 0x3f;
        /* Get ride of bit 0 and 1 because they are in byte 2. */
        temp >>=2;

        --utf8;
        byte1 = *utf8;
        /* Move bits 7-4 to correct position. */
        byte1 <<= 4;

        byte1 |= temp;

        *unicode = byte1;
        *(unicode + 1) =  byte2;

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
*       Trie_UTF8_To_Codepage
*
* DESCRIPTION
*
*       Decodes a UTF-8 character being pointed to by utf8 and stores 
*       its codepage representation in the buffer being pointed to by cp.
*       
*
*
* INPUTS
*
*       *cp(out)                            Pointer to buffer where
*                                           the codepage representation
*                                           of utf8 will be stored.
*       *utf8(in)                           Pointer to a UTF-8 
*                                           character. 
*       *trie_idx                           Pointer to the index value 
*                                           for that codepage table.
*
* OUTPUTS
*
*       Number of bytes used to represent cp.
*
*************************************************************************/
UINT8 Trie_UTF8_To_Codepage(UINT8 *cp, UINT8 *utf8, void *trie_idx)
{
    UINT8 uni[4];
    UINT8 ret_len;
    UINT16 key;
    UINT16 trie_val;

    Trie_utf8_to_unicode(uni, utf8);
    
    key =((UINT16)uni[0]);
    key <<= 8;
    key |= (UINT16)uni[1];
    
     
    trie_val = trie_get_val(key, trie_idx);
    
    /* If ASCII our codepage value is only one byte. */
    if(*utf8 <= 0x7F) 
    {
        *cp++ = 0x00;
        ret_len = 2;
    }
    else
    {
        *cp++ = (UINT8)((trie_val & 0xff00) >> 8);
        ret_len = 2;
    } 
    *(cp) = (UINT8)(trie_val & 0x00ff);

    return (ret_len);

}
/************************************************************************
* FUNCTION
*
*       Trie_CP932_Codepage_To_UTF8
*
* DESCRIPTION
*
*       Wrapper function for codepage 932. Calls the conversion function
*       that decodes the codepage character pointed to by cp and converts
*       that character to a UTF-8 representation. The UTF-8 representation
*       is stored into the buffer that utf8 points to.
*
*
*
* INPUTS
*
*       *cp(in)                             Pointer to a character in
*                                           codepage format.
*       *utf8(out)                          Pointer to a buffer to
*                                           be filled with the UTF-8
*                                           representation of cp.
*
* OUTPUTS
*
*       Number of bytes used to represent UTF8 encoded character.
*
*************************************************************************/
UINT8 Trie_CP932_Codepage_To_UTF8(UINT8 *utf8, UINT8 *cp)
{
    return (Trie_Codepage_To_UTF8(utf8, cp, cp932_CP_idx));
}
/************************************************************************
* FUNCTION
*
*       Trie_CP932_UTF8_To_Codepage
*
* DESCRIPTION
*
*       Wrapper function for codepage 932. Calls the conversion function
*       that decodes a UTF-8 encoded character being pointed to by utf8 
*       and then encodes that in its codepage representation. The codepage 
*       representation is then stored in the buffer pointed to by cp.
*
*
*
* INPUTS
*
*       *utf8(in)                           Pointer to a UTF-8 encoded 
*                                           character.
*       *cp(out)                            Pointer to a buffer to
*                                           be filled with the codepage
*                                           representation of utf8.
*
* OUTPUTS
*
*       Number of bytes used to represent the codepage encoded character.
*
*************************************************************************/
UINT8 Trie_CP932_UTF8_To_Codepage(UINT8 *cp, UINT8 *utf8)
{
    return (Trie_UTF8_To_Codepage(cp, utf8, cp932_UNI_idx));
}
/************************************************************************
* FUNCTION
*
*       Trie_CP936_Codepage_To_UTF8
*
* DESCRIPTION
*
*       Wrapper function for codepage 936. Calls the conversion function
*       that decodes the codepage character pointed to by cp and converts
*       that character to a UTF-8 representation. The UTF-8 representation
*       is stored into the buffer that utf8 points to.
*
*
*
* INPUTS
*
*       *cp(in)                             Pointer to a character in
*                                           codepage format.
*       *utf8(out)                          Pointer to a buffer to
*                                           be filled with the UTF-8
*                                           representation of cp.
*
* OUTPUTS
*
*       Number of bytes used to represent UTF8 encoded character.
*
*************************************************************************/
UINT8 Trie_CP936_Codepage_To_UTF8(UINT8 *utf8, UINT8 *cp)
{
    return (Trie_Codepage_To_UTF8(utf8, cp, cp936_CP_idx));
}
/************************************************************************
* FUNCTION
*
*       Trie_CP936_UTF8_To_Codepage
*
* DESCRIPTION
*
*       Wrapper function for codepage 936. Calls the conversion function
*       that decodes a UTF-8 encoded character being pointed to by utf8 
*       and then encodes that in its codepage representation. The codepage 
*       representation is then stored in the buffer pointed to by cp.
*
*
*
* INPUTS
*
*       *utf8(in)                           Pointer to a UTF-8 encoded 
*                                           character.
*       *cp(out)                            Pointer to a buffer to
*                                           be filled with the codepage
*                                           representation of utf8.
*
* OUTPUTS
*
*       Number of bytes used to represent the codepage encoded character.
*
*************************************************************************/
UINT8 Trie_CP936_UTF8_To_Codepage(UINT8 *cp, UINT8 *utf8)
{
    return (Trie_UTF8_To_Codepage(cp, utf8, cp936_UNI_idx));
}
/************************************************************************
* FUNCTION
*
*       Trie_CP949_Codepage_To_UTF8
*
* DESCRIPTION
*
*       Wrapper function for codepage 949. Calls the conversion function
*       that decodes the codepage character pointed to by cp and converts
*       that character to a UTF-8 representation. The UTF-8 representation
*       is stored into the buffer that utf8 points to.
*
*
*
* INPUTS
*
*       *cp(in)                             Pointer to a character in
*                                           codepage format.
*       *utf8(out)                          Pointer to a buffer to
*                                           be filled with the UTF-8
*                                           representation of cp.
*
* OUTPUTS
*
*       Number of bytes used to represent UTF8 encoded character.
*
*************************************************************************/
UINT8 Trie_CP949_Codepage_To_UTF8(UINT8 *utf8, UINT8 *cp)
{
    return (Trie_Codepage_To_UTF8(utf8, cp, cp949_CP_idx));
}
/************************************************************************
* FUNCTION
*
*       Trie_CP949_UTF8_To_Codepage
*
* DESCRIPTION
*
*       Wrapper function for codepage 949. Calls the conversion function
*       that decodes a UTF-8 encoded character being pointed to by utf8 
*       and then encodes that in its codepage representation. The codepage 
*       representation is then stored in the buffer pointed to by cp.
*
*
*
* INPUTS
*
*       *utf8(in)                           Pointer to a UTF-8 encoded 
*                                           character.
*       *cp(out)                            Pointer to a buffer to
*                                           be filled with the codepage
*                                           representation of utf8.
*
* OUTPUTS
*
*       Number of bytes used to represent the codepage encoded character.
*
*************************************************************************/
UINT8 Trie_CP949_UTF8_To_Codepage(UINT8 *cp, UINT8 *utf8)
{
    return (Trie_UTF8_To_Codepage(cp, utf8, cp949_UNI_idx));
}
/************************************************************************
* FUNCTION
*
*       Trie_CP950_Codepage_To_UTF8
*
* DESCRIPTION
*
*       Wrapper function for codepage 950. Calls the conversion function
*       that decodes the codepage character pointed to by cp and converts
*       that character to a UTF-8 representation. The UTF-8 representation
*       is stored into the buffer that utf8 points to.
*
*
*
* INPUTS
*
*       *cp(in)                             Pointer to a character in
*                                           codepage format.
*       *utf8(out)                          Pointer to a buffer to
*                                           be filled with the UTF-8
*                                           representation of cp.
*
* OUTPUTS
*
*       Number of bytes used to represent UTF8 encoded character.
*
*************************************************************************/
UINT8 Trie_CP950_Codepage_To_UTF8(UINT8 *utf8, UINT8 *cp)
{
    return (Trie_Codepage_To_UTF8(utf8, cp, cp950_CP_idx));
}
/************************************************************************
* FUNCTION
*
*       Trie_CP950_UTF8_To_Codepage
*
* DESCRIPTION
*
*       Wrapper function for codepage 950. Calls the conversion function
*       that decodes a UTF-8 encoded character being pointed to by utf8 
*       and then encodes that in its codepage representation. The codepage 
*       representation is then stored in the buffer pointed to by cp.
*
*
*
* INPUTS
*
*       *utf8(in)                           Pointer to a UTF-8 encoded 
*                                           character.
*       *cp(out)                            Pointer to a buffer to
*                                           be filled with the codepage
*                                           representation of utf8.
*
* OUTPUTS
*
*       Number of bytes used to represent the codepage encoded character.
*
*************************************************************************/
UINT8 Trie_CP950_UTF8_To_Codepage(UINT8 *cp, UINT8 *utf8)
{
    return (Trie_UTF8_To_Codepage(cp, utf8, cp950_UNI_idx));
}
#endif /* CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CP_SUPPORT == 1 */
