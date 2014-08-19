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
*       fileutil.c
*
* COMPONENT
*
*       FAT
*
* DESCRIPTION
*
*       Utility routines for FAT component
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       pc_allspace                         Test if size characters in
*                                            a string are spaces.
*       pc_cppad                            Copy one buffer to another
*                                            and right fill with spaces.
*       pc_isdot                            Test to see if fname is
*                                            exactly '.'.
*       pc_isdotdot                         Test if a filename is
*                                            exactly {'.','.'};
*       NUF_Memfill                          Fill a buffer with a
*                                            character.
*       pc_parsenewname                     Setup the new file name.
*       pc_next_fparse                      Next upperbar file name.
*       pc_fileparse                        Copy the short file name and
*                                            the short file extension.
*       pc_nibbleparse                      Nibble off the left most
*                                            part of a path specifier.
*       pc_parsepath                        Parse a path specifier into
*                                            path : file : ext.
*       pc_patcmp                          Compare strings1 and
*                                            strings2 as upper letter.
*       pc_strcat                           strcat
*       pc_usewdcard                        Check the use of wild cards.
*       pc_use_upperbar                     Check the use of upperbar name.
*       pc_checkpath                        Check the name.
*
*************************************************************************/

#include        "storage/fat_defs.h"


/************************************************************************
* FUNCTION
*
*       pc_allspace
*
* DESCRIPTION
*
*       Test if the first size spaces of string are ' ' characters.
*       Test if size characters in a string are spaces
*
*
* INPUTS
*
*       p                                   String
*       i                                   Size
*
* OUTPUTS
*
*       Return YES if n bytes of p are spaces
*       YES if all spaces.
*
*************************************************************************/
INT pc_allspace(UINT8 *p, INT i)
{
INT ret_val = YES;

    /* Do not attempt using a pointer to 0x0 */
    if(p != NU_NULL)
    {
        while (i--)
        {
            if (*p != ' ')
            {
                ret_val = NO;
                break;
            }
            NUF_NEXT_CHAR(p);
        }
    }

    return(ret_val);
}


/************************************************************************
* FUNCTION
*
*       pc_cppad
*
* DESCRIPTION
*
*       Copy one buffer to another and right fill with spaces
*       Copy up to size characters from "from" to "to". If less than
*       size characters are transferred before reaching \0 fill "to"
*       with ' ' characters until its length reaches size.
*       Note: "to" is NOT Null terminated!
*
*
* INPUTS
*
*       to                                  Copy to data buffer
*       from                                Copy from data buffer
*       size                                Size
*
* OUTPUTS
*
*       None.
*
*************************************************************************/
VOID pc_cppad(UINT8 *to, UINT8 *from, INT size)
{
    CHAR spc = ' ';

    /* Do not attempt using a pointer to 0x0 */
    if((to != NU_NULL) && (from != NU_NULL) && (size >= 0))
    {
        while (size--)
        {
            if (*from)
            {
                NUF_COPYBUFF(to,from,1);
                NUF_NEXT_CHAR(to);
                NUF_NEXT_CHAR(from);
            }
            else
            {
                NUF_COPYBUFF(to,&spc,1);
                NUF_NEXT_CHAR(to);
            }

        }
    }
}


/************************************************************************
* FUNCTION
*
*       pc_isdot
*
* DESCRIPTION
*
*       Test to see if fname is exactly '.' followed by seven spaces and
*       fext is exactly three spaces.
*
*
* INPUTS
*
*       fname                               File name
*       fext                                File extension
*
* OUTPUTS
*
*       Return YES if File is exactly '.'
*       YES if file:ext == '.'
*
*************************************************************************/
INT pc_isdot(UINT8 *fname, UINT8 *fext)
{
INT         stat,stat2,stat3;
STATUS      ret_stat = NU_FALSE;
STATUS      get_char_stat;
CHAR       *char_pos = NU_NULL;

    /* Do not attempt using a pointer to 0x0 */
    if(fname != NU_NULL)
    {
        stat = (*fname == '.');
        if(stat != 0)        
        {
            get_char_stat = NUF_GET_CHAR(&char_pos, (CHAR*)fname, 1);
            if(get_char_stat == NU_SUCCESS)
            {
                /* stat2 = check 7 spaces in name and 3 spaces in ext */
                stat2 = pc_allspace((UINT8*)char_pos, 7) && pc_allspace(fext, 3);
                                
                /* Name == "." and ext = NU_NULL also valid. */
                stat3 = (*char_pos == '\0') && (fext == NU_NULL);
                ret_stat = stat2 | stat3;   
            }
        }
    }
 
    return((INT)ret_stat);
}


