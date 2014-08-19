/*
 * Dropbear - a SSH2 server
 * 
 * Copyright (c) 2002,2003 Matt Johnston
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. */

#ifndef _INCLUDES_H_
#define _INCLUDES_H_

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "networking/nu_networking.h"
#include "services/nu_services.h"
#include "storage/nu_storage.h"
#include "nussh_types.h"
#include "config.h"
#include "options.h"
#include "debug.h"

#include <termios.h>

#ifdef BUNDLED_LIBTOM
#include "libtomcrypt/src/headers/tomcrypt.h"
#include "libtommath/tommath.h"
#else
#include <tomcrypt.h>
#include <tommath.h>
#endif

#include "compat.h"
#include "fake-rfc2553.h"

#ifdef SSH_SYS_OS_NUCLEUS
#include "nussh_unimp.h"
#include "nussh_rtl.h"
#include "nussh_fs.h"
#include "nussh_net.h"
#include "nussh_userauth.h"
#ifdef USE_NU_PTY
#include "nussh_shell.h"
#endif /*USE_NU_PTY*/
#endif /*SSH_SYS_OS_NUCLEUS*/

#ifndef HAVE_UINT16_T
#ifndef HAVE_U_INT16_T
typedef unsigned short u_int16_t;
#endif /* HAVE_U_INT16_T */
typedef u_int16_t uint16_t;
#endif /* HAVE_UINT16_T */

#ifndef LOG_AUTHPRIV
#define LOG_AUTHPRIV LOG_AUTH
#endif

/* so we can avoid warnings about unused params (ie in signal handlers etc) */
#ifdef UNUSED 
#elif defined(__GNUC__) 
# define UNUSED(x) UNUSED_ ## x __attribute__((unused)) 
#elif defined(__LCLINT__) 
# define UNUSED(x) /*@unused@*/ x 
#else 
# define UNUSED(x) x 
#endif

#endif /* _INCLUDES_H_ */
