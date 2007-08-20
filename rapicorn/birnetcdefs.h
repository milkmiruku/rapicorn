/* BirnetCDefs - C compatible definitions
 * Copyright (C) 2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#ifndef __BIRNET_CDEFS_H__
#define __BIRNET_CDEFS_H__

#include <rapicorn/rapicornconfig.h>	/* _GNU_SOURCE */
#include <stdbool.h>
#include <stddef.h>			/* NULL */
#include <sys/types.h>			/* uint, ssize */
#include <limits.h>                     /* {INT|CHAR|...}_{MIN|MAX} */
#include <float.h>                      /* {FLT|DBL}_{MIN|MAX|EPSILON} */

RAPICORN_EXTERN_C_BEGIN();

/* --- standard macros --- */
#ifndef FALSE
#  define FALSE					false
#endif
#ifndef TRUE
#  define TRUE					true
#endif
#define RAPICORN_ABS(a)                       	((a) > -(a) ? (a) : -(a))
#define RAPICORN_MIN(a,b)                         ((a) <= (b) ? (a) : (b))
#define RAPICORN_MAX(a,b)                         ((a) >= (b) ? (a) : (b))
#define RAPICORN_CLAMP(v,mi,ma)                   ((v) < (mi) ? (mi) : ((v) > (ma) ? (ma) : (v)))
#define RAPICORN_ARRAY_SIZE(array)		(sizeof (array) / sizeof ((array)[0]))
#undef ABS
#define ABS                                     RAPICORN_ABS
#undef MIN
#define MIN                                     RAPICORN_MIN
#undef MAX
#define MAX                                     RAPICORN_MAX
#undef CLAMP
#define CLAMP                                   RAPICORN_CLAMP
#undef ARRAY_SIZE
#define ARRAY_SIZE				RAPICORN_ARRAY_SIZE
#undef EXTERN_C
#ifdef	__cplusplus
#define EXTERN_C                                extern "C"
#else
#define EXTERN_C                                extern
#endif
#undef STRFUNC
#if defined (__GNUC__)
#  define STRFUNC				((const char*) (__PRETTY_FUNCTION__))
#elif defined (G_HAVE_ISO_VARARGS)
#  define STRFUNC				G_STRFUNC /* GLib present */
#else
#  define STRFUNC				("<unknown>()")
#endif
#if     !defined (INT64_MAX) || !defined (INT64_MIN) || !defined (UINT64_MAX)
#ifdef  LLONG_MAX       /* some gcc versions ship limits.h that fail to define LLONG_MAX for C99 */
#  define INT64_MAX     LLONG_MAX       // +9223372036854775807LL
#  define INT64_MIN     LLONG_MIN       // -9223372036854775808LL
#  define UINT64_MAX    ULLONG_MAX      // +18446744073709551615LLU
#else /* !LLONG_MAX but gcc always has __LONG_LONG_MAX__ */
#  define INT64_MAX     __LONG_LONG_MAX__
#  define INT64_MIN     (-INT64_MAX - 1LL)
#  define UINT64_MAX    (INT64_MAX * 2ULL + 1ULL)
#endif
#endif

/* --- likelyness hinting --- */
#define	RAPICORN__BOOL(expr)		__extension__ ({ bool _birnet__bool; if (expr) _birnet__bool = 1; else _birnet__bool = 0; _birnet__bool; })
#define	RAPICORN_ISLIKELY(expr)		__builtin_expect (RAPICORN__BOOL (expr), 1)
#define	RAPICORN_UNLIKELY(expr)		__builtin_expect (RAPICORN__BOOL (expr), 0)
#define	RAPICORN_LIKELY			RAPICORN_ISLIKELY