/************************************************************************
* FUNCTION
*
*       pc_isdotdot
*
* DESCRIPTION
*
*       Test if a filename is exactly {'.','.'};
*
*
* INPUTS
*
*       fname                               File name
*       fext                                File extension
*
* OUTPUTS
*
*       Return YES if File is exactly '.'
*       YES if file:ext == {'.','.'}
*
*************************************************************************/
INT pc_isdotdot(UINT8 *fname, UINT8 *fext)
{
INT         stat,stat2,stat3;
STATUS      ret_stat = NU_FALSE;
STATUS      get_char_stat;
CHAR        *char_pos = NU_NULL;

    /* Do not attempt using a pointer to 0x0 */
    if(fname != NU_NULL)
    {
        get_char_stat = NUF_GET_CHAR(&char_pos, (CHAR*)fname, 1);
        if(get_char_stat == NU_SUCCESS)
        {
            stat =  (*fname == '.') && (*char_pos == '.');
           
            if(stat != 0) 
            {
                get_char_stat = NUF_GET_CHAR(&char_pos, (CHAR*)fname, 2);
                if(get_char_stat == NU_SUCCESS)
                {
                    /* Check if 6 remaining chars and 3 extensions are spaces. */
                    stat2 = ( pc_allspace((UINT8*)char_pos, 6) && pc_allspace(fext, 3) );
                    
                    /* Check if ".." is followed by a NU_NULL and ext is NU_NULL. */
                    stat3 = (*char_pos == '\0') && (fext == NU_NULL);
                    ret_stat = stat2 | stat3;               
                }
            }
        }
    }
 
    return((INT)ret_stat);
}

/************************************************************************
* FUNCTION
*
*       pc_parsenewname
*
* DESCRIPTION
*
*       If a wild card is used, setup the new file name.
*
*
* INPUTS
*
*       *pfi                                File directory entry
*       **newname                           File name(wild card)
*       **newext                            File extension(wild card)
*       *fname                              File name buffer
*
* OUTPUTS
*
*       Always YES                          Success complete.
*
*************************************************************************/
INT pc_parsenewname(DROBJ *pobj, UINT8 *name, UINT8 *ext,
                    VOID **new_name, VOID **new_ext, UINT8 *fname)
{
LNAMINFO    *linfo;
DOSINODE    pi;
UINT8       *p;
UINT8       *old;
UINT8       *op_nmend;
UINT8       *op_exend;
UINT8       old_name[MAX_LFN+1];
FINODE      *pfi;
CHAR        char_match;
CHAR        *str_ptr = NU_NULL;


    /* Initialize new name. */
    *new_name = *new_ext = NU_NULL;

    /* get long file name info */
    linfo = &pobj->linfo;

    /* Setup the old file name */
    if (linfo->lnament)             /* Long file name */
    {
        /* Convert directory entry long file name to character long file name. */
        pc_cre_longname((UINT8 *)old_name, linfo);
    }
    else                            /* Short file name */
    {
        /* get file directory entry */
        pfi = (FINODE *)pobj->finode;
        pc_ino2dos(&pi, pfi,pobj->pdrive->dh);

        /* Convert directory entry short file name to character short file name. */
        pc_cre_shortname((UINT8 *)old_name, pi.fname, pi.fext);
    }

    /* Mark the old file name end. */
    p = old_name;
    op_nmend = NU_NULL;
    while (*p)
    {
        char_match = '.';
        if(NUF_IS_EQUAL((CHAR*)p,&char_match))
        {
            (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)p,1);
            if(!NUF_IS_EQUAL(str_ptr,&char_match))
                op_nmend = p;
        }
        NUF_NEXT_CHAR(p);
    }
    op_exend = p;

    if (!op_nmend)
        op_nmend = op_exend;

    /* Setup the new file name pointer */
    *new_name = fname;

    /* Setup the new file name */
    p = name;
    old = old_name;

    while (*p)
    {
        char_match = '*';
        if(NUF_IS_EQUAL((CHAR*)p,&char_match))
        {
            /* More of the new file name. */
            if (old < op_nmend)
            {
                while (old != op_nmend)
                {
                    NUF_COPYBUFF(fname,old,1);
                    NUF_NEXT_CHAR(old);
                    NUF_NEXT_CHAR(fname);
                }
                break;
            }
        }
        else
        {
            char_match = '?';
            if(NUF_IS_EQUAL((CHAR*)p,&char_match))
            {
                NUF_COPYBUFF(fname,old,1);
            }
            else
            {
                NUF_COPYBUFF(fname,p,1);
            }
        }

        NUF_NEXT_CHAR(fname);
        NUF_NEXT_CHAR(old);
        NUF_NEXT_CHAR(p);

        /* File name end ? */
        if ( ext && (p == (ext-1)) )
            break;
    }


    /* Setup the new file extension. */
    if (ext)
    {
        /* file name .ext */
        char_match = '.';
        NUF_COPYBUFF(fname,&char_match,1);
        NUF_NEXT_CHAR(fname);

        p = ext;
        (VOID)NUF_GET_CHAR(((CHAR**)&old),(CHAR*)op_nmend,1);

        /* Setup the new file extension pointer */
        *new_ext = fname;

        while (*p)
        {
            char_match = '*';
            if(NUF_IS_EQUAL((CHAR*)p,&char_match))
            {
                /* More of the new file extension. */
                if (old < op_exend)
                {
                    while (old != op_exend)
                    {
                        NUF_COPYBUFF(fname,old,1);
                        NUF_NEXT_CHAR(old);
                        NUF_NEXT_CHAR(fname);
                    }
                    break;
                }
            }
            else
            {
                char_match = '?';
                if(NUF_IS_EQUAL((CHAR*)p,&char_match))
                {
                    NUF_COPYBUFF(fname,old,1);
                }
                else
                {
                    NUF_COPYBUFF(fname,p,1);
                }
            }
            NUF_NEXT_CHAR(fname);
            NUF_NEXT_CHAR(old);
            NUF_NEXT_CHAR(p);
        }
    }
    char_match = '\0';
    NUF_COPYBUFF(fname,&char_match,1);

    return(YES);
}


