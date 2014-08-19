/*************************************************************************
*
*              Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/**************************************************************************
*
*   FILENAME
*
*       evp_ui_callbacks.c
*
*   DESCRIPTION
*
*       This file implements EVP calls to some UI functions.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       UI_add_input_string
*       UI_add_verify_string
*       UI_free
*       UI_new
*       UI_process
*
*   DEPENDENCIES
*
*       ui.h
*
****************************************************************************/

#include <string.h>
#include "openssl/ui.h"

#ifdef OPENSSL_SYS_NUCLEUS

static int ref_cnt = 0;

int UI_add_input_string(UI *ui, const char *prompt, int flags,
                        char *result_buf, int minsize, int maxsize)
{
    strcpy(result_buf, "whatever");
    return 0;
}

int UI_add_verify_string(UI *ui, const char *prompt, int flags,
                         char *result_buf, int minsize, int maxsize,
                         const char *test_buf)
{
    return -1;
}

void UI_free(UI *ui)
{
    ref_cnt--;
}

UI* UI_new(void)
{
    ref_cnt++;
    return ((UI*)ref_cnt);
}

int UI_process(UI *ui)
{
    return 0;
}

#endif /* OPENSSL_SYS_NUCLEUS */