/* --- assertions and runtime errors --- */
#define RAPICORN_RETURN_IF_FAIL(e)	do { if (RAPICORN_ISLIKELY (e)) break; RAPICORN__RUNTIME_PROBLEM ('R', RAPICORN_LOG_DOMAIN, __FILE__, __LINE__, RAPICORN_SIMPLE_FUNCTION, "%s", #e); return; } while (0)
#define RAPICORN_RETURN_VAL_IF_FAIL(e,v)	do { if (RAPICORN_ISLIKELY (e)) break; RAPICORN__RUNTIME_PROBLEM ('R', RAPICORN_LOG_DOMAIN, __FILE__, __LINE__, RAPICORN_SIMPLE_FUNCTION, "%s", #e); return v; } while (0)
#define RAPICORN_ASSERT(e)		do { if (RAPICORN_ISLIKELY (e)) break; RAPICORN__RUNTIME_PROBLEM ('A', RAPICORN_LOG_DOMAIN, __FILE__, __LINE__, RAPICORN_SIMPLE_FUNCTION, "%s", #e); while (1) *(void*volatile*)0; } while (0)
#define RAPICORN_ASSERT_NOT_REACHED()	do { RAPICORN__RUNTIME_PROBLEM ('N', RAPICORN_LOG_DOMAIN, __FILE__, __LINE__, RAPICORN_SIMPLE_FUNCTION, NULL); RAPICORN_ABORT_NORETURN(); } while (0)
#define RAPICORN_WARNING(...)		do { RAPICORN__RUNTIME_PROBLEM ('W', RAPICORN_LOG_DOMAIN, __FILE__, __LINE__, RAPICORN_SIMPLE_FUNCTION, __VA_ARGS__); } while (0)
#define RAPICORN_ERROR(...)		do { RAPICORN__RUNTIME_PROBLEM ('E', RAPICORN_LOG_DOMAIN, __FILE__, __LINE__, RAPICORN_SIMPLE_FUNCTION, __VA_ARGS__); RAPICORN_ABORT_NORETURN(); } while (0)
#define RAPICORN_ABORT_NORETURN()		birnet_abort_noreturn()

/* --- convenient aliases --- */
#ifdef  _BIRNET_SOURCE_EXTENSIONS
#define	ISLIKELY		RAPICORN_ISLIKELY
#define	UNLIKELY		RAPICORN_UNLIKELY
#define	LIKELY			RAPICORN_LIKELY
#define	return_if_fail		RAPICORN_RETURN_IF_FAIL
#define	return_val_if_fail	RAPICORN_RETURN_VAL_IF_FAIL
#define	assert_not_reached	RAPICORN_ASSERT_NOT_REACHED
#undef  assert
#define	assert			RAPICORN_ASSERT
#endif /* _BIRNET_SOURCE_EXTENSIONS */

/* --- preprocessor pasting --- */
#define RAPICORN_CPP_PASTE4i(a,b,c,d)             a ## b ## c ## d  /* twofold indirection is required to expand macros like __LINE__ */
#define RAPICORN_CPP_PASTE4(a,b,c,d)              RAPICORN_CPP_PASTE4i (a,b,c,d)
#define RAPICORN_CPP_PASTE3i(a,b,c)               a ## b ## c	  /* twofold indirection is required to expand macros like __LINE__ */
#define RAPICORN_CPP_PASTE3(a,b,c)                RAPICORN_CPP_PASTE3i (a,b,c)
#define RAPICORN_CPP_PASTE2i(a,b)                 a ## b      	  /* twofold indirection is required to expand macros like __LINE__ */
#define RAPICORN_CPP_PASTE2(a,b)                  RAPICORN_CPP_PASTE2i (a,b)
#define RAPICORN_STATIC_ASSERT_NAMED(expr,asname) typedef struct { char asname[(expr) ? 1 : -1]; } RAPICORN_CPP_PASTE2 (Birnet_StaticAssertion_LINE, __LINE__)
#define RAPICORN_STATIC_ASSERT(expr)              RAPICORN_STATIC_ASSERT_NAMED (expr, compile_time_assertion_failed)