/************************************************************************
* FUNCTION
*
*       pc_next_fparse
*
* DESCRIPTION
*
*       Next upperbar file name.
*       Create a filename~XXX.
*
*
* INPUTS
*
*       filename                            Upperbar file name.
*
* OUTPUTS
*
*       YES    Success complete.
*       NO     Can't create new filename.
*
*************************************************************************/
INT pc_next_fparse(UINT8 *filename)
{
UINT8       *pfr;
UINT8       *pupbar;
UINT8       *pnamend;
UINT32      tailno = 0L;
UINT32      temp;
UINT8       nm;
INT16       n = 0;
INT16       num_char;
INT16       i;
STATUS      ret_stat = YES;
CHAR        char_match;
CHAR        second_char_match;
CHAR        *str_ptr = NU_NULL;

    /* Defaults */
    pfr = filename;
    pnamend = pupbar = 0;

    /* Do not attempt using a pointer to 0x0 */
    if(filename != NU_NULL)
    {
        /* Mark the upperbar pointer and filenumber pointer. */
        while (*pfr)
        {
            if( pnamend == 0 )
            {
                char_match = '\0';
                second_char_match = ' ';
                (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)pfr,1);

                if(NUF_IS_EQUAL(str_ptr,&char_match) ||
                    NUF_IS_EQUAL(str_ptr,&second_char_match))
                    pnamend = pfr;
            }

            char_match = '~';
            if(NUF_IS_EQUAL((CHAR*)pfr,&char_match))
                pupbar = pfr;

            NUF_NEXT_CHAR(pfr);
            /* File name end ? */
            char_match = '.';
            if(NUF_IS_EQUAL((CHAR*)pfr,&char_match))
                break;
        }
    }
    /* Not use the filename upperbar and filenumber. */
    if ( (!pnamend) || (!pupbar)||(!filename) )
        ret_stat = NO;
    else
    {
        /* Take a filename number(~XXX) */
        pfr = pnamend;
        temp = 1L;
        while (pfr != pupbar)
        {
            tailno += (*pfr - '0') * temp;
            temp *= 10L;
            pfr--;
        }
        /* Next a filename number. */
        tailno +=1;

        /* How many single figures */
        temp = tailno;
        while (temp)
        {
            temp /= 10L;
            n++;
        }

        if ((pnamend-n) <= filename)
            ret_stat = NO;
        else
        {

            /* Check to see if the digits after pupbar(the '~') are all 9's. */
            str_ptr = (CHAR*)pupbar;            
            while(str_ptr < (CHAR*)pnamend)
            {
                if(*(str_ptr+1) == 0x39)
                    ++str_ptr;
                else
                {                    
                    str_ptr = NU_NULL;
                    break;
                }
            }
            
            /* If all the digits between pupbar(the '~') and pnamend are 9's, 
               then check to see if we have room after the last digit to add 
               another. So "name~99  " becomes "name~100 ". Otherwise, we
               have to remove a character inorder to add an extra digit. */
            if((str_ptr == (CHAR*)pnamend) && (*(pnamend+1) == 0x20))
            {
                ++pnamend;
            }
        
            /* Set the upperbar mark. */
            str_ptr = (CHAR*)pnamend;
            for(num_char = n; num_char > 0; --num_char)
            {
                NUF_PREV_CHAR(&str_ptr);
            }
            *str_ptr++ = '~';

            /* Set the next number. */
            temp = 1L;
            for (i = 0; i < n; i++)
            {
                nm = (UINT8) (((tailno%(temp*10))/temp) + '0');
                temp *= 10L;

                if (filename < (pnamend-i))
                {
                    *(str_ptr + (n-i-1)) = nm;
                }
                else
                {
                    ret_stat = NO;
                    break;
                }
            }
        }
    }
    if(str_ptr)
    {
        if(n != 1)
        {
            str_ptr += n;
            if (*str_ptr != '\0')
            {
                /* Add any needed blank spaces(0x20) after ~n. */
                while((UINT8)*(str_ptr+1) != '\0')
                {
                    *str_ptr++ = 0x20;
                }
            }
        }
        else
        {
            str_ptr++;
        }
        *str_ptr = '\0';
    }
    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       pc_fileparse
