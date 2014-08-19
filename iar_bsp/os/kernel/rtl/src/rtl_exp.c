/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2013
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/
/*************************************************************************
*
*   FILE NAME
*
*       rtl_exp.c
*
*   COMPONENT
*
*       Run-Time Library (RTL)
*
*   DESCRIPTION
*
*       Export symbols for RTL component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       None
*
*************************************************************************/

#include "nucleus.h"

#if (defined(CFG_NU_OS_KERN_PROCESS_CORE_ENABLE) && (CFG_NU_OS_KERN_RTL_EXPORT_SYMBOLS == NU_TRUE))

#include "kernel/proc_extern.h"

/* Define component name for these symbols */
NU_SYMBOL_COMPONENT(NU_OS_KERN_RTL);

#include <ctype.h>
NU_EXPORT_SYMBOL (isalnum);
NU_EXPORT_SYMBOL (ispunct);
NU_EXPORT_SYMBOL (isalpha);
NU_EXPORT_SYMBOL (isupper);
NU_EXPORT_SYMBOL (iscntrl);
NU_EXPORT_SYMBOL (isxdigit);
NU_EXPORT_SYMBOL (isdigit);
NU_EXPORT_SYMBOL (isprint);
NU_EXPORT_SYMBOL (isgraph);
NU_EXPORT_SYMBOL (isspace);
NU_EXPORT_SYMBOL (islower);

#include <math.h>
NU_EXPORT_SYMBOL (acos);
NU_EXPORT_SYMBOL (sqrt);
NU_EXPORT_SYMBOL (asin);
NU_EXPORT_SYMBOL (ceil);
NU_EXPORT_SYMBOL (atan);
NU_EXPORT_SYMBOL (floor);
NU_EXPORT_SYMBOL (cos);
NU_EXPORT_SYMBOL (fmod);
NU_EXPORT_SYMBOL (sin);
NU_EXPORT_SYMBOL (atan2);
NU_EXPORT_SYMBOL (tan);
NU_EXPORT_SYMBOL (cosh);
NU_EXPORT_SYMBOL (exp);
NU_EXPORT_SYMBOL (sinh);
NU_EXPORT_SYMBOL (log);
NU_EXPORT_SYMBOL (tanh);
NU_EXPORT_SYMBOL (log10);
NU_EXPORT_SYMBOL (frexp);
NU_EXPORT_SYMBOL (fabs);
NU_EXPORT_SYMBOL (ldexp);
NU_EXPORT_SYMBOL (pow);
NU_EXPORT_SYMBOL (modf);

#include <stdio.h>
NU_EXPORT_SYMBOL (getc);
NU_EXPORT_SYMBOL (putc);
NU_EXPORT_SYMBOL (getchar);
NU_EXPORT_SYMBOL (putchar);
NU_EXPORT_SYMBOL (gets);
NU_EXPORT_SYMBOL (scanf);
NU_EXPORT_SYMBOL (sscanf);
NU_EXPORT_SYMBOL (printf);
NU_EXPORT_SYMBOL (puts);
NU_EXPORT_SYMBOL (vprintf);
NU_EXPORT_SYMBOL (sprintf);
NU_EXPORT_SYMBOL (snprintf);
NU_EXPORT_SYMBOL (vsprintf);
NU_EXPORT_SYMBOL (perror);
NU_EXPORT_SYMBOL (vsnprintf);
NU_EXPORT_SYMBOL (fputc);
NU_EXPORT_SYMBOL (fputs);
NU_EXPORT_SYMBOL (fwrite);

#include <stdlib.h>
NU_EXPORT_SYMBOL (atof);
NU_EXPORT_SYMBOL (malloc);
NU_EXPORT_SYMBOL (atoi);
NU_EXPORT_SYMBOL (realloc);
NU_EXPORT_SYMBOL (atol);
NU_EXPORT_SYMBOL (abs);
NU_EXPORT_SYMBOL (strtod);
NU_EXPORT_SYMBOL (labs);
NU_EXPORT_SYMBOL (strtol);
NU_EXPORT_SYMBOL (mblen);
NU_EXPORT_SYMBOL (strtoul);
NU_EXPORT_SYMBOL (mbtowc);
NU_EXPORT_SYMBOL (rand);
NU_EXPORT_SYMBOL (wctomb);
NU_EXPORT_SYMBOL (srand);
NU_EXPORT_SYMBOL (mbstowcs);
NU_EXPORT_SYMBOL (calloc);
NU_EXPORT_SYMBOL (wcstombs);
NU_EXPORT_SYMBOL (free);
NU_EXPORT_SYMBOL (qsort);
NU_EXPORT_SYMBOL (atexit);

#include <string.h>
NU_EXPORT_SYMBOL (memcpy);
NU_EXPORT_SYMBOL (strcoll);
NU_EXPORT_SYMBOL (memmove);
NU_EXPORT_SYMBOL (strcpy);
NU_EXPORT_SYMBOL (memchr);
NU_EXPORT_SYMBOL (strncpy);
NU_EXPORT_SYMBOL (memcmp);
NU_EXPORT_SYMBOL (strerror);
NU_EXPORT_SYMBOL (memset);
NU_EXPORT_SYMBOL (strlen);
NU_EXPORT_SYMBOL (strcat);
NU_EXPORT_SYMBOL (strspn);
NU_EXPORT_SYMBOL (strncat);
NU_EXPORT_SYMBOL (strcspn);
NU_EXPORT_SYMBOL (strchr);
NU_EXPORT_SYMBOL (strpbrk);
NU_EXPORT_SYMBOL (strrchr);
NU_EXPORT_SYMBOL (strstr);
NU_EXPORT_SYMBOL (strcmp);
NU_EXPORT_SYMBOL (strncmp);
NU_EXPORT_SYMBOL (strtok);
NU_EXPORT_SYMBOL (strxfrm);
NU_EXPORT_SYMBOL (stpcpy);

#include <time.h>
NU_EXPORT_SYMBOL (asctime);
NU_EXPORT_SYMBOL (asctime_r);
NU_EXPORT_SYMBOL (ctime);
NU_EXPORT_SYMBOL (ctime_r);
NU_EXPORT_SYMBOL (difftime);
NU_EXPORT_SYMBOL (gmtime);
NU_EXPORT_SYMBOL (gmtime_r);
NU_EXPORT_SYMBOL (localtime);
NU_EXPORT_SYMBOL (localtime_r);
NU_EXPORT_SYMBOL (mktime);
NU_EXPORT_SYMBOL (strftime);
NU_EXPORT_SYMBOL (strptime);
NU_EXPORT_SYMBOL (time);
NU_EXPORT_SYMBOL (tzset);

#include <wchar.h>
NU_EXPORT_SYMBOL (wcscmp);
NU_EXPORT_SYMBOL (wcsncpy);
NU_EXPORT_SYMBOL (wcstok);
NU_EXPORT_SYMBOL (wmemset);
NU_EXPORT_SYMBOL (wcslen);
NU_EXPORT_SYMBOL (wcschr);
NU_EXPORT_SYMBOL (wcscpy);
NU_EXPORT_SYMBOL (wcstoul);
NU_EXPORT_SYMBOL (wcstoull);

#endif /* CFG_NU_OS_KERN_RTL_EXPORT_SYMBOLS == NU_TRUE */