/* --- attributes --- */
#if     __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)
#define RAPICORN_PRETTY_FUNCTION                  (__PRETTY_FUNCTION__)
#define RAPICORN_PURE                             __attribute__ ((__pure__))
#define RAPICORN_MALLOC                           __attribute__ ((__malloc__))
#define RAPICORN_PRINTF(format_idx, arg_idx)      __attribute__ ((__format__ (__printf__, format_idx, arg_idx)))
#define RAPICORN_SCANF(format_idx, arg_idx)       __attribute__ ((__format__ (__scanf__, format_idx, arg_idx)))
#define RAPICORN_FORMAT(arg_idx)                  __attribute__ ((__format_arg__ (arg_idx)))
#define RAPICORN_NORETURN                         __attribute__ ((__noreturn__))
#define RAPICORN_CONST                            __attribute__ ((__const__))
#define RAPICORN_UNUSED                           __attribute__ ((__unused__))
#define RAPICORN_NO_INSTRUMENT                    __attribute__ ((__no_instrument_function__))
#define RAPICORN_DEPRECATED                       __attribute__ ((__deprecated__))
#define RAPICORN_ALWAYS_INLINE			__attribute__ ((always_inline))
#define RAPICORN_NEVER_INLINE			__attribute__ ((noinline))
#define RAPICORN_CONSTRUCTOR			__attribute__ ((constructor,used)) /* gcc-3.3 also needs "used" to emit code */
#define RAPICORN_MAY_ALIAS                        __attribute__ ((may_alias))
#else   /* !__GNUC__ */
#define RAPICORN_PRETTY_FUNCTION                  ("<unknown>")
#define RAPICORN_PURE
#define RAPICORN_MALLOC
#define RAPICORN_PRINTF(format_idx, arg_idx)
#define RAPICORN_SCANF(format_idx, arg_idx)
#define RAPICORN_FORMAT(arg_idx)
#define RAPICORN_NORETURN
#define RAPICORN_CONST
#define RAPICORN_UNUSED
#define RAPICORN_NO_INSTRUMENT
#define RAPICORN_DEPRECATED
#define RAPICORN_ALWAYS_INLINE
#define RAPICORN_NEVER_INLINE
#define RAPICORN_CONSTRUCTOR
#define RAPICORN_MAY_ALIAS
#error  Failed to detect a recent GCC version (>= 3.3)
#endif  /* !__GNUC__ */
#ifdef	__cplusplus
#define	RAPICORN_SIMPLE_FUNCTION			(__func__)
#else
#define	RAPICORN_SIMPLE_FUNCTION			RAPICORN_PRETTY_FUNCTION
#endif

/* --- provide canonical integer types --- */
#if 	RAPICORN_SIZEOF_SYS_TYPESH_UINT == 0
typedef unsigned int		uint;	/* for systems that don't define uint in types.h */
#else
RAPICORN_STATIC_ASSERT (RAPICORN_SIZEOF_SYS_TYPESH_UINT == 4);
#endif
RAPICORN_STATIC_ASSERT (sizeof (uint) == 4);
typedef unsigned int		BirnetUInt8  __attribute__ ((__mode__ (__QI__)));
typedef unsigned int		BirnetUInt16 __attribute__ ((__mode__ (__HI__)));
typedef unsigned int		BirnetUInt32 __attribute__ ((__mode__ (__SI__)));
// typedef unsigned int         BirnetUInt64 __attribute__ ((__mode__ (__DI__)));
typedef unsigned long long int  BirnetUInt64; // AMD64 needs this for %llu printf format strings
// provided by birnetcdefs.h: uint;
RAPICORN_STATIC_ASSERT (sizeof (BirnetUInt8)  == 1);
RAPICORN_STATIC_ASSERT (sizeof (BirnetUInt16) == 2);
RAPICORN_STATIC_ASSERT (sizeof (BirnetUInt32) == 4);
RAPICORN_STATIC_ASSERT (sizeof (BirnetUInt64) == 8);
typedef signed int		BirnetInt8  __attribute__ ((__mode__ (__QI__)));
typedef signed int		BirnetInt16 __attribute__ ((__mode__ (__HI__)));
typedef signed int		BirnetInt32 __attribute__ ((__mode__ (__SI__)));
// typedef signed long long int BirnetInt64 __attribute__ ((__mode__ (__DI__)));
typedef signed long long int	BirnetInt64;  // AMD64 needs this for %lld printf format strings
// provided by compiler       int;
RAPICORN_STATIC_ASSERT (sizeof (BirnetInt8)  == 1);
RAPICORN_STATIC_ASSERT (sizeof (BirnetInt16) == 2);
RAPICORN_STATIC_ASSERT (sizeof (BirnetInt32) == 4);
RAPICORN_STATIC_ASSERT (sizeof (BirnetInt64) == 8);
typedef BirnetUInt32		BirnetUnichar;
RAPICORN_STATIC_ASSERT (sizeof (BirnetUnichar) == 4);


