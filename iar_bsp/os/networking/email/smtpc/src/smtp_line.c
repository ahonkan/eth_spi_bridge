/*************************************************************************
*
*               Copyright 2012 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************/

/*************************************************************************
*
*   FILE
*
*       smtp_line.c
*
*   COMPONENT
*
*       SMTP Client - Line Utility.
*
*   DESCRIPTION
*
*       This file contains the SMTP line utility implementation. A line
*       is identified by it's delimiter that must be passed during
*       initialization of a line object.
*
*   DATA STRUCTURES
*
*       NONE
*
*   FUNCTIONS
*
*       SMTP_Init_Line_Obj
*       SMTP_Delete_Line_Obj
*       SMTP_Get_Line
*       SMTP_Reset_Line_Obj
*       SMTP_Find_Keyword
*
*   DEPENDENCIES
*
*       smtp_client.h
*       smtp_line.h
*
*************************************************************************/
#include "os/networking/email/smtpc/inc/smtp_client.h"
#include "os/networking/email/smtpc/inc/smtp_line.h"

/************************************************************************
*
* FUNCTION
*
*       SMTP_Init_Line_Obj
*
* DESCRIPTION
*
*       This function will initialize SMTP_LINE object. Delimiter must be
*       a permanent link as delimiter will not be stored in line object.
*
* INPUTS
*
*       *line_obj               Line object.
*       *data_ptr               Pointer to data that has to be parsed.
*       *delimiter              Line delimiter.
*       data_len                Data length to avoid wrong memory access.
*       maxline                 Maximum line that can be returned.
*
* OUTPUTS
*
*       NU_SUCCESS              If object was successfully created.
*
*************************************************************************/
STATUS SMTP_Init_Line_Obj(SMTP_LINE *line_obj, CHAR *data_ptr,
        CHAR *delimiter, UINT32 data_len, UINT32 maxline)
{
    STATUS status;

    if ((data_ptr == NU_NULL) || (strlen(data_ptr) == 0))
    {
        status = NU_INVALID_PARM;
    }
    else
    {
        line_obj->smtp_data         = data_ptr;
        line_obj->smtp_last_line    = data_ptr;
        line_obj->smtp_maxline      = maxline;
        line_obj->smtp_delim        = delimiter;
        line_obj->smtp_data_len     = data_len;

        /* Allocate memory to store a line. */
        status = NU_Allocate_Memory(SMTP_Memory_Pool,
                (VOID **)&line_obj->smtp_line,
                line_obj->smtp_maxline,
                NU_SUSPEND);
    }

    return status;
} /* SMTP_Init_Line_Obj */

/************************************************************************
*
* FUNCTION
*
*       SMTP_Delete_Line_Obj
*
* DESCRIPTION
*
*       This function will deallocate memory reserved by a line object.
*
* INPUTS
*
*       *line_obj               Line object.
*
* OUTPUTS
*
*       NONE
*
*************************************************************************/
VOID SMTP_Delete_Line_Obj(SMTP_LINE *line_obj)
{
    if (line_obj != NU_NULL)
    {
        NU_Deallocate_Memory(line_obj->smtp_line);
    }
} /* SMTP_Delete_Line_Obj */

/************************************************************************
*
* FUNCTION
*
*       SMTP_Get_Line
*
* DESCRIPTION
*
*       This function will return the next line in the data. A line can be
*       of zero size, in case when there are two consecutive delimiters.
*
* INPUTS
*
*       *line_obj               Line object.
*
* OUTPUTS
*
*       Pointer to line.
*       NU_NULL                 If there is no next line or line is
*                               to big to return.
*
*************************************************************************/
CHAR *SMTP_Get_Line(SMTP_LINE *line_obj)
{
    CHAR    *ret_ptr = NU_NULL;
    CHAR    *start = line_obj->smtp_last_line;
    CHAR    *end;
    CHAR    *data_end = line_obj->smtp_data +
                line_obj->smtp_data_len;
    UINT8   add_delim = 1;

    /* Search for line delimiter. */
    end = strstr(start, line_obj->smtp_delim);

    if (end == NU_NULL)
    {
        /* This will handle if data don't have last delimiter. */
        end = data_end;
        add_delim = 0;
    }

    /* Handle end of data and overflow. */
    if (start >= data_end)
    {
        end = NU_NULL;
    }

    /* Handle overflow of end pointer. */
    if (end > data_end)
    {
        end = data_end;
        add_delim = 0;
    }

    if (end != NU_NULL)
    {
        if ((UINT32)(end - start) > line_obj->smtp_maxline)
        {
            end = start + line_obj->smtp_maxline;
            add_delim = 0;
        }

        /* Copy data to line buffer. */
        strncpy(line_obj->smtp_line, start, (end - start));
        line_obj->smtp_line[end - start] = '\0';

        /* Skip new line characters. */
        if (add_delim == 1)
        {
            end += strlen(line_obj->smtp_delim);
        }

        line_obj->smtp_last_line = end;
        ret_ptr = line_obj->smtp_line;
    }

    return (ret_ptr);
} /* SMTP_Get_Line */

/************************************************************************
*
* FUNCTION
*
*       SMTP_Reset_Line_Obj
*
* DESCRIPTION
*
*       This function will reset the SMTP_LINE object such that
*       SMTP_Get_Line will return first line.
*
* INPUTS
*
*       *line_obj               Line object.
*
* OUTPUTS
*
*       NONE
*
*************************************************************************/
VOID SMTP_Reset_Line_Obj(SMTP_LINE *line_obj)
{
    if (line_obj->smtp_data)
    {
        line_obj->smtp_last_line = line_obj->smtp_data;
    }
} /* SMTP_Reset_Line_Obj */

/************************************************************************
*
* FUNCTION
*
*       SMTP_Find_Keyword
*
* DESCRIPTION
*
*       This function will search a keyword in the data and the smtp_line
*       will hold the line in which that keyword was found.
*
* INPUTS
*
*       *line_obj               Line object.
*       *keyword                String to find.
*
* OUTPUTS
*
*       NU_SUCCESS              If keyword was found.
*       -1                      If keyword was not found.
*
*************************************************************************/
STATUS SMTP_Find_Keyword(SMTP_LINE *line_obj, CHAR *keyword)
{
    STATUS  status = -1;
    CHAR    *ptr;

    SMTP_Reset_Line_Obj(line_obj);
    ptr = SMTP_Get_Line(line_obj);

    while (ptr != NU_NULL)
    {
        if (strstr(ptr, keyword))
        {
            /* "line_obj->smtp_line" will have the line in which keyword was found. */
            status = NU_SUCCESS;
            break;
        }
        ptr = SMTP_Get_Line(line_obj);
    }

    return status;
} /* SMTP_Find_Keyword */