*
* DESCRIPTION
*
*       Copy the short file name and the short file extension to the
*       buffer of an output from the start pointer of the file name and
*       the extension. End each name with '\0'.  It will be
*       completed unsuccessfully if the character that can not be used
*       in the file name is used. When the long file name uses more than
*       eight characters in the file name, uses lower case,
*       uses ' = ' ' [ '' ] ' ' ; ' ' + ' ' , ' '  '  ' . ', or uses
*       more than four characters in the extension, the short file name
*       is created by using the first six characters of the long file
*       name with an upper bar and 1 following it.  The upper function
*       that calls this function has to prepare at least nine short
*       file name buffer and four short file name extension buffer, both
*       in Char.
*
*
*
* INPUTS
*
*       filename                            File name
*       fileext                             File extension
*       pfname                              File name pointer
*       pfext                               File extension pointer
*
* OUTPUTS
*
*       2                                   File name is a long file name
*                                            and uses the upperbar file
*                                            name.
*       1                                   File name is a long file name
*                                            and does not use the upperbar
*                                            file name.
*       0                                   File name is a short file name.
*       NUF_INVNAME                         Path or filename includes
*                                            invalid character.
*
*************************************************************************/
INT pc_fileparse(UINT8 *filename, UINT8 *fileext, VOID *pfname, VOID *pfext)
{
INT16       i;
UINT8       *fname;
UINT8       *fext;
INT         upbar;
INT         upbar_set;
INT         longfile;
CHAR        *str_ptr;
UINT8       num_bytes;


    /* Defaults */
    NUF_Memfill(filename, MAX_SFN, ' ');
    filename[MAX_SFN] = '\0';
    NUF_Memfill(fileext, MAX_EXT, ' ');
    fileext[MAX_EXT] = '\0';

    upbar_set = 0;
    longfile = 0;

    /* Check the file name and file extension. */
    if (!pc_checkpath((UINT8 *)pfname, NO))
        return(NUF_INVNAME);

    if (!pc_checkpath((UINT8 *)pfext, NO))
        return(NUF_INVNAME);

    /* Setup pointer. */
    fname = (UINT8 *)pfname;
    fext = (UINT8 *)pfext;

    /* Special cases of . and .. */
    if (*fname == '.')
    {    
        /* . */
        *filename = '.';
        (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)fname,1);
        if (*str_ptr == '\0')
        {
            return(0); 
        }
        
        /* .. */
        (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)fname,1);
        if (*str_ptr == '.')
        {
            /* Check to make sure all we have is '..' */
            (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)fname,2);
            if (*str_ptr == '\0')
            {
                NUF_NEXT_CHAR(filename);
                *filename = '.';
                return(0); 
            }
        }
    }

    /* Check use of the upperbar.
    Note: filename~xxx */
    upbar = pc_use_upperbar(fname);
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CP_SUPPORT == 0)
    /* Setup the short filename. */
    i = 1;    
#else
    i = 0;
    num_bytes = NUF_GET_CP_LEN((CHAR*)fname);
    i += num_bytes;