/* --- path handling --- */
#ifdef	RAPICORN_OS_WIN32
#define RAPICORN_DIR_SEPARATOR		  '\\'
#define RAPICORN_DIR_SEPARATOR_S		  "\\"
#define RAPICORN_SEARCHPATH_SEPARATOR	  ';'
#define RAPICORN_SEARCHPATH_SEPARATOR_S	  ";"
#else	/* !RAPICORN_OS_WIN32 */
#define RAPICORN_DIR_SEPARATOR		  '/'
#define RAPICORN_DIR_SEPARATOR_S		  "/"
#define RAPICORN_SEARCHPATH_SEPARATOR	  ':'
#define RAPICORN_SEARCHPATH_SEPARATOR_S	  ":"
#endif	/* !RAPICORN_OS_WIN32 */
#define	RAPICORN_IS_DIR_SEPARATOR(c)    	  ((c) == RAPICORN_DIR_SEPARATOR)
#define RAPICORN_IS_SEARCHPATH_SEPARATOR(c) ((c) == RAPICORN_SEARCHPATH_SEPARATOR)

/* --- initialization --- */
typedef struct {
  bool stand_alone;		/* "stand-alone": no rcfiles, boot scripts, etc. */
  bool test_quick;		/* run quick tests */
  bool test_slow;		/* run slow tests */
  bool test_perf;		/* run benchmarks, test performance */
} BirnetInitSettings;

typedef struct {
  const char *value_name;     	/* value list ends with value_name == NULL */
  const char *value_string;
  long double value_num;     	/* valid if value_string == NULL */
} BirnetInitValue;

/* --- CPU info --- */
typedef struct {
  /* architecture name */
  const char *machine;
  /* CPU Vendor ID */
  const char *cpu_vendor;
  /* CPU features on X86 */
  uint x86_fpu : 1, x86_ssesys : 1, x86_tsc   : 1, x86_htt      : 1;
  uint x86_mmx : 1, x86_mmxext : 1, x86_3dnow : 1, x86_3dnowext : 1;
  uint x86_sse : 1, x86_sse2   : 1, x86_sse3  : 1, x86_sse4     : 1;
} BirnetCPUInfo;

/* --- Thread info --- */
typedef enum {
  RAPICORN_THREAD_UNKNOWN    = '?',
  RAPICORN_THREAD_RUNNING    = 'R',
  RAPICORN_THREAD_SLEEPING   = 'S',
  RAPICORN_THREAD_DISKWAIT   = 'D',
  RAPICORN_THREAD_TRACED     = 'T',
  RAPICORN_THREAD_PAGING     = 'W',
  RAPICORN_THREAD_ZOMBIE     = 'Z',
  RAPICORN_THREAD_DEAD       = 'X',
} BirnetThreadState;
typedef struct {
  int                	thread_id;
  char                 *name;
  uint                 	aborted : 1;
  BirnetThreadState     state;
  int                  	priority;      	/* nice value */
  int                  	processor;     	/* running processor # */
  BirnetUInt64         	utime;		/* user time */
  BirnetUInt64         	stime;         	/* system time */
  BirnetUInt64		cutime;        	/* user time of dead children */
  BirnetUInt64		cstime;		/* system time of dead children */
} BirnetThreadInfo;

