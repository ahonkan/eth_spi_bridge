/***************************************************************************
*
*           Copyright 1998 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
****************************************************************************

****************************************************************************
*
* FILE NAME                                             
*
*  str_utils.c                                                  
*
* DESCRIPTION
*
*  This file contains the GRAFIX string utility functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  STR_all_trim
*  STR_between
*  STR_empty
*  STR_in_between
*  STR_mem_chr
*  STR_mem_cmp
*  STR_mem_cpy
*  STR_mem_move
*  STR_mem_set
*  STR_str_cat
*  STR_str_chr
*  STR_str_cmp
*  STR_str_cpy
*  STR_str_dup
*  STR_str_len
*  STR_strr_chr
*  STR_str_rev  
*  STR_str_spn
*  STR_str_str
*  STR_str_tok
*  STR_stri_cmp
*  STR_strn_cmp
*  STR_strn_cpy
*  STR_sub_str
*  
* DEPENDENCIES
*
*  rs_base.h
*  rsfonts.h
*  str_utils.h
*  global.h
*
***************************************************************************/

#include "ui/rs_base.h" 
#include "ui/rsfonts.h"
#include "ui/str_utils.h" 
#include "ui/global.h"

#ifdef USE_UNICODE
	UNICHAR  EMPTY_STR[] = {0x0000};
	UNICHAR  SPACE_STR[2]= {' ',0};
#else
	UNICHAR  EMPTY_STR[] = "";
	UNICHAR  SPACE_STR[] = " ";
#endif

/***************************************************************************
* FUNCTION
*
*    STR_all_trim
*
* DESCRIPTION
*
*    Function STR_all_trim trims leading and trailing spaces from a string.
*
* INPUTS
*
*    UNICHAR *str - Pointer to the string to check.
*
* OUTPUTS
*
*    UNICHAR - Returns pointer to the resulting string.
*
***************************************************************************/
UNICHAR *STR_all_trim(UNICHAR *str)
{
    UNICHAR     *e;
    UNICHAR     *p;
    UNICHAR     *temp;

    p = str;
    e = str + STR_str_len(p) - 1;

    /* Find first non-space character, searching backwards from the end 
       of the string. */
    while( e > p && *e == ' ' )                                     
    {
        e--;
    }

    /* Trim trailing spaces, if any. */
    *(e+1) = '\0';

    /* Find first non-space character. */
    while( *p && *p == ' ' ) 
    {
        p++;
    }
    
    /* Trim leading spaces, if any, overwriting them by moving the string 
       to start, starting from first non-space character. */
    if (*p != '\0' && p != str)
    {                                          
        temp = str;
                                                
        while (*p != '\0')
        {
            *temp = *p;
            temp++;
            p++;
        }
        
        /* Null terminate the moved string. */
        *temp = '\0';
    }
    
    return (str);
}

/***************************************************************************
* FUNCTION
*
*   STR_between
*
* DESCRIPTION
*
*    Function STR_between test for a value to be in a specified range up to and
*    including range min and max.
*
* INPUTS
*
*    INT32 a - Value to test.
*    INT32 b - Lower value of range to check.
*    INT32 c - Upper value of range to check.
*
* OUTPUTS
*
*    INT32 - Returns 1 if within range.
*
***************************************************************************/
INT32 STR_between(INT32 a, INT32 b, INT32 c)
{

    return (a >= b && a <= c);

}

/***************************************************************************
* FUNCTION
*
*    STR_empty
*
* DESCRIPTION
*
*    Function STR_empty tests for an empty string.
*
* INPUTS
*
*    UNICHAR *str - Pointer to the string to check.
*
* OUTPUTS
*
*    INT32 - Returns 1 string if empty.
*
***************************************************************************/
INT32 STR_empty(UNICHAR *str)
{
    INT32       len;
    INT32       ret_val = 1;
    UINT8       done = NU_FALSE;
    UNICHAR     *p;

    /* if its NULL return true */
    if( str != NU_NULL ) 
    {
        /* get the length of the string */
        len = STR_str_len(str);

        /* if there are no characters in the */
        /* string it must be empty so return true */
        if( len != 0 )
        {
            /* if there is only whitespace in the string */
            /* it is considered to be empty so return true */
            p = str;

            while( *p && !done)
            {
                if( *p != ' ' ) 
                {
                    ret_val = 0;
                    done = NU_TRUE;
                }

                if (!done)
                {
                    p++;
                }
            }
        }
    }

    return (ret_val);
}