#endif
    while (*fname)
    {
        num_bytes = NUF_GET_CP_LEN((CHAR*)fname);
        /* Use upperbar ? */
        if (upbar)
        {
            if ( (*fname != ' ') && (*fname != '.') )
            {
                if ( (*fname == '=') ||
                    (*fname == '[') ||
                    (*fname == ']') ||
                    (*fname == ';') ||
                    (*fname == '+') ||
                    (*fname == ',') )
                {
                    *filename = '_';
                    NUF_NEXT_CHAR(filename);
                }
                else
                {
                    /* If it is an ASCII character. */
                    if(num_bytes == 1)
                    {
                        if ( (*fname >= 'a') && (*fname <= 'z') )
                        {
                            *filename++ = (UINT8) ('A' + *fname - 'a');
                        }
                        else
                            *filename++ = *fname;

                        i++;
                    }
                    /* If our UTF8 character isn't ASCII. */
                    else
                    {
                        /* Copy current fname character to filename. */
                        NUF_NCPBUFF(filename,fname,1);
                        NUF_NEXT_CHAR(filename);
                        (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)fname,1);
                        i += num_bytes;
                    }
                }
            }

            /* Filename end or upper 6 */
            (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)fname,2);
            if ( ((UINT8 *)(str_ptr) == ((UINT8 *)pfext)) || (i > 6) )
            {
                *filename = '~';
                NUF_NEXT_CHAR(filename);
                *filename = '1';
                NUF_NEXT_CHAR(filename);
                *filename = '\0';

#if(CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CP_SUPPORT == 1)
                /* Subtract i's previous value. */
                switch(num_bytes)
                {
                case 1:
                    /* Make sure this won't but us past 6 characters. */
                    i -= 1;
                    break;

                case 2:
                case 3:
                    i -= 2;
                    break;

                case 4:
                    i -= 3;
                    break;

                default:
                    break;
                }
                /* Add 2 for ~1. */
                i += 2;
                /* If our filename isn't 8 bytes yet because the next character would make
                our shortfile name greater than 8 bytes, then pad with 0x20. */
                if( i < 8 && (8 - i)  != 0)
                {
                    do {
                        NUF_NEXT_CHAR(filename);

                    } while(++i != 8);
                    /* Set last character to NULL. */
                    *filename = '\0';
                }
#endif
                upbar_set = 1;      /* upper bar set flag */

                break;
            }
        }
        else
        {
            if ( (*fname >= 'a') && (*fname <= 'z') )
            {
                *filename++ = (UINT8) ('A' + *fname - 'a');
                if (!longfile)
                    longfile = 1;

            }
            else
            {
                /* Check to see if this is a Unicode character,
                any UTF8 encoded character greater than 0x7F is
                is going to be a non ASCII character. This means 
                it has to have LFN. */
                if (!longfile && *fname > 0x7F)
                    longfile = 1;

                NUF_NCPBUFF(filename,fname,1);
                NUF_NEXT_CHAR(filename);
            }

            i++;
            if (i > 8)
                break;
        }
        NUF_NEXT_CHAR(fname);

        /* File name end ? */
        (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)fname,1);
        if(((UINT8*)(str_ptr)) == ((UINT8 *)pfext))
            break;
    }

    /* Setup the short fileext. */
    if (fext)
    {
        i = 0;
        while (*fext)
        {
            if ( (*fext != ' ') && (*fext != '.') )
            {
                if (i++ < 3)
                {
                    if ( (*fext == '=') ||
                        (*fext == '[') ||
                        (*fext == ']') ||
                        (*fext == ';') ||
                        (*fext == '+') ||
                        (*fext == ',') )
                    {
                        *fileext = '_';
                        NUF_NEXT_CHAR(fileext);
                    }
                    else if ( (*fext >= 'a') && (*fext <= 'z') )
                    {
                        *fileext++ = (UINT8) ('A' + *fext - 'a');
                        if (!longfile)
                            longfile = 1;
                    }
                    else
                    {
                        /* Check to see if this is a Unicode character,
                        any UTF8 encoded character greater than 0x7F is
                        is going to be a non ASCII character. This means 
                        it has to have LFN. */
                        if (!longfile && *fext > 0x7F)
                            longfile = 1;

                        NUF_NCPBUFF(fileext,fext,1);
                        NUF_NEXT_CHAR(fileext);
                    }
                }
            }
            NUF_NEXT_CHAR(fext);
        }
    }
    else
    {
        if ( (upbar) && (!upbar_set) )
        {
            /* No fileextension */
            *filename = '~';
            NUF_NEXT_CHAR(filename);
            *filename = '1';
        }
    }

    if (upbar)
        return(2);

    return(longfile);
}
/************************************************************************
* FUNCTION
*
*       pc_nibbleparse
*
* DESCRIPTION
*
*       Nibble off the left most part of a path specifier
*       Take a path specifier (no leading D:). and parse the left most
*       element into filename and file ext. (SPACE right filled.).
*
*       Parse a path. Return NULL if problems or a pointer to the ""
*       If input path is NULL, return NULL and set NO to stat.
*
*
* INPUTS
*
*       filename                            File name
*       fileext                             File extension
*       path                                Path name
*
* OUTPUTS
*
*       Returns a pointer to the rest of the path specifier beyond
*       file.ext
*
*************************************************************************/
UINT8 *pc_nibbleparse(UINT8 *topath)
{
UINT8       *p;
UINT8       *ppend;


    /* Defaults. */
    ppend = 0;
    p = topath;

    /* Do not attempt using a pointer to 0x0 */
    if(topath != NU_NULL)
    {
        /* Mark the next backslash */
        while (*p)
        {
            if (*p == BACKSLASH)
            {
                ppend = p;
                break;
            }
            NUF_NEXT_CHAR(p);
        }
    }

    /* Check the path end */
    if (!ppend)
        ppend = NU_NULL;
    else /* Return next path */
        ppend += 1;

    return(ppend);
}


