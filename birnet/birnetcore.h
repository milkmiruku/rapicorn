/* Birnet
 * Copyright (C) 2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __BIRNET_CORE_H__
#define __BIRNET_CORE_H__

#include <stdbool.h>
#include <glib.h>
#include <birnet/birnetconfig.h>

BIRNET_EXTERN_C_BEGIN();

/* --- reliable assert --- */
#define BIRNET_RETURN_IF_FAIL(e)	do { if G_LIKELY (e) {} else { g_return_if_fail_warning (G_LOG_DOMAIN, __PRETTY_FUNCTION__, #e); return; } } while (0)
#define BIRNET_RETURN_VAL_IF_FAIL(e,v)	do { if G_LIKELY (e) {} else { g_return_if_fail_warning (G_LOG_DOMAIN, __PRETTY_FUNCTION__, #e); return v; } } while (0)
#define BIRNET_ASSERT_NOT_REACHED()	do { g_assert_warning (G_LOG_DOMAIN, __FILE__, __LINE__, __PRETTY_FUNCTION__, NULL); } while (0)
#define BIRNET_ASSERT(expr)   do { /* never disabled */ \
  if G_LIKELY (expr) {} else                            \
    g_assert_warning (G_LOG_DOMAIN, __FILE__, __LINE__, \
                      __PRETTY_FUNCTION__, #expr);      \
} while (0)
/* convenient aliases */
#ifdef  _BIRNET_SOURCE_EXTENSIONS
#define	RETURN_IF_FAIL		BIRNET_RETURN_IF_FAIL
#define	RETURN_VAL_IF_FAIL	BIRNET_RETURN_VAL_IF_FAIL
#define	ASSERT_NOT_REACHED	BIRNET_ASSERT_NOT_REACHED
#define	ASSERT			BIRNET_ASSERT
#define	LIKELY			G_LIKELY
#define	ISLIKELY		G_LIKELY
#define	UNLIKELY		G_UNLIKELY
#endif /* _BIRNET_SOURCE_EXTENSIONS */

/* --- compile time assertions --- */
#define BIRNET_CPP_PASTE2(a,b)                  a ## b
#define BIRNET_CPP_PASTE(a,b)                   BIRNET_CPP_PASTE2 (a, b)
#define BIRNET_STATIC_ASSERT_NAMED(expr,asname) typedef struct { char asname[(expr) ? 1 : -1]; } BIRNET_CPP_PASTE (Birnet_StaticAssertion_LINE, __LINE__)
#define BIRNET_STATIC_ASSERT(expr)              BIRNET_STATIC_ASSERT_NAMED (expr, compile_time_assertion_failed)

/* --- common type definitions --- */
typedef unsigned int            BirnetUInt;
typedef unsigned char           BirnetUInt8;
typedef unsigned short          BirnetUInt16;
typedef unsigned int            BirnetUInt32;
typedef unsigned long long      BirnetUInt64;
typedef signed char             BirnetInt8;
typedef signed short            BirnetInt16;
typedef signed int              BirnetInt32;
typedef signed long long        BirnetInt64;
BIRNET_STATIC_ASSERT (sizeof (BirnetInt8) == 1);
BIRNET_STATIC_ASSERT (sizeof (BirnetInt16) == 2);
BIRNET_STATIC_ASSERT (sizeof (BirnetInt32) == 4);
BIRNET_STATIC_ASSERT (sizeof (BirnetInt64) == 8);
typedef BirnetUInt32            BirnetUniChar;

/* --- convenient type shorthands --- */
#ifdef  _BIRNET_SOURCE_EXTENSIONS
typedef BirnetUInt		uint;
typedef BirnetUInt8		uint8;
typedef BirnetUInt16		uint16;
typedef BirnetUInt32		uint32;
typedef BirnetUInt64		uint64;
typedef BirnetInt8		int8;
typedef BirnetInt16		int16;
typedef BirnetInt32		int32;
typedef BirnetInt64		int64;
typedef BirnetUniChar		unichar;
#endif /* _BIRNET_SOURCE_EXTENSIONS */

/* --- birnet initialization --- */
void	birnet_init (int	*argcp,
		     char     ***argvp,
		     const char *app_name); /* in birnetutilsxx.cc */


BIRNET_EXTERN_C_END();

#endif /* __BIRNET_CORE_H__ */

/* vim:set ts=8 sts=2 sw=2: */