/***************************************************************************
* FUNCTION
*
*   STR_in_between
*
* DESCRIPTION
*
*    Function STR_in_between test for a value to be in a specified range up to
*    an excluding range min and max.
*
* INPUTS
*
*    INT32 a - Value to test.
*    INT32 b - Lower value of range to check.
*    INT32 c - Upper value of range to check.
*
* OUTPUTS
*
*    INT32 - Returns 1 if between range limits.
*
***************************************************************************/

INT32 STR_in_between(INT32 a, INT32 b, INT32 c)
{

    return (a > b && a < c);

}

/*************************************************************************
*
*   FUNCTION
*
*       STR_mem_chr
*
*   DESCRIPTION
*
*       Return a pointer to first occurrence of c in s or 0 if s does not
*       contain c.  If c==0 it returns a pointer to the string's NU_NULL
*       character.
*
*   INPUTS
*
*       s                   Memory to search within.
*       c                   Values to search for.
*       n                   Number of bytes to search.
*
*   OUTPUTS
*
*       VOID*               Pointer to first occurrence of c in s or 0 if
*                             s does not contain c.
*
*************************************************************************/

VOID *STR_mem_chr (const VOID *s, INT32 c, INT32 n)
{
    UNICHAR  val = (UNICHAR)  c;
    UNICHAR *ptr = (UNICHAR*) s;
    UNICHAR *end = ptr + n;

    while (ptr < end)
    {
        if (*ptr++ == val)
        {
            return ((VOID*) (ptr-1));
        }
    }

    return (NU_NULL);
}

/*************************************************************************
*
*   FUNCTION
*
*       STR_mem_cmp
*
*   DESCRIPTION
*
*      Copies a section of memory
*
*   INPUTS
*
*       s1                  Memory to compare.
*       s2                  Memory to compare.
*       n                   Number of bytes to compare.
*
*   OUTPUTS
*
*       INT32               0 means s1 == s2;
*                           -1 means s1 < s2;
*                           1 means s1 > s2
*
*************************************************************************/

INT32 STR_mem_cmp (const VOID *s1, const VOID *s2, INT32 n)
{
    UNICHAR *p1 = (UNICHAR*) s1;
    UNICHAR *p2 = (UNICHAR*) s2;
    UNICHAR *end = p1 + n;

    while (p1 < end)
    {
        if (*p1 != *p2)
        {
            if (*p1 < *p2)
            {
                return (-1);
            }
            else
            {
                return (1);
            }
        }
        ++p1;
        ++p2;
    }

    return (0);               /* memory blocks are equal */
}

/***************************************************************************
* FUNCTION
*
*    STR_mem_cpy
*
* DESCRIPTION
*
*    Function STR_mem_cpy is the re-entrant mem copy function.
*
* INPUTS
*
*    VOID *d - Pointer to the destination memory location.
*    VOID *s - Pointer to the source memory location.
*    INT32 n - Number of bytes.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID STR_mem_cpy(VOID *d, VOID *s, INT32 n)
{
    INT32       i;
    UINT8       *dst;
    UINT8       *src;

    if( d > s )
    {
        dst = (UINT8 *)d;
        src = (UINT8 *)s;

        for(i = 0; i < n; i++)
        {
            *dst++ = *src++;
        }
    }
    else
    {
        dst = (UINT8 *)d + (n-1);
        src = (UINT8 *)s + (n-1);

        for(i = 0; i < n; i++)
        {
            *dst-- = *src--;
        }
   }
}

/*************************************************************************
*
*   FUNCTION
*
*       STR_mem_move
*
*   DESCRIPTION
*
*      Moves values in memory.
*
*   INPUTS
*
*       s1                  Destination.
*       s2                  Source.
*       n                   Number of bytes to move.
*
*   OUTPUTS
*
*       VOID*               Pointer to s1.
*
*************************************************************************/

VOID *STR_mem_move (VOID *s1, const VOID *s2, INT32 n)
{
    UNICHAR *dst = (UNICHAR*) s1;
    UNICHAR *src = (UNICHAR*) s2;
    UNICHAR *end;

    if (dst < src)
	{
        for (end=src+n ;src<end ;)
		{
            *dst++ = *src++;
		}
	}
    else
	{
        for (end=src, src+=n, dst+=n; src>end;)
		{
            *--dst = *--src;
		}
	}

    return (s1);
}

