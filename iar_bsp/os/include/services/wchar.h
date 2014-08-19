#ifndef WCHAR_H
#define WCHAR_H

/* stddef.h for wchar_t and size_t */
#include <stddef.h>

/* stdio.h for FILE */
#include <stdio.h>

/* stdarg.h for va_list */
#include <stdarg.h>

/* Includes some POSIX wide-character support found in stdlib.h */
#include <stdlib.h>

/* Character set bit masks */
#define _U  01
#define _L  02
#define _N  04
#define _S  010
#define _P  020
#define _C  040
#define _X  0100
#define _B  0200

/* valid values for wctype_t */
#define WC_ALNUM    1
#define WC_ALPHA    2
#define WC_BLANK    3
#define WC_CNTRL    4
#define WC_DIGIT    5
#define WC_GRAPH    6
#define WC_LOWER    7
#define WC_PRINT    8
#define WC_PUNCT    9
#define WC_SPACE    10
#define WC_UPPER    11
#define WC_XDIGIT   12

/* Wide-character support data types */
typedef unsigned short  wint_t;
typedef int wctype_t;
typedef struct mbstate_t_struct
{
    int __count;
    union
    {
            wint_t __wch;
            unsigned char __wchb[4];
    } __value;

} mbstate_t;

/* Wide-character EOF value */
#define WEOF            ((wint_t)-1)

#ifdef __cplusplus
extern "C" {
#endif /*  __cplusplus  */

/* Wide-character API */
wint_t              btowc(int c);
wint_t              fgetwc(FILE * stream);
wchar_t *           fgetws(wchar_t * ws, int n, FILE * stream);
wint_t              fputwc(wchar_t wc, FILE * stream);
int                 fputws(const wchar_t * ws, FILE * stream);
int                 fwide(FILE * stream, int mode);
int                 fwprintf(FILE * stream, const wchar_t * format, ...);
int                 fwscanf(FILE * stream, const wchar_t * format, ...);
wint_t              getwc(FILE * stream);
wint_t              getwchar(void);
int                 iswalnum(wint_t wc);
int                 iswalpha(wint_t wc);
int                 iswcntrl(wint_t wc);
int                 iswctype(wint_t wc, wctype_t charclass);
int                 iswdigit(wint_t wc);
int                 iswgraph(wint_t wc);
int                 iswlower(wint_t wc);
int                 iswprint(wint_t wc);
int                 iswpunct(wint_t wc);
int                 iswspace(wint_t wc);
int                 iswupper(wint_t wc);
int                 iswxdigit(wint_t wc);
size_t              mbrlen(const char * s, size_t n, mbstate_t * ps);
size_t              mbrtowc(wchar_t * pwc, const char * s, size_t n, mbstate_t * ps);
int                 mbsinit(const mbstate_t * ps);
size_t              mbsrtowcs(wchar_t * dst, const char ** src, size_t len, mbstate_t * ps);
wint_t              putwc(wchar_t wc, FILE * stream);
wint_t              putwchar(wchar_t wc);
int                 swprintf(wchar_t * ws, size_t n, const wchar_t * format, ...);
int                 swscanf(const wchar_t * ws, const wchar_t * format, ...);
wint_t              towlower(wint_t wc);
wint_t              towupper(wint_t wc);
wint_t              ungetwc(wint_t wc, FILE * stream);
int                 vfwprintf(FILE * stream, const wchar_t * format, va_list arg);
int                 vfwscanf(FILE *stream, const wchar_t * format, va_list arg);
int                 vwprintf(const wchar_t * format, va_list arg);
int                 vswprintf(wchar_t * ws, size_t n, const wchar_t * format, va_list arg);
int                 vswscanf(const wchar_t * ws, const wchar_t * format, va_list arg);
int                 vwscanf(const wchar_t * format, va_list arg);
size_t              wcrtomb(char * s, wchar_t wc, mbstate_t * ps);
wchar_t *           wcscat(wchar_t * ws1, const wchar_t * ws2);
wchar_t *           wcschr(const wchar_t * ws, wchar_t wc);
int                 wcscmp(const wchar_t * ws1, const wchar_t * ws2);
int                 wcscoll(const wchar_t * ws1, const wchar_t * ws2);
wchar_t *           wcscpy(wchar_t * ws1, const wchar_t *ws2);
size_t              wcscspn(const wchar_t * ws1, const wchar_t * ws2);
size_t              wcsftime(wchar_t * wcs, size_t maxsize, const wchar_t * format, const struct tm * timeptr);
size_t              wcslen(const wchar_t * ws);
wchar_t *           wcsncat(wchar_t * ws1, const wchar_t * ws2, size_t n);
int                 wcsncmp(const wchar_t * ws1, const wchar_t * ws2, size_t n);
wchar_t *           wcsncpy(wchar_t * ws1, const wchar_t * ws2, size_t n);
wchar_t *           wcspbrk(const wchar_t * ws1, const wchar_t * ws2);
wchar_t *           wcsrchr(const wchar_t * ws, wchar_t wc);
size_t              wcsrtombs(char * dst, const wchar_t ** src, size_t len, mbstate_t * ps);
size_t              wcsspn(const wchar_t * ws1, const wchar_t * ws2);
wchar_t *           wcsstr(const wchar_t * ws1, const wchar_t * ws2);
double              wcstod(const wchar_t * nptr, wchar_t ** endptr);
float               wcstof(const wchar_t * nptr, wchar_t ** endptr);
wchar_t *           wcstok(wchar_t * ws1, const wchar_t * ws2, wchar_t ** ptr);
long                wcstol(const wchar_t * nptr, wchar_t ** endptr, int base);
long double         wcstold(const wchar_t * nptr, wchar_t ** endptr);
long long           wcstoll(const wchar_t * nptr, wchar_t ** endptr, int base);
unsigned long       wcstoul(const wchar_t * nptr, wchar_t ** endptr, int base);
unsigned long long  wcstoull(const wchar_t * nptr, wchar_t ** endptr, int base);
size_t              wcsxfrm(wchar_t * ws1, const wchar_t * ws2, size_t n);
int                 wctob (wint_t wc);
wctype_t            wctype(const char * property);
int                 wcwidth(wchar_t wc);
wchar_t *           wmemchr(const wchar_t * ws, wchar_t wc, size_t n);
int                 wmemcmp(const wchar_t * ws1, const wchar_t * ws2, size_t n);
wchar_t *           wmemcpy(wchar_t * ws1, const wchar_t * ws2, size_t n);
wchar_t *           wmemmove(wchar_t * ws1, const wchar_t * ws2, size_t n);
wchar_t *           wmemset(wchar_t * ws, wchar_t wc, size_t n);
int                 wprintf(const wchar_t * format, ...);
int                 wscanf(const wchar_t * format, ...);

#ifdef __cplusplus
}
#endif  /*  __cplusplus  */

#endif /* WCHAR_H */