/************************************************************************
* FUNCTION
*
*       pc_parsepath
*
* DESCRIPTION
*
*       Parse a path specifier into path,file,ext
*       Take a path specifier in path and break it into three null
*       terminated strings "topath", "pfname", and "pfext".
*
*
* INPUTS
*
*       topath                              Path name pointer.
*       pfname                              File name pointer
*       pfext                               File extension pointer
*       path                                Path name
*
* OUTPUTS
*
*       NU_SUCCESS                          If service is successful.
*       NUF_BADPARM                         Path is NULL.
*       NUF_LONGPATH                        Path or filename too long.
*       NUF_INVNAME                         Path or filename includes
*                                            invalid character.
*
*
*************************************************************************/
STATUS pc_parsepath(VOID **topath, VOID **pfname, VOID **pfext, UINT8 *path)
{
UINT8       *pfr;
UINT8       *pslash;
UINT8       *pperiod;
UINT8       *temp;
INT         colon;
UINT16      length;
CHAR        *str_ptr;
CHAR        *filename_ptr;
UINT8       leading_dot;
    
    /* Assume leading character of the filename isn't a period. */
    leading_dot = 1;
    
    /* Path is NULL */
    if( ! path )
        return  NUF_BADPARM;

    /* Move path pointer to local */
    pfr = path;

    /* Initialize local variable */
    pslash = pperiod = NU_NULL;
    *topath = *pfname = *pfext = NU_NULL;

    (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)path,1);
    /* Check is there colon? */
    if ( path[0] && *str_ptr == ':' )
    {
        colon = 1;
    }
    else
    {
        colon = 0;
    }

    /* Parse a path
    Mark the last backslash and last period */
    length = 1;

    /* Get last slash position and last period position and check length. */
    while (*pfr)
    {
        /* Check to see if the leading character is a period. */
        if(leading_dot != 0 && *pfr != '.')
        {
            /* Set this to false because we have encounter another character,
            which means we are no longer traversing the leading dot part of
            the name. */
            leading_dot = 0;
        }
                
        /* Is current pointer is slash ? */
        if (*pfr == '\\')
        {
            pslash = pfr;
            /* Reset our leading dot assumption*/
            leading_dot = 1;
        }
       
        /* Is current pointer is period ? */
        if (*pfr == '.')
        {
            
            /* Note: Not mark the next name period, backslash and NULL  */
            (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)pfr,1);
            if ( (*str_ptr != '.') && (*str_ptr != '\\') && (*str_ptr != '\0') && (leading_dot == 0) )
                pperiod = pfr;
            
        }              
               
        length++;
        NUF_NEXT_CHAR(pfr);
        /* Check length */
        if (length > EMAXPATH)
            return(NUF_LONGPATH);

    }


    /**** Setup the path pointer ****/
    if (pslash)  /* Is the path include slash ? */
    {
        /* Clear period if in middle of path */
        if(pslash > pperiod )
            pperiod = NU_NULL;

        /* Yes, we assume pathname is specified. */
        if (colon)
        {
            (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)path,2);
            *topath = str_ptr;
        }
        else
        {
            *topath = path;
        }
    }
    else
    {
        /* No slash */
        if (colon)
        {
            (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)path,2);


            *pfname = str_ptr;
        }
        else
        {
            *pfname = path;
        }

        /* Special cases of "." or  ".." */
        (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)*pfname,1);
        (VOID)NUF_GET_CHAR(&filename_ptr,(CHAR*)*pfname,2);

        if( ( ((**(UINT8 **)pfname == '.') && (*(str_ptr) == '\0')) ||
              ((**(UINT8 **)pfname == '.') && (*(str_ptr) == '.'))) && 
            (*(filename_ptr) == '\0') )
        {
            *topath = path;
            return(NU_SUCCESS);
        }
        else
            *topath = NU_NULL;
    }

    
    /**** Setup the filename pointer ****/
    if (pslash) /* The path include slash.*/
    {
        (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)pslash,1);

        if ( *(str_ptr) != '\0' )
            *pfname = str_ptr;
        else
            *pfname = NU_NULL;
    }
    else  /*  No slash */
    {
        if (colon)  /* C:filename...... */
        {
            (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)path,2);

            if ( *(str_ptr) != '\0' )
                *pfname = str_ptr;
            else
                *pfname = NU_NULL;
        }
        else    /* Only filename */
        {
            *pfname = path;
        }
    }
   
    /* Check the filename */
    if (*pfname)
    {
        temp = (UINT8 *)(*pfname);

        /* Special cases of .\ or ..\ or . or ..*/
        if (*temp == '.')
        {
            (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)temp,1);

            if (*(str_ptr) != '.')            
            {
                (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)temp,2);
                if (*(str_ptr) == '\\' )     /* .\ */
                {
                    (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)temp,3);

                    if (*(str_ptr) != '\0')
                        return(NUF_INVNAME);
                }            
            }
        }
        else
        {
            /* Nothing file extension
            Delete filename period. filename..... */
            if (!pperiod)
            {
                while (*temp)
                {
                    /* Special cases of "*." */
                    (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)temp,1);

                    if ( (*temp == '*') && (*(str_ptr) == '.') )
                    {
                        /* skip */
                        NUF_NEXT_CHAR(temp);
                    }
                    else if (*temp == '.')
                    {
                        /* filename...... -> filename */
                        (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)temp,1);

                        NUF_NEXT_CHAR(temp);
                        if ( (*(str_ptr) == '\0') || (*(str_ptr) == '.') )
                        {
                            *temp = '\0';
                            break;
                        }
                    }

                    NUF_NEXT_CHAR(temp);
                }
            }
        }
    }

    /* Check the path name */
    temp = (UINT8 *)(*topath);
    length = 0;
    if (*topath)
    {
        while (*temp)
        {

            switch (*temp)
            {
            case '.': /* Special cases of .\ or ..\ */
                {
                    (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)temp,1);

                    if (*str_ptr == '.')
                    {
                        (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)temp,2);

                        if (*str_ptr == '\\')
                        {
                            length += 1;
                            NUF_NEXT_CHAR(temp);

                        }                    
                    }
                    else
                    {
                        (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)temp,1);

                        if (*str_ptr == '\\')
                        {
                            break;
                        }
                    }
                    break;
                }
            case '\\': /* \..\ or \.\ or \. or \..  */
                {
                    (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)temp,1);
                    if (*str_ptr == '\\')
                    {
                        return(NUF_INVNAME);
                    }
                    break;
                }
            default:
                {
                    break;
                }
            }/* End switch */

            if (temp == pslash)
            {
                break;
            }

            length++;
            NUF_NEXT_CHAR(temp);

        } /* End While */
    }

    /**** Setup the file extension pointer ****/
    if ((*pfname) && pperiod)
    {
        /* Check the file extension
        Delete extension period.
        xxxx.extension...... -> extension */
        temp = pperiod + 1;
        while (*temp)
        {
            if (*temp == '.')
            {
                (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)temp,1);
                if ( (*str_ptr == '\0') || (*str_ptr == '.') )
                {
                    *temp = '\0';
                }
            }
            NUF_NEXT_CHAR(temp);
        }

        (VOID)NUF_GET_CHAR((CHAR**)pfext,(CHAR*)pperiod,1);
    }
    else
    {
        *pfext = NU_NULL;
    }

    return(NU_SUCCESS);
}