/***************************************************************************
* FUNCTION
*
*    STR_mem_set
*
* DESCRIPTION
*
*    Function STR_mem_set is the re-entrant memset function.
*
* INPUTS
*
*    VOID *s   - Pointer to the memory location.
*    INT32 c   - Value to set.
*    INT32 cnt - Number of bytes.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID STR_mem_set( VOID *s, INT32 c, INT32 cnt)
{
    UNICHAR        *p;
    INT32       i;
	NU_SUPERV_USER_VARIABLES
	
    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();
    p = (UNICHAR *)s; 

    for(i = 0; i < cnt; i++)
    {
        *p++ = (UNICHAR)(c);
    }
	NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    STR_str_cat
*
* DESCRIPTION
*
*    Function STR_str_cat is the re-entrant strcat function.
*
* INPUTS
*
*    UNICHAR *strDestination - Pointer to the destination string.
*    UNICHAR *strSource      - Pointer to the source string.
*
* OUTPUTS
*
*    UNICHAR - Returns pointer to result string.
*
***************************************************************************/
UNICHAR *STR_str_cat( UNICHAR *str_destination, const UNICHAR *str_source )
{

    STR_str_cpy(&str_destination[STR_str_len(str_destination)], str_source);

    return (str_destination);
}

/***************************************************************************
* FUNCTION
*
*    STR_str_chr
*
* DESCRIPTION
*
*    Function STR_str_chr is the re-entrant strchr function.
*
* INPUTS
*
*    UNICHAR *d - Pointer to the string.
*    INT32 n    - Number of the character.
*
* OUTPUTS
*
*    UNICHAR - Pointer to the output string.
*
***************************************************************************/
UNICHAR *STR_str_chr( const UNICHAR *string, INT32 c )
{
    UINT8       done = NU_FALSE;
    UNICHAR     *s;
    UNICHAR     *ret_val = NU_NULL;

    s = (UNICHAR *)string;

    while( *s && !done)
    {
        if( *s == (UINT32) c )
        {
            ret_val = s;
            done = NU_TRUE;
        }

        if ( !done )
        {
            s++;
        }
        
    }

    return (ret_val);
}

/***************************************************************************
* FUNCTION
*
*    STR_str_cmp
*
* DESCRIPTION
*
*    Function STR_str_cmp is the re-entrant strcmp function.
*
* INPUTS
*
*    UNICHAR *string1 - Pointer to the first string.
*    UNICHAR *string2 - Pointer to the second string.
*
* OUTPUTS
*
*    UINT32 - Returns 0 if strings match.
*
***************************************************************************/
INT32 STR_str_cmp( const UNICHAR *string1, const UNICHAR *string2 )
{
    UNICHAR     *s1;
    UNICHAR     *s2;

    s1 = (UNICHAR *)string1;
    s2 = (UNICHAR *)string2;

    while( (*s1 == *s2) && (*s1) ) ++s1, ++s2;

    return ((INT32)(UNICHAR)*s1) - ((INT32)(UNICHAR)*s2);
}

/***************************************************************************
* FUNCTION
*
*    STR_str_cpy
*
* DESCRIPTION
*
*    Function STR_str_cpy is the re-entrant string copy function.
*
* INPUTS
*
*    UNICHAR *d - Pointer to the destination string.
*    UNICHAR *s - Pointer to the source string.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID STR_str_cpy(UNICHAR *d, const UNICHAR *s)
{
	
	NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    while( *s )
    {
        *d++ = *s++;
    }
    
    *d = 0;

	NU_USER_MODE();
} 

/***************************************************************************
* FUNCTION
*
*    STR_str_dup
*
* DESCRIPTION
*
*    Function STR_str_dup is the re-entrant strdup function.
*
* INPUTS
*
*    UNICHAR *s - Input ASCII string.
*
* OUTPUTS
*
*    UNICHAR - Output ASCII string.
*
***************************************************************************/
UNICHAR *STR_str_dup(const UNICHAR *s)
{
    UNICHAR     *block;

    block = (UNICHAR *)MEM_calloc(STR_str_len(s)+1,CHAR_SIZE);

    if( block )
    {
        STR_str_cpy(block,s);
    }

    return (block);
}

/***************************************************************************
* FUNCTION
*
*    STR_str_len
*
* DESCRIPTION
*
*    Function STR_str_len is the re-entrant strlen function.
*
* INPUTS
*
*    UNICHAR *string - Pointer to the input string.
*    INT32 n         - Number of the character.
*
* OUTPUTS
*
*    UINT32 - Number of characters in the string.
*
***************************************************************************/
UINT32 STR_str_len( const UNICHAR *string )
{    
    INT32       i = 0;
    UNICHAR     *s;

    s = (UNICHAR *)string;

    while( *s )
    {
        i++;
        s++;
    }

    return (i);
}