/* --- threading ABI --- */
typedef struct _BirnetThread BirnetThread;
typedef void (*BirnetThreadFunc)   (void *user_data);
typedef void (*BirnetThreadWakeup) (void *wakeup_data);
typedef union {
  void	     *cond_pointer;
  BirnetUInt8 cond_dummy[MAX (8, RAPICORN_SIZEOF_PTH_COND_T)];
} BirnetCond;
typedef union {
  void	     *mutex_pointer;
  BirnetUInt8 mutex_dummy[MAX (8, RAPICORN_SIZEOF_PTH_MUTEX_T)];
} BirnetMutex;
typedef struct {
  BirnetMutex   mutex;
  BirnetThread *owner;
  uint 		depth;
} BirnetRecMutex;
typedef struct {
  void              (*mutex_chain4init)     (BirnetMutex       *mutex);
  void              (*mutex_unchain)        (BirnetMutex       *mutex);
  void              (*rec_mutex_chain4init) (BirnetRecMutex    *mutex);
  void              (*rec_mutex_unchain)    (BirnetRecMutex    *mutex);
  void              (*cond_chain4init)      (BirnetCond        *cond);
  void              (*cond_unchain)         (BirnetCond        *cond);
  void		    (*atomic_pointer_set)   (volatile void     *atomic,
					     volatile void     *value);
  void*		    (*atomic_pointer_get)   (volatile void     *atomic);
  int/*bool*/	    (*atomic_pointer_cas)   (volatile void     *atomic,
					     volatile void     *oldval,
					     volatile void     *newval);
  void		    (*atomic_int_set)	    (volatile int      *atomic,
					     int                newval);
  int		    (*atomic_int_get)	    (volatile int      *atomic);
  int/*bool*/	    (*atomic_int_cas)	    (volatile int      *atomic,
					     int           	oldval,
					     int           	newval);
  void		    (*atomic_int_add)	    (volatile int      *atomic,
					     int           	diff);
  int		    (*atomic_int_swap_add)  (volatile int      *atomic,
					     int           	diff);
  void		    (*atomic_uint_set)	    (volatile uint     *atomic,
					     uint               newval);
  uint		    (*atomic_uint_get)	    (volatile uint     *atomic);
  int/*bool*/	    (*atomic_uint_cas)	    (volatile uint     *atomic,
					     uint           	oldval,
					     uint           	newval);
  void		    (*atomic_uint_add)	    (volatile uint     *atomic,
					     uint           	diff);
  uint		    (*atomic_uint_swap_add) (volatile uint     *atomic,
					     uint           	diff);
  BirnetThread*     (*thread_new)           (const char        *name);
  BirnetThread*     (*thread_ref)           (BirnetThread      *thread);
  BirnetThread*     (*thread_ref_sink)      (BirnetThread      *thread);
  void              (*thread_unref)         (BirnetThread      *thread);
  bool              (*thread_start)         (BirnetThread      *thread,
					     BirnetThreadFunc 	func,
					     void              *user_data);
  BirnetThread*     (*thread_self)          (void);
  void*             (*thread_selfxx)        (void);
  void*             (*thread_getxx)         (BirnetThread      *thread);
  bool              (*thread_setxx)         (BirnetThread      *thread,
					     void              *xxdata);
  int               (*thread_pid)           (BirnetThread      *thread);
  const char*       (*thread_name)          (BirnetThread      *thread);
  void              (*thread_set_name)      (const char        *newname);
  bool		    (*thread_sleep)	    (BirnetInt64        max_useconds);
  void		    (*thread_wakeup)	    (BirnetThread      *thread);
  void		    (*thread_awake_after)   (BirnetUInt64       stamp);
  void		    (*thread_emit_wakeups)  (BirnetUInt64       wakeup_stamp);
  void		    (*thread_set_wakeup)    (BirnetThreadWakeup wakeup_func,
					     void              *wakeup_data,
					     void             (*destroy_data) (void*));
  void              (*thread_abort) 	    (BirnetThread      *thread);
  void              (*thread_queue_abort)   (BirnetThread      *thread);
  bool              (*thread_aborted)	    (void);
  bool		    (*thread_get_aborted)   (BirnetThread      *thread);
  bool	            (*thread_get_running)   (BirnetThread      *thread);
  void		    (*thread_wait_for_exit) (BirnetThread      *thread);
  void              (*thread_yield)         (void);
  void              (*thread_exit)          (void              *retval) RAPICORN_NORETURN;
  void              (*thread_set_handle)    (BirnetThread      *handle);
  BirnetThread*     (*thread_get_handle)    (void);
  BirnetThreadInfo* (*info_collect)         (BirnetThread      *thread);
  void              (*info_free)            (BirnetThreadInfo  *info);
  void*		    (*qdata_get)	    (uint               glib_quark);
  void		    (*qdata_set)	    (uint               glib_quark,
					     void              *data,
                                             void             (*destroy_data) (void*));
  void*		    (*qdata_steal)	    (uint		glib_quark);
  void              (*mutex_init)           (BirnetMutex       *mutex);
  void              (*mutex_lock)           (BirnetMutex       *mutex);
  int               (*mutex_trylock)        (BirnetMutex       *mutex); /* 0==has_lock */
  void              (*mutex_unlock)         (BirnetMutex       *mutex);
  void              (*mutex_destroy)        (BirnetMutex       *mutex);
  void              (*rec_mutex_init)       (BirnetRecMutex    *mutex);
  void              (*rec_mutex_lock)       (BirnetRecMutex    *mutex);
  int               (*rec_mutex_trylock)    (BirnetRecMutex    *mutex); /* 0==has_lock */
  void              (*rec_mutex_unlock)     (BirnetRecMutex    *mutex);
  void              (*rec_mutex_destroy)    (BirnetRecMutex    *mutex);
  void              (*cond_init)            (BirnetCond        *cond);
  void              (*cond_signal)          (BirnetCond        *cond);
  void              (*cond_broadcast)       (BirnetCond        *cond);
  void              (*cond_wait)            (BirnetCond        *cond,
					     BirnetMutex       *mutex);
  void		    (*cond_wait_timed)      (BirnetCond        *cond,
					     BirnetMutex       *mutex,
					     BirnetInt64 	max_useconds);
  void              (*cond_destroy)         (BirnetCond        *cond);
} BirnetThreadTable;

