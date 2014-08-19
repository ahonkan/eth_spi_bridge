/* memory.c 
 *
 * Copyright (C) 2006-2013 wolfSSL Inc.  All rights reserved.
 *
 * This file is part of CyaSSL.
 *
 * Contact licensing@yassl.com with any questions or comments.
 *
 * http://www.yassl.com
 */


#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

/* submitted by eof */


#include <cyassl/ctaocrypt/settings.h>

#ifdef USE_CYASSL_MEMORY

#include <cyassl/ctaocrypt/memory.h>
#include <cyassl/ctaocrypt/error.h>


/* Set these to default values initially. */
static CyaSSL_Malloc_cb  malloc_function = 0;
static CyaSSL_Free_cb    free_function = 0;
static CyaSSL_Realloc_cb realloc_function = 0;

int CyaSSL_SetAllocators(CyaSSL_Malloc_cb  mf,
                         CyaSSL_Free_cb    ff,
                         CyaSSL_Realloc_cb rf)
{
    int res = 0;

    if (mf)
        malloc_function = mf;
	else
        res = BAD_FUNC_ARG;

    if (ff)
        free_function = ff;
    else
        res = BAD_FUNC_ARG;

    if (rf)
        realloc_function = rf;
    else
        res = BAD_FUNC_ARG;

    return res;
}


void* CyaSSL_Malloc(size_t size)
{
    void* res = 0;

    if (malloc_function)
        res = malloc_function(size);
    else
        res = malloc(size);

    return res;
}

void CyaSSL_Free(void *ptr)
{
    if (free_function)
        free_function(ptr);
    else
        free(ptr);
}

void* CyaSSL_Realloc(void *ptr, size_t size)
{
    void* res = 0;

    if (realloc_function)
        res = realloc_function(ptr, size);
    else
        res = realloc(ptr, size);

    return res;
}

#endif /* USE_CYASSL_MEMORY */