/*************************************************************************
*
*   FUNCTION
*
*       STR_strr_chr
*
*   DESCRIPTION
*
*       Return a pointer to last occurrence of c in s, or 0 if s does
*       not contain c.  if c==0 it returns a pointer to the string's
*       null character.
*
*   INPUTS
*
*       s                   String to search within
*       c                   Characters to search for
*
*   OUTPUTS
*
*       CHAR*               Pointer to last occurrence of c in s or 0 if
*                             s does not contain c.
*
*************************************************************************/

UNICHAR *STR_strr_chr(const UNICHAR *s, INT32 c)
{
    const UNICHAR *mrk = (UNICHAR *)0;

    do
    {
        if (*s == (UNICHAR)c)
		{
            mrk = s;
		}
    } while (*s++);

    return ((UNICHAR *)mrk);
}

/***************************************************************************
* FUNCTION
*
*    STR_str_rev
*
* DESCRIPTION
*
*    Function STR_str_rev is the re-entrant strrev function.
*
* INPUTS
*
*    UNICHAR *s - Input ASCII string.
*
* OUTPUTS
*
*    UNICHAR - Output ASCII string.
*
***************************************************************************/
UNICHAR *STR_str_rev(UNICHAR *s)
{
    INT32       x;
    INT32       y;
    INT32       length;
    UNICHAR     hold;
    
    length = STR_str_len(s);
    x = length - 1;
    y = 0;

    while (x >= (length / 2))
    {
		hold = s[y];
		s[y] = s[x];
		s[x] = hold;
		
		y++;
		x--;
    }

    return(s);
}

/***************************************************************************
* FUNCTION
*
*    STR_str_spn
*
* DESCRIPTION
*
*    Function STR_str_spn is the re-entrant strspn function.
*
* INPUTS
*
*    UNICHAR *s1 - Pointer to the first string.
*    UNICHAR *s2 - Pointer to the second string.
*
* OUTPUTS
*
*    UINT32 - Returns n if char not found.
*
***************************************************************************/
UINT32 STR_str_spn( const UNICHAR *s1, const UNICHAR *s2 )
{
    UINT8           done;
    UINT32          n;
    UNICHAR         c;
    const UNICHAR   *p;

    for( n = 0; (c = *s1++) != 0; ++n ) 
    {
        done = NU_FALSE;

        p = s2;
        
        do 
        {
            if( *p == c )
            {
                done = NU_TRUE;
                break;
            }

        } while( *p++ );

        if ( !done )
        {
            break;
        }
    }

    return (n);
}

/***************************************************************************
* FUNCTION
*
*    STR_str_str
*
* DESCRIPTION
*
*    Function STR_str_str is the re-entrant strstr function.
*
* INPUTS
*
*    UNICHAR *s1 - Pointer to the first string.
*    UNICHAR *s2 - Pointer to the second string.
*
* OUTPUTS
*
*    UNICHAR - Returns pointer to resulting string.
*
***************************************************************************/
UNICHAR *STR_str_str(const UNICHAR *s1, const UNICHAR *s2 ) 
{
    UINT8       done = NU_FALSE;
    UINT32      len2;
    UNICHAR     *ret_val = NU_NULL;
    
    len2 = STR_str_len(s2);

    if( !(len2) )
    {
        ret_val = (UNICHAR *) s1;
        done    = NU_TRUE;
    }

    if ( !done )
    {
        for( ; *s1; ++s1 ) 
        {
            if( *s1 == *s2 && STR_strn_cmp( s1, s2, len2 )==0 ) 
            {
                ret_val = (UNICHAR *) s1;
                break;
            }
        }
    }

    return (ret_val); 
}

/***************************************************************************
* FUNCTION
*
*    STR_str_tok
*
* DESCRIPTION
*
*    Function STR_str_tok is the re-entrant strtok function.
*
* INPUTS
*
*    UNICHAR *string1 - Pointer to the first string.
*    UNICHAR *string2 - Pointer to the second string.
*
* OUTPUTS
*
*    UINT32 - Returns 0 if strings match.
*
***************************************************************************/
UNICHAR *STR_str_tok(UNICHAR *s1, const UNICHAR *s2 )
{
    UINT8           done = NU_FALSE;
    UNICHAR         c;
    UNICHAR         *p;
    UNICHAR         *ret_val = NU_NULL;
    const UNICHAR   *s;
    static UNICHAR  *tok;

    if( s1 == 0 )
    {
        s1 = tok;
    }

    s1 += STR_str_spn( s1, s2 );

    if( *s1 == '\0' )
    {
        done = NU_TRUE;
    }

    if ( !done )
    {
        for( p = s1; (c = *p) != 0; ++p) 
        {
            s = s2;
            do 
            {
                if ( *s == c ) 
                {
                    *p = '\0';
                    tok = p + 1;

                    ret_val = s1;
                    done = NU_TRUE;
                }
            } while ( *s++ && !done); 

            if ( done )
            {
                break;
            }
        }

        if ( !done )
        {
            tok = p;
            ret_val = s1;
        }
    }

    return (ret_val);
}