/* --- implementation bits --- */
/* the above macros rely on a problem handler macro: */
// RAPICORN__RUNTIME_PROBLEM(ErrorWarningReturnAssertNotreach,domain,file,line,funcname,exprformat,...); // noreturn cases: 'E', 'A', 'N'
extern inline void birnet_abort_noreturn (void) RAPICORN_NORETURN;
extern inline void birnet_abort_noreturn (void) { while (1) *(void*volatile*)0; }
#if RAPICORN_MEMORY_BARRIER_NEEDED
#define RAPICORN_MEMORY_BARRIER_RO(tht)   do { int _b_dummy; tht.atomic_int_get (&_b_dummy); } while (0)
#define RAPICORN_MEMORY_BARRIER_WO(tht)   do { int _b_dummy; tht.atomic_int_set (&_b_dummy, 0); } while (0)
#define RAPICORN_MEMORY_BARRIER_RW(tht)   do { RAPICORN_MEMORY_BARRIER_WO (tht); RAPICORN_MEMORY_BARRIER_RO (tht); } while (0)
#else
#define RAPICORN_MEMORY_BARRIER_RO(tht)   do { } while (0)
#define RAPICORN_MEMORY_BARRIER_WO(tht)   do { } while (0)
#define RAPICORN_MEMORY_BARRIER_RW(tht)   do { } while (0)
#endif         
RAPICORN_EXTERN_C_END();

#endif /* __BIRNET_CDEFS_H__ */

/* vim:set ts=8 sts=2 sw=2: */