/************************************************************************
* FUNCTION
*
*       pc_patcmp
*
* DESCRIPTION
*
*       Compare "disk_fnam" and "in_fnam" in uppercase letter. Wild card is
*       available.
*
*
*
* INPUTS
*
*       *disk_fnam                          Pointer to filename in disk
*       *in_fnam                            Pointer to filename which
*                                            application specified.
*
* OUTPUTS
*
*       YES                                 compare match.
*       NO                                  Not match.
*
*
*************************************************************************/
INT pc_patcmp(UINT8 *disk_fnam, UINT8 *in_fnam )
{
UINT8       *c1;
UINT8       *c2;
INT         pd;
CHAR        *str_ptr;
CHAR        *filename_ptr;
UINT8       ascii_char1 = 0;
UINT8       ascii_char2 = 0;

    if ((in_fnam == NU_NULL) || (disk_fnam == NU_NULL))
        return(NO);


    for (;;)
    {
        if (! *disk_fnam) /* End of the filename */
        {
            if ( (! *in_fnam ) || (*in_fnam == BACKSLASH) )
                return(YES);
            else
                return(NO);
        }
        if (*in_fnam == '*')    /* '*' matches the rest of the name */
        {
            NUF_NEXT_CHAR(in_fnam);
            if (! *in_fnam)
                return(YES);

            pd = 0;
            /* Compare after wild card. */
            for (; *disk_fnam;)
            {
                if (*disk_fnam == '.')
                    pd = 1;

                if (YES == pc_patcmp(disk_fnam, in_fnam))
                    return(YES);
                NUF_NEXT_CHAR(disk_fnam);
            }
            (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)in_fnam,1);
            (VOID)NUF_GET_CHAR(&filename_ptr,(CHAR*)in_fnam,2);
            if ( (*in_fnam == '.') && (*(str_ptr) == '*') && (*(filename_ptr)=='\0') )
                return(YES);

            if ( (!pd) && (*in_fnam == '.') && (*(str_ptr)=='\0') )
                return(YES);

            return(NO);
        }
        if (*disk_fnam == '*')    /* '*' matches the rest of the name */
        {
            NUF_NEXT_CHAR(disk_fnam);
            if (! *disk_fnam)
                return(YES);

            for (; *in_fnam; ) /* Compare after wild card */
            {
                if (YES == pc_patcmp(in_fnam, disk_fnam) )
                    return(YES);
                NUF_NEXT_CHAR(in_fnam);
            }
            (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)disk_fnam,1);
            (VOID)NUF_GET_CHAR(&filename_ptr,(CHAR*)disk_fnam,2);
            if ( (*disk_fnam == '.') && (*(str_ptr) == '*') && (*(filename_ptr) =='\0') )
                return(YES);
            return(NO);
        }
        /* Convert to upper letter */
        if ( (*disk_fnam >= 'a') && (*disk_fnam <= 'z') )
        {
            ascii_char1 = (UINT8) ('A' + *disk_fnam - 'a');
            c1 = &ascii_char1;
        }
        else
            c1 = disk_fnam;

        /* Convert to upper letter */
        if ( (*in_fnam >= 'a') && (*in_fnam <= 'z') )
        {
            ascii_char2 = (UINT8) ('A' + *in_fnam - 'a');
            c2 = &ascii_char2;
        }
        else
            c2 = in_fnam;


        if(!NUF_IS_EQUAL((CHAR*)c1,(CHAR*)c2))
            if (*c2 != '?')  /* ? is wild card */
                return(NO);

        NUF_NEXT_CHAR(disk_fnam);
        NUF_NEXT_CHAR(in_fnam);

    }

}