/***************************************************************************
* FUNCTION
*
*    STR_stri_cmp
*
* DESCRIPTION
*
*    Function STR_stri_cmp is the re-entrant STR_stri_cmp function.
*
* INPUTS
*
*    UNICHAR *string1 - Pointer to the first string.
*    UNICHAR *string2 - Pointer to the second string.
*
* OUTPUTS
*
*    UINT32 - Returns 0 if strings match.
*
***************************************************************************/
INT32 STR_stri_cmp(const UNICHAR *string1, const UNICHAR *string2)
{
    UNICHAR     *s1;
    UNICHAR     *s2;
    
    s1 = (UNICHAR *)string1;
    s2 = (UNICHAR *)string2;

    while(((*s1 == *s2) || 
          ((STR_between(*s1,'A','Z') && ((*s1+0x20) == *s2)) || 
           (STR_between(*s2,'A','Z') && (*s1 == (*s2+0x20))))) && 
          (*s1)) ++s1, ++s2;
    
    return ((INT32)(UNICHAR)*s1) - ((INT32)(UNICHAR)*s2);
}

/***************************************************************************
* FUNCTION
*
*    STR_strn_cmp
*
* DESCRIPTION
*
*    Function STR_strn_cmp is the re-entrant strstr function.
*
* INPUTS
*
*    UNICHAR *s1 - Pointer to the first string.
*    UNICHAR *s2 - Pointer to the second string.
*    UINT32 n    - Number of character.
*
* OUTPUTS
*
*    INT32 - Returns number of characters.
*
***************************************************************************/
INT32 STR_strn_cmp(const UNICHAR *s1, const UNICHAR *s2, UINT32 n)
{
    INT32       ret_val = 0;

    while( (n > 0) && (*s1 == *s2) && (*s1) ) ++s1, ++s2, --n;

    if( n != 0 )
    {
        ret_val = ((INT32)(UNICHAR)*s1) - ((INT32)(UNICHAR)*s2);
    }
    
    return (ret_val);
}

/***************************************************************************
* FUNCTION
*
*    STR_strn_cpy
*
* DESCRIPTION
*
*    Function STR_strn_cpy is the re-entrant string-n copy function.
*
* INPUTS
*
*    UNICHAR *d - Pointer to the destination string.
*    UNICHAR *s - Pointer to the source string.
*    INT32 n    - Number of bytes.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID STR_strn_cpy(UNICHAR *d, const UNICHAR *s, INT32 n)
{
    INT32       i;

    for(i = 0; (i < n) && (*s ); i++)
    {
        *d++ = *s++;
    }

    *d = 0;
}

/***************************************************************************
* FUNCTION
*
*    STR_sub_str
*
* DESCRIPTION
*
*    Function STR_sub_str extracts a string from a string.
*
* INPUTS
*
*    UNICHAR *str - Pointer to the string to check.
*    INT32 b      - Index of the character.
*    INT32 len    - Length of resulting string.
*
* OUTPUTS
*
*    UNICHAR - Returns pointer to the resulting string.
*
***************************************************************************/
UNICHAR *STR_sub_str(UNICHAR *str, INT32 b, INT32 len)
{
    INT32       slen;
    UNICHAR     *c; 
    UNICHAR     *dest_str;

    /* allocate and zero memory for the STD_SubString */
    dest_str = (UNICHAR *)MEM_calloc(len, CHAR_SIZE);
    STR_mem_set(dest_str, 0, len * CHAR_SIZE);

    /* get the length of the original string */
    slen = STR_str_len(str);

    /* if it is less than the start index then just put a */
    /* space in dest to denote that it is empty */
    if( b > slen)
    { 
        STR_str_cpy(dest_str,(UNICHAR *)EMPTY_STR);
    }
    else
    {
        /* decrement the start index */
        if( b > 0)
        {
            b--;
        }
        else 
        {
            b = 0;
        }

        /* make a pointer to str + start index */
        c = str + b;

        /* get the sub string starting at str + start index */
        STR_mem_cpy(dest_str, c, len * CHAR_SIZE); 
    }

    return (dest_str);
}