/************************************************************************
* FUNCTION
*
*       pc_strcat
*
* DESCRIPTION
*
*       strcat
*
*
* INPUTS
*
*       to                          To data buffer
*       from                        From data buffer
*
* OUTPUTS
*
*       None.
*
*************************************************************************/
VOID pc_strcat(UINT8 *to, UINT8 *from)
{
    /* Do not attempt using a pointer to 0x0 */
    if((to != NU_NULL) && (from != NU_NULL))
    {
        while (*to)  NUF_NEXT_CHAR(to);
        while (*from)
        {
            NUF_COPYBUFF(to,from,1);
            NUF_NEXT_CHAR(to);
            NUF_NEXT_CHAR(from);
        }
        *to = '\0';
    }
}


/************************************************************************
* FUNCTION
*
*       pc_use_wdcard
*
* DESCRIPTION
*
*       Check the use of wild card.
*
*
* INPUTS
*
*       code                                name
*
* OUTPUTS
*
*       YES                                 Uses the wild card.
*       NO                                  Does not use the wild card.
*
*************************************************************************/
INT pc_use_wdcard(UINT8 *code)
{
INT    ret_val = NO;

    /* Do not attempt using a pointer to 0x0 */
    if(code != NU_NULL)
    {
        while (*code)
        {
            if ( (*code == '*') || (*code == '?') )
            {
                ret_val = YES;
                break;
            }
            NUF_NEXT_CHAR(code);
        }
    }
    return(ret_val);
}


/************************************************************************
* FUNCTION
*
*       pc_use_upperbar
*
* DESCRIPTION
*
*       Check the use of upperbar name.
*       ' '(space), '.'(period), '=', '[', ']', ';', ':', '+', ','
*
*
* INPUTS
*
*       code                        File name or File extension
*
* OUTPUTS
*
*       YES                         Uses the upperbar name.
*       NO                          Does not use the upperbar name.
*
*************************************************************************/
INT pc_use_upperbar(UINT8 *code)
{
UINT16      n, j;
UINT16      period = 0;
STATUS      ret_stat = NO;
CHAR        *str_ptr;

    n = j = 0;

    /* Do not attempt using a pointer to 0x0 */
    if(code != NU_NULL)
    {
        while (*code)
        {
            switch (*code)
            {
            case ' ':
            case '=':
            case '[':
            case ']':
            case ';':
            case '+':
            case ',':
                {
                    ret_stat = YES;
                    break;
                }
            case '.':
                {
                    /* filename.... or extension..... */
                    (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)code,1);
                    if (*str_ptr == '\0')
                    {
                        period = j = 0;
                    }
                    else
                    {
                        period++;
                    }
                    break;
                }
            default:
                break;
            }

            /* Check the length of extension */
            if (period)
            {
                if (j++ > 3)
                    ret_stat = YES;
            }
            /* Check the length of filename */
            else
            {
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CP_SUPPORT == 0)
                n++;
#elif(CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CP_SUPPORT == 1)

               n += NUF_GET_CP_LEN((CHAR*)code);
#endif
                if (n > 8)
                    ret_stat = YES;
            }
            if (ret_stat == YES)
                break;
            NUF_NEXT_CHAR(code);
        }
    }

    /* Another way to differentiate filename and extension ? */
    if (period > 1)
    {
        ret_stat = YES;
    }
    /* Handle case of ".XXX" as filename */
    else if ((period == 1) && (n == 0))
    {
        ret_stat = YES;
    }

    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       pc_checkpath
*
* DESCRIPTION
*
*       Check the path name or Volume label.
*
*
* INPUTS
*
*       code                                Path name or Volume label.
*       vol                                 YES : Volume label
*                                           NO  : Path name
*
* OUTPUTS
*
*       YES                                 Success.
*       NO                                  Path name error.
*
*************************************************************************/
INT pc_checkpath(UINT8 *code, INT vol)
{
UINT16      n = 0;
INT         ret_val = YES;
CHAR        *str_ptr;

    if (code)
    {
        while (*code)
        {

            if (!vol)
            {
                /* If a colon is found, it must preceed a slash */
                if (*code == ':')
                {
                    (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)code,1);
                    if ( (n != 1) || (*str_ptr != '\\') )
                    {
                        ret_val = NO;
                        break;
                    }
                }

                /* Only single slashes are allowed */
                if (*code == '\\')
                {
                    (VOID)NUF_GET_CHAR(&str_ptr,(CHAR*)code,1);
                    if (*str_ptr == '\\')
                    {
                        ret_val = NO;
                        break;
                    }
                }
            }
            else
            {
                /* For volumes:
                Cannot contain + . = [ ] \ : , ;  */
                switch (*code)
                {
                case '+':
                case '.':
                case '=':
                case '[':
                case '\\':
                case ']':
                case ':':
                case ',':
                case ';':
                    ret_val = NO;
                    break;

                default:
                    break;
                }
                if (ret_val == NO)
                    break;
            }

            /* Paths and volume names:
            Cannot contain " * / < > ? |    */
            switch (*code)
            {
            case '"':
            case '*':
            case '/':
            case '<':
            case '>':
            case '?':
            case '|':
                ret_val = NO;
                break;

            default:
                break;
            }
            if (ret_val == NO)
                break;

            NUF_NEXT_CHAR(code);
            ++n;
        }
    }
    return(ret_val);
}


