/* RapicornThreadImpl
 * Copyright (C) 2002-2006 Tim Janik
 * Copyright (C) 2002 Stefan Westerfeld
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
#include "rapicornconfig.h" // RAPICORN_HAVE_MUTEXATTR_SETTYPE
#if	(RAPICORN_HAVE_MUTEXATTR_SETTYPE > 0)
#define	_XOPEN_SOURCE   600	/* for full pthread facilities */
#endif	/* defining _XOPEN_SOURCE on random systems can have bad effects */
#include <glib.h>
#include <sys/time.h>
#include <sched.h>
#include <unistd.h>     /* sched_yield() */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/times.h>
#include "birnetutils.hh"
#include "birnetthread.hh"

#define FLOATING_FLAG                           (1 << 31)
#define THREAD_REF_COUNT(thread)                (thread->ref_field & ~FLOATING_FLAG)
#define THREAD_CAS(thread, oldval, newval)      Atomic::uint_cas (&thread->ref_field, oldval, newval)

/* --- some GLib compat --- */
#define HAVE_GSLICE     GLIB_CHECK_VERSION (2, 9, 1)
#if !GLIB_CHECK_VERSION (2, 10, 0)
static void
g_atomic_int_set (volatile int *atomic,
                  int           value)
{
  while (!g_atomic_int_compare_and_exchange ((int*) atomic, *atomic, value));
}
static void
g_atomic_pointer_set (volatile gpointer *atomic,
                      gpointer           value)
{
  while (!g_atomic_pointer_compare_and_exchange ((gpointer*) atomic, *atomic, value));
}
#endif

/* --- structures --- */
struct _RapicornThread
{
  volatile gpointer      threadxx;
  volatile uint          ref_field;
  char		        *name;
  gint8		         aborted;
  gint8		         got_wakeup;
  gint8                  accounting;
  volatile void*         guard_cache;
  Rapicorn::Cond	         wakeup_cond;
  RapicornThreadWakeup     wakeup_func;
  gpointer	         wakeup_data;
  GDestroyNotify         wakeup_destroy;
  guint64	         awake_stamp;
  gint                   tid;
  GData		        *qdata;
  /* accounting */
  struct {
    struct timeval stamp;
    gint64         utime, stime;
    gint64         cutime, cstime;
  }                ac;
  struct {
    guint          processor;
    gint           priority;
    RapicornThreadState state;
    gint           utime, stime;
    gint           cutime, cstime;
  }                info;
};

namespace Rapicorn {

/* --- prototypes --- */
static void             rapicorn_guard_deregister_all   (RapicornThread *thread);
static void	        rapicorn_thread_handle_exit     (RapicornThread *thread);
static void             rapicorn_thread_accounting_L    (RapicornThread *self,
                                                         bool            force_update);
static void             thread_get_tid                  (RapicornThread *thread);
static inline guint     cached_getpid                   (void);


/* --- variables --- */
static Mutex       global_thread_mutex;
static Cond        global_thread_cond;
static Mutex       global_startup_mutex;
static GSList     *global_thread_list = NULL;
static GSList     *thread_awaken_list = NULL;
static RapicornMutex *mutex_init_chain = NULL;
static RapicornMutex *rec_mutex_init_chain = NULL;
static RapicornCond  *cond_init_chain = NULL;


/* --- functions --- */
static RapicornThread*
common_thread_new (const gchar *name)
{
  g_return_val_if_fail (name && name[0], NULL);
  RapicornThread *thread;
#if HAVE_GSLICE
  thread = g_slice_new0 (RapicornThread);
#else
  thread = g_new0 (RapicornThread, 1);
#endif

  ThreadTable.atomic_pointer_set (&thread->threadxx, NULL);
  thread->ref_field = FLOATING_FLAG + 1;
  thread->name = g_strdup (name);
  thread->aborted = FALSE;
  thread->got_wakeup = FALSE;
  thread->accounting = 0;
  thread->guard_cache = NULL;
  new (&thread->wakeup_cond) Cond ();
  thread->wakeup_func = NULL;
  thread->wakeup_destroy = NULL;
  thread->awake_stamp = 0;
  thread->tid = -1;
  g_datalist_init (&thread->qdata);
  return thread;
}

static RapicornThread*
common_thread_ref (RapicornThread *thread)
{
  g_return_val_if_fail (thread != NULL, NULL);
  RAPICORN_ASSERT (THREAD_REF_COUNT (thread) > 0);
  uint32 old_ref, new_ref;
  do {
    old_ref = Atomic::uint_get (&thread->ref_field);
    new_ref = old_ref + 1;
    RAPICORN_ASSERT (new_ref & ~FLOATING_FLAG); /* catch overflow */
  } while (!THREAD_CAS (thread, old_ref, new_ref));
  return thread;
}

static RapicornThread*
common_thread_ref_sink (RapicornThread *thread)
{
  g_return_val_if_fail (thread != NULL, NULL);
  RAPICORN_ASSERT (THREAD_REF_COUNT (thread) > 0);
  ThreadTable.thread_ref (thread);
  uint32 old_ref, new_ref;
  do {
    old_ref = Atomic::uint_get (&thread->ref_field);
    new_ref = old_ref & ~FLOATING_FLAG;
  } while (!THREAD_CAS (thread, old_ref, new_ref));
  if (old_ref & FLOATING_FLAG)
    ThreadTable.thread_unref (thread);
  return thread;
}

static void
common_thread_unref (RapicornThread *thread)
{
  RAPICORN_ASSERT (THREAD_REF_COUNT (thread) > 0);
  uint32 old_ref, new_ref;
  do {
    old_ref = Atomic::uint_get (&thread->ref_field);
    RAPICORN_ASSERT (old_ref & ~FLOATING_FLAG); /* catch underflow */
    new_ref = old_ref - 1;
  } while (!THREAD_CAS (thread, old_ref, new_ref));
  if (0 == (new_ref & ~FLOATING_FLAG))
    {
      g_assert (thread->qdata == NULL);
      g_assert (ThreadTable.atomic_pointer_get (&thread->threadxx) == NULL);
      /* final cleanup code, all custom hooks have been processed now */
      rapicorn_guard_deregister_all (thread);
      thread->wakeup_cond.~Cond();
      g_free (thread->name);
      thread->name = NULL;
#if HAVE_GSLICE
      g_slice_free (RapicornThread, thread);
#else
      g_free (thread);
#endif
    }
}

static void
rapicorn_thread_handle_exit (RapicornThread *thread)
{
  /* run custom data cleanup handlers */
  g_datalist_clear (&thread->qdata);
  thread->wakeup_func = NULL;
  while (thread->wakeup_destroy)
    {
      GDestroyNotify wakeup_destroy = thread->wakeup_destroy;
      thread->wakeup_destroy = NULL;
      wakeup_destroy (thread->wakeup_data);
    }
  g_datalist_clear (&thread->qdata);
  void *threadcxx = ThreadTable.atomic_pointer_get (&thread->threadxx);
  while (threadcxx)
    {
      struct ThreadAccessWrapper : public Thread {
        ThreadAccessWrapper () : Thread ("") {}
        static void tdelete (void *cxxthread) { return threadxx_delete (cxxthread); }
      };
      ThreadAccessWrapper::tdelete (threadcxx);
      g_datalist_clear (&thread->qdata);
      threadcxx = ThreadTable.atomic_pointer_get (&thread->threadxx);
    }
  global_thread_mutex.lock();
  global_thread_list = g_slist_remove (global_thread_list, thread);
  if (thread->awake_stamp)
    thread_awaken_list = g_slist_remove (thread_awaken_list, thread);
  thread->awake_stamp = 1;
  global_thread_cond.broadcast();
  global_thread_mutex.unlock();
  
  /* free thread structure */
  ThreadTable.thread_unref (thread);
}

static void
filter_priority_warning (const gchar    *log_domain,
                         GLogLevelFlags  log_level,
                         const gchar    *message,
                         gpointer        unused_data)
{
  static const char *fmsg = "Priorities can only be increased by root.";
  if (message && strcmp (message, fmsg) == 0)
    ;   /* ignore warning */
  else
    g_log_default_handler (log_domain, log_level, message, unused_data);
}

static gpointer
rapicorn_thread_exec (gpointer data)
{
  void           **tfdx      = (void**) data;
  RapicornThread    *thread    = (RapicornThread*) tfdx[0];
  RapicornThreadFunc func      = (RapicornThreadFunc) tfdx[1];
  gpointer         user_data = tfdx[2];
  ThreadTable.thread_set_handle (thread);
  
  RapicornThread *self = ThreadTable.thread_self ();
  g_assert (self == thread);
  
  thread_get_tid (thread);

  ThreadTable.thread_ref (thread);
  
  global_thread_mutex.lock();
  global_thread_list = g_slist_append (global_thread_list, self);
  self->accounting = 1;
  rapicorn_thread_accounting_L (self, TRUE);
  global_thread_cond.broadcast();
  global_thread_mutex.unlock();
  /* here, tfdx contents have become invalid */

  global_startup_mutex.lock();
  /* acquiring this mutex waits for rapicorn_thread_run() to figure inlist (global_thread_list, self) */
  global_startup_mutex.unlock();
  
  func (user_data);
  g_datalist_clear (&thread->qdata);

  /* because func() can be prematurely exited via pthread_exit(),
   * rapicorn_thread_handle_exit() does unref and final destruction
   */
  return NULL;
}

/**
 * @param thread        a valid, unstarted RapicornThread
 * @param func	        function to execute in new thread
 * @param user_data     user data to pass into @a func
 * @param returns       FALSE in case of error
 *
 * Create a new thread running @a func.
 */
static bool
common_thread_start (RapicornThread    *thread,
                     RapicornThreadFunc func,
                     void            *user_data)
{
  GThread *gthread = NULL;
  GError *gerror = NULL;
  
  g_return_val_if_fail (thread != NULL, FALSE);
  g_return_val_if_fail (thread->tid == -1, FALSE);
  g_return_val_if_fail (func != NULL, FALSE);
  
  ThreadTable.thread_ref (thread);

  /* silence those stupid priority warnings triggered by glib */
  guint hid = g_log_set_handler ("GLib", G_LOG_LEVEL_WARNING, filter_priority_warning, NULL);

  /* thread creation context, protection by global_startup_mutex */
  global_startup_mutex.lock();
  void **tfdx = g_new0 (void*, 4);
  tfdx[0] = thread;
  tfdx[1] = (void*) func;
  tfdx[2] = user_data;
  tfdx[3] = NULL;
  
  thread->tid = cached_getpid();

  /* don't dare setting joinable to TRUE, that prevents the thread's
   * resources from being freed, since we don't offer pthread_join().
   * so we'd just run out of stack at some point.
   */
  const gboolean joinable = FALSE;
  gthread = g_thread_create_full (rapicorn_thread_exec, tfdx, 0, joinable, FALSE,
                                  G_THREAD_PRIORITY_NORMAL, &gerror);
  if (gthread)
    {
      global_thread_mutex.lock();
      while (!g_slist_find (global_thread_list, thread))
        global_thread_cond.wait (global_thread_mutex);
      global_thread_mutex.unlock();
    }
  else
    {
      thread->tid = -1;
      g_message ("failed to create thread \"%s\": %s", thread->name, gerror->message);
      g_error_free (gerror);
    }

  /* let the new thread actually start out */
  global_startup_mutex.unlock();

  /* withdraw warning filter */
  g_free (tfdx);
  g_log_remove_handler ("GLib", hid);

  ThreadTable.thread_unref (thread);

  return gthread != NULL;
}

/**
 * Volountarily give up the curren scheduler time slice and let
 * another process or thread run, if any is in the queue.
 * The effect of this funciton is highly system dependent and
 * may simply result in the current thread being continued.
 */
static void
common_thread_yield (void)
{
#ifdef  _POSIX_PRIORITY_SCHEDULING
  sched_yield();
#else
  g_thread_yield();
#endif
}

/**
 * @return		thread handle
 *
 * Return the thread handle of the currently running thread.
 */
static RapicornThread*
common_thread_self (void)
{
  RapicornThread *thread = ThreadTable.thread_get_handle ();
  if G_UNLIKELY (!thread)
    {
      /* this function is also used during thread initialization,
       * so not all library components are yet usable
       */
      static volatile int anon_count = 1;
      guint id = Atomic::int_swap_add (&anon_count, 1);
      gchar name[256];
      g_snprintf (name, 256, "Anon%u", id);
      thread = ThreadTable.thread_new (name);
      ThreadTable.thread_ref_sink (thread);
      thread_get_tid (thread);
      ThreadTable.thread_set_handle (thread);
      global_thread_mutex.lock();
      global_thread_list = g_slist_append (global_thread_list, thread);
      global_thread_mutex.unlock();
    }
  return thread;
}

static inline void*
common_thread_getxx (RapicornThread *thread)
{
  void *ptr = ThreadTable.atomic_pointer_get (&thread->threadxx);
  if (UNLIKELY (!ptr))
    {
      struct ThreadAccessWrapper : public Thread {
        ThreadAccessWrapper () : Thread ("") {}
        static void wrap (RapicornThread *cthread) { return threadxx_wrap (cthread); }
      };
      ThreadAccessWrapper::wrap (thread);
      ptr = ThreadTable.atomic_pointer_get (&thread->threadxx);
    }
  return ptr;
}

static void*
common_thread_selfxx (void)
{
  RapicornThread *thread = ThreadTable.thread_get_handle ();
  if (UNLIKELY (!thread))
    thread = ThreadTable.thread_self();
  return common_thread_getxx (thread);
}

static bool
common_thread_setxx (RapicornThread *thread,
                     void         *xxdata)
{
  global_thread_mutex.lock();
  bool success = false;
  if (!ThreadTable.atomic_pointer_get (&thread->threadxx) || !xxdata)
    {
      ThreadTable.atomic_pointer_set (&thread->threadxx, xxdata);
      success = true;
    }
  else
    g_error ("attempt to exchange C++ thread handle");
  global_thread_mutex.unlock();
  return success;
}

/**
 * @param thread	a valid RapicornThread handle
 * @return		thread id
 *
 * Return the specific id for @a thread. This function is highly
 * system dependant. The thread id may deviate from the overall
 * process id or not. On linux, threads have their own id,
 * allthough since kernel 2.6, they share the same process id.
 */
static int
common_thread_pid (RapicornThread *thread)
{
  thread = thread ? thread : ThreadTable.thread_self();
  return thread->tid;
}

/**
 * @param thread	a valid RapicornThread handle
 * @return		thread name
 *
 * Return the name of @a thread as specified upon invokation of
 * rapicorn_thread_run() or assigned by rapicorn_thread_set_name().
 */
static const char*
common_thread_name (RapicornThread *thread)
{
  thread = thread ? thread : ThreadTable.thread_self();
  return thread->name;
}

static void
common_thread_set_name (const gchar *name)
{
  RapicornThread *thread = ThreadTable.thread_self ();
  if (name)
    {
      global_thread_mutex.lock();
      g_free (thread->name);
      thread->name = g_strdup (name);
      global_thread_mutex.unlock();
    }
}

static void
rapicorn_thread_wakeup_L (RapicornThread *thread)
{
  thread->wakeup_cond.signal();
  if (thread->wakeup_func)
    thread->wakeup_func (thread->wakeup_data);
  thread->got_wakeup = TRUE;
}

/**
 * @param max_useconds	maximum amount of micro seconds to sleep (-1 for infinite time)
 * @param returns	TRUE while the thread should continue execution
 *
 * Sleep for the amount of time given.
 * This function may get interrupted by wakeup requests from
 * rapicorn_thread_wakeup(), abort requests from rapicorn_thread_queue_abort()
 * or other means. It returns whether the thread is supposed to
 * continue execution after waking up.
 * This function or alternatively rapicorn_thread_aborted() should be called
 * periodically, to react to thread abortion requests and to update
 * internal accounting information.
 */
static bool
common_thread_sleep (RapicornInt64 max_useconds)
{
  RapicornThread *self = ThreadTable.thread_self ();
  bool aborted;
  
  global_thread_mutex.lock();
  
  rapicorn_thread_accounting_L (self, FALSE);
  
  if (!self->got_wakeup && max_useconds != 0)
    {
      if (max_useconds >= 0) /* wait once without time adjustments */
	self->wakeup_cond.wait_timed (global_thread_mutex, max_useconds);
      else /* wait forever */
	while (!self->got_wakeup)
	  self->wakeup_cond.wait (global_thread_mutex);
    }
  
  self->got_wakeup = FALSE;
  aborted = self->aborted != FALSE;
  global_thread_mutex.unlock();
  
  return !aborted;
}

/**
 * @param wakeup_func	wakeup function to be called by rapicorn_thread_wakeup()
 * @param wakeup_data	data passed into wakeup_func()
 * @param destroy	destroy handler for @a wakeup_data
 *
 * Set the wakeup function for the current thread. This enables
 * the thread to be woken up through rapicorn_thread_wakeup() even
 * if not sleeping in rapicorn_thread_sleep(). The wakeup function
 * must be thread-safe, so it may be called from any thread,
 * and it should be fast, because the global thread system lock
 * is held during its invokation.
 * Per thread, the wakeup function may be set only once.
 */
static void
common_thread_set_wakeup (RapicornThreadWakeup wakeup_func,
                          gpointer           wakeup_data,
                          GDestroyNotify     destroy)
{
  RapicornThread *self = ThreadTable.thread_self ();
  
  g_return_if_fail (wakeup_func != NULL);
  g_return_if_fail (self->wakeup_func == NULL);
  
  global_thread_mutex.lock();
  self->wakeup_func = wakeup_func;
  self->wakeup_data = wakeup_data;
  self->wakeup_destroy = destroy;
  global_thread_mutex.unlock();
}

/**
 * @param thread	thread to wake up
 *
 * Wake up a currently sleeping thread. In practice, this
 * function simply causes the next call to rapicorn_thread_sleep()
 * within @a thread to last for 0 seconds.
 */
static void
common_thread_wakeup (RapicornThread *thread)
{
  g_return_if_fail (thread != NULL);
  
  global_thread_mutex.lock();
  g_assert (g_slist_find (global_thread_list, thread));
  rapicorn_thread_wakeup_L (thread);
  global_thread_mutex.unlock();
}

/**
 * @param stamp	stamp to trigger wakeup
 *
 * Wake the current thread up at the next invocation
 * of rapicorn_thread_emit_wakeups() with a wakup_stamp
 * greater than @a stamp.
 */
static void
common_thread_awake_after (RapicornUInt64 stamp)
{
  RapicornThread *self = ThreadTable.thread_self ();
  
  g_return_if_fail (stamp > 0);
  
  global_thread_mutex.lock();
  if (!self->awake_stamp)
    {
      thread_awaken_list = g_slist_prepend (thread_awaken_list, self);
      self->awake_stamp = stamp;
    }
  else
    self->awake_stamp = MIN (self->awake_stamp, stamp);
  global_thread_mutex.unlock();
}

/**
 * @param wakeup_stamp	wakeup stamp to trigger wakeups
 *
 * Wake all currently sleeping threads up which queued
 * a wakeup through rapicorn_thread_awake_after() with a
 * stamp smaller than @a wakeup_stamp.
 */
static void
common_thread_emit_wakeups (RapicornUInt64 wakeup_stamp)
{
  g_return_if_fail (wakeup_stamp > 0);
  
  global_thread_mutex.lock();
  GSList *next, *node;
  for (node = thread_awaken_list; node; node = next)
    {
      RapicornThread *thread = (RapicornThread*) node->data;
      next = node->next;
      if (thread->awake_stamp <= wakeup_stamp)
	{
	  thread->awake_stamp = 0;
	  thread_awaken_list = g_slist_remove (thread_awaken_list, thread);
	  rapicorn_thread_wakeup_L (thread);
	}
    }
  global_thread_mutex.unlock();
}

/**
 * @param thread	thread to abort
 *
 * Abort a currently running thread. This function does not
 * return until the thread in question terminated execution.
 * Note that the thread handle gets invalidated with invocation
 * of rapicorn_thread_abort() or rapicorn_thread_queue_abort().
 */
static void
common_thread_abort (RapicornThread *thread)
{
  g_return_if_fail (thread != NULL);
  g_return_if_fail (thread != ThreadTable.thread_self ());
  
  global_thread_mutex.lock();
  g_assert (g_slist_find (global_thread_list, thread));
  thread->aborted = TRUE;
  rapicorn_thread_wakeup_L (thread);
  while (g_slist_find (global_thread_list, thread))
    global_thread_cond.wait (global_thread_mutex);
  global_thread_mutex.unlock();
}

/**
 * @param thread	thread to abort
 *
 * Same as rapicorn_thread_abort(), but returns as soon as possible,
 * even if thread hasn't stopped execution yet.
 * Note that the thread handle gets invalidated with invocation
 * of rapicorn_thread_abort() or rapicorn_thread_queue_abort().
 */
static void
common_thread_queue_abort (RapicornThread *thread)
{
  g_return_if_fail (thread != NULL);
  
  global_thread_mutex.lock();
  g_assert (g_slist_find (global_thread_list, thread));
  thread->aborted = TRUE;
  rapicorn_thread_wakeup_L (thread);
  global_thread_mutex.unlock();
}

/**
 * @param returns	TRUE if the thread should abort execution
 *
 * Find out if the currently running thread should be aborted (the thread is
 * supposed to return from its main thread function). This function or
 * alternatively rapicorn_thread_sleep() should be called periodically, to
 * react to thread abortion requests and to update internal accounting
 * information.
 */
static bool
common_thread_aborted (void)
{
  RapicornThread *self = ThreadTable.thread_self ();
  global_thread_mutex.lock();
  rapicorn_thread_accounting_L (self, FALSE);
  bool aborted = self->aborted != FALSE;
  global_thread_mutex.unlock();
  return aborted;
}

/**
 * @param thread	thread to abort
 * @param returns	TRUE if the thread should abort execution
 *
 * Find out if the currently running thread should be aborted (the thread is
 * supposed to return from its main thread function). This function or
 * alternatively rapicorn_thread_sleep() should be called periodically, to
 * react to thread abortion requests and to update internal accounting
 * information.
 */
static bool
common_thread_get_aborted (RapicornThread *thread)
{
  global_thread_mutex.lock();
  bool aborted = thread->aborted != FALSE;
  global_thread_mutex.unlock();
  return aborted;
}

static bool
common_thread_get_running (RapicornThread *thread)
{
  global_thread_mutex.lock();
  bool running = g_slist_find (global_thread_list, thread);
  global_thread_mutex.unlock();
  return running;
}

static void
common_thread_wait_for_exit (RapicornThread *thread)
{
  global_thread_mutex.lock();
  while (g_slist_find (global_thread_list, thread))
    global_thread_cond.wait (global_thread_mutex);
  global_thread_mutex.unlock();
}

static gpointer
common_thread_get_qdata (GQuark quark)
{
  RapicornThread *self = ThreadTable.thread_self ();
  return quark ? g_datalist_id_get_data (&self->qdata, quark) : NULL;
}

static void
common_thread_set_qdata_full (GQuark         quark,
                              gpointer       data,
                              GDestroyNotify destroy)
{
  RapicornThread *self = ThreadTable.thread_self ();
  g_return_if_fail (quark > 0);
  g_datalist_id_set_data_full (&self->qdata, quark, data,
			       data ? destroy : (GDestroyNotify) NULL);
}

static gpointer
common_thread_steal_qdata (GQuark quark)
{
  RapicornThread *self = ThreadTable.thread_self ();
  return quark ? g_datalist_id_remove_no_notify (&self->qdata, quark) : NULL;
}

static bool
common_split_useconds (RapicornInt64   max_useconds,
                       RapicornUInt64 *abs_secs,
                       RapicornUInt64 *abs_usecs)
{
  if (max_useconds < 0)
    return false;
  struct timeval now;
  gettimeofday (&now, NULL);
  RapicornUInt64 secs = max_useconds / 1000000;
  RapicornUInt64 limit_sec = now.tv_sec + secs;
  max_useconds -= secs * 1000000;
  RapicornUInt64 limit_usec = now.tv_usec + max_useconds;
  if (limit_usec >= 1000000)
    {
      limit_usec -= 1000000;
      limit_sec += 1;
    }
  *abs_secs = limit_sec;
  *abs_usecs = limit_usec;
  return true;
}

static inline guint
cached_getpid (void)
{
  static pid_t cached_pid = 0;
  if (G_UNLIKELY (!cached_pid))
    cached_pid = getpid();
  return cached_pid;
}

#ifdef  __linux__
#include <sys/types.h>
#include <sys/syscall.h>
#include <linux/unistd.h>
#endif
static void
thread_get_tid (RapicornThread *thread)
{
  gint ppid = thread->tid;      /* creator process id */
  gint tid = -1;
  
#if     defined (__linux__) && defined (__NR_gettid)    /* present on linux >= 2.4.20 */
  tid = syscall (__NR_gettid);
#endif
  if (tid < 0)
    tid = cached_getpid();
  if (tid != ppid &&            /* thread pid different from creator pid, probably correct */
      tid > 0)
    thread->tid = tid;
  else
    thread->tid = 0;            /* failed to figure id */
}


/* --- thread info --- */
static inline unsigned long long int
timeval_usecs (const struct timeval *tv)
{
  return tv->tv_usec + tv->tv_sec * (guint64) 1000000;
}

static void
thread_info_from_stat_L (RapicornThread *self,
                         double     usec_norm)
{
  int pid = -1, ppid = -1, pgrp = -1, session = -1, tty_nr = -1, tpgid = -1;
  int exit_signal = 0, processor = 0;
  long cutime = 0, cstime = 0, priority = 0, nice = 0, dummyld = 0;
  long itrealvalue = 0, rss = 0;
  unsigned long flags = 0, minflt = 0, cminflt = 0, majflt = 0, cmajflt = 0;
  unsigned long utime = 0, stime = 0, vsize = 0, rlim = 0, startcode = 0;
  unsigned long endcode = 0, startstack = 0, kstkesp = 0, kstkeip = 0;
  unsigned long signal = 0, blocked = 0, sigignore = 0, sigcatch = 0;
  unsigned long wchan = 0, nswap = 0, cnswap = 0, rt_priority = 0, policy = 0;
  unsigned long long starttime = 0;
  char state = 0, command[8192 + 1] = { 0 };
  FILE *file = NULL;
  int n = 0;
  static int have_stat = 1;
  if (have_stat)
    {
      gchar *filename = g_strdup_printf ("/proc/%u/task/%u/stat", cached_getpid(), self->tid);
      file = fopen (filename, "r");
      g_free (filename);
      if (!file)
        have_stat = 0;  /* reading /proc/self/stat should always succeed, so try only once */
    }
  if (file)
    n = fscanf (file,
                "%d %8192s %c "
                "%d %d %d %d %d "
                "%lu %lu %lu %lu %lu %lu %lu "
                "%ld %ld %ld %ld %ld %ld "
                "%llu %lu %ld "
                "%lu %lu %lu %lu %lu "
                "%lu %lu %lu %lu %lu "
                "%lu %lu %lu %d %d "
                "%lu %lu",
                &pid, command, &state, /* n=3 */
                &ppid, &pgrp, &session, &tty_nr, &tpgid, /* n=8 */
                &flags, &minflt, &cminflt, &majflt, &cmajflt, &utime, &stime, /* n=15 */
                &cutime, &cstime, &priority, &nice, &dummyld, &itrealvalue, /* n=21 */
                &starttime, &vsize, &rss, /* n=24 */
                &rlim, &startcode, &endcode, &startstack, &kstkesp, /* n=29 */
                &kstkeip, &signal, &blocked, &sigignore, &sigcatch, /* n=34 */
                &wchan, &nswap, &cnswap, &exit_signal, &processor, /* n=39 */
                &rt_priority, &policy /* n=41 */
                );
  if (file)
    fclose (file);

  if (n >= 15)
    {
      self->ac.utime = utime * 10000;
      self->ac.stime = stime * 10000;
    }
  if (n >= 17)
    {
      self->ac.cutime = cutime * 10000;
      self->ac.cstime = cstime * 10000;
    }
  if (n >= 3)
    self->info.state = RapicornThreadState (state);
  if (n >= 39)
    self->info.processor = 1 + processor;
}

#define ACCOUNTING_MSECS        (500)

static void
rapicorn_thread_accounting_L (RapicornThread *self,
                              bool          force_update)
{
  struct timeval stamp, ostamp = self->ac.stamp;
  guint diff = 0;
  if (self->accounting)
    {
      gettimeofday (&stamp, NULL);
      diff = timeval_usecs (&stamp) - timeval_usecs (&ostamp);
      diff = MAX (diff, 0);
    }
  if (force_update || diff >= ACCOUNTING_MSECS * 1000)  /* limit accounting to a few times per second */
    {
      gint64 old_utime = self->ac.utime;
      gint64 old_stime = self->ac.stime;
      gint64 old_cutime = self->ac.cutime;
      gint64 old_cstime = self->ac.cstime;
      gdouble dfact = 1000000.0 / MAX (diff, 1);
      self->ac.stamp = stamp;
      if (0)
        {
          struct rusage res = { { 0 } };
          getrusage (RUSAGE_SELF, &res);
          self->ac.utime = timeval_usecs (&res.ru_utime); /* user time used */
          self->ac.stime = timeval_usecs (&res.ru_stime); /* system time used */
          getrusage (RUSAGE_CHILDREN, &res);
          self->ac.cutime = timeval_usecs (&res.ru_utime);
          self->ac.cstime = timeval_usecs (&res.ru_stime);
        }
      thread_info_from_stat_L (self, dfact);
      self->info.priority = getpriority (PRIO_PROCESS, self->tid);
      self->info.utime = int64 (MAX (self->ac.utime - old_utime, 0) * dfact);
      self->info.stime = int64 (MAX (self->ac.stime - old_stime, 0) * dfact);
      self->info.cutime = int64 (MAX (self->ac.cutime - old_cutime, 0) * dfact);
      self->info.cstime = int64 (MAX (self->ac.cstime - old_cstime, 0) * dfact);
      self->accounting--;
    }
}

static RapicornThreadInfo*
common_thread_info_collect (RapicornThread *thread)
{
  RapicornThreadInfo *info = g_new0 (RapicornThreadInfo, 1);
  struct timeval now;
  gboolean recent = TRUE;
  if (!thread)
    thread = ThreadTable.thread_self ();
  gettimeofday (&now, NULL);
  global_thread_mutex.lock();
  info->name = g_strdup (thread->name);
  info->aborted = thread->aborted;
  info->thread_id = thread->tid;
  if (timeval_usecs (&thread->ac.stamp) + ACCOUNTING_MSECS * 1000 < timeval_usecs (&now))
    recent = FALSE;             /* accounting data too old */
  info->state = thread->info.state;
  info->priority = thread->info.priority;
  info->processor = thread->info.processor;
  if (recent)
    {
      info->utime = thread->info.utime;
      info->stime = thread->info.stime;
      info->cutime = thread->info.cutime;
      info->cstime = thread->info.cstime;
    }
  thread->accounting = 5;       /* update accounting info in the future */
  global_thread_mutex.unlock();
  return info;
}

static void
common_thread_info_free (RapicornThreadInfo  *info)
{
  g_return_if_fail (info != NULL);
  g_free (info->name);
  g_free (info);
}

/* --- structure chaining for initialization --- */
static void
common_mutex_chain4init (RapicornMutex *mutex)
{
  g_assert (mutex->mutex_pointer == NULL);
  mutex->mutex_pointer = mutex_init_chain;
  mutex_init_chain = mutex;
}

static void
common_mutex_unchain (RapicornMutex *mutex)
{
  RapicornMutex *last = NULL, *m = mutex_init_chain;
  while (m != mutex)
    {
      last = m;
      m = (RapicornMutex*) last->mutex_pointer;
    }
  if (last)
    last->mutex_pointer = mutex->mutex_pointer;
  else
    mutex_init_chain = (RapicornMutex*) mutex->mutex_pointer;
}

static void
common_rec_mutex_chain4init (RapicornRecMutex *rec_mutex)
{
  RAPICORN_STATIC_ASSERT (offsetof (RapicornRecMutex, mutex) == 0);
  g_assert (rec_mutex->mutex.mutex_pointer == NULL);
  rec_mutex->mutex.mutex_pointer = rec_mutex_init_chain;
  rec_mutex_init_chain = &rec_mutex->mutex;
}

static void
common_rec_mutex_unchain (RapicornRecMutex *rec_mutex)
{
  RapicornMutex *mutex = (RapicornMutex*) rec_mutex;
  RapicornMutex *last = NULL, *m = rec_mutex_init_chain;
  while (m != mutex)
    {
      last = m;
      m = (RapicornMutex*) last->mutex_pointer;
    }
  if (last)
    last->mutex_pointer = mutex->mutex_pointer;
  else
    rec_mutex_init_chain = (RapicornMutex*) mutex->mutex_pointer;
}

static void
common_cond_chain4init (RapicornCond *cond)
{
  g_assert (cond->cond_pointer == NULL);
  cond->cond_pointer = cond_init_chain;
  cond_init_chain = cond;
}

static void
common_cond_unchain (RapicornCond *cond)
{
  RapicornCond *last = NULL, *c = cond_init_chain;
  while (c != cond)
    {
      last = c;
      c = (RapicornCond*) last->cond_pointer;
    }
  if (last)
    last->cond_pointer = cond->cond_pointer;
  else
    cond_init_chain = (RapicornCond*) cond->cond_pointer;
}

/* --- hazard pointer guards --- */
struct RapicornGuard
{
  volatile RapicornGuard *next;       /* global guard list */
  RapicornThread         *thread;
  volatile RapicornGuard *cache_next; /* per thread free list */
  guint                 n_values;
  volatile gpointer     values[1];  /* variable length array */
};
static volatile RapicornGuard * volatile guard_list = NULL;
static gint       volatile guard_list_length = 0;
#define RAPICORN_GUARD_ALIGN  (4)
#define guard2values(ptr)       G_STRUCT_MEMBER_P (ptr, +G_STRUCT_OFFSET (RapicornGuard, values[0]))
#define values2guard(ptr)       G_STRUCT_MEMBER_P (ptr, -G_STRUCT_OFFSET (RapicornGuard, values[0]))

/**
 * @param n_hazards	number of required hazard pointers
 * @return		a valid RapicornGuard
 *
 * Retrieve a new guard for node protection of the current thread.
 * The exact mechanism of protection is described in rapicorn_guard_protect().
 * Note that rapicorn_guard_snap_values() will walk the hazard pointer
 * array in ascending order, so that pointers may migrate from array
 * positions with a lower index to positions with a higher index while
 * retaining protection, according to condition C2 as described in
 * http://www.research.ibm.com/people/m/michael/podc-2002.pdf.
 * If an equally or bigger sized hazard pointer array was previously
 * deregistered by this thread, registration takes constant time.
 */
static volatile RapicornGuard*
rapicorn_guard_register (guint n_hazards) RAPICORN_UNUSED;
static volatile RapicornGuard*
rapicorn_guard_register (guint n_hazards)
{
  RapicornThread *thread = ThreadTable.thread_self();
  volatile RapicornGuard *guard, *last = NULL;
  /* reuse cached guards */
  for (guard = (volatile Rapicorn::RapicornGuard*) thread->guard_cache; guard; last = guard, guard = last->cache_next)
    if (n_hazards <= guard->n_values)
      {
        if (last)
          last->cache_next = guard->cache_next;
        else
          thread->guard_cache = guard->cache_next;
        guard->cache_next = NULL;
        break;
      }
  /* allocate new guard */
  if (!guard)
    {
      n_hazards = ((MAX (n_hazards, 3) + RAPICORN_GUARD_ALIGN - 1) / RAPICORN_GUARD_ALIGN) * RAPICORN_GUARD_ALIGN;
      Atomic::int_add (&guard_list_length, n_hazards);
      guard = (volatile Rapicorn::RapicornGuard*) g_malloc0 (sizeof (RapicornGuard) + (n_hazards - 1) * sizeof (guard->values[0]));
      guard->n_values = n_hazards;
      guard->thread = thread;
      do
        guard->next = (volatile RapicornGuard*) ThreadTable.atomic_pointer_get ((volatile void*) &guard_list);
      while (!ThreadTable.atomic_pointer_cas (&guard_list, guard->next, guard));
    }
  return (volatile RapicornGuard*) guard2values (guard);
}

/**
 * @param guard	a valid RapicornGuard as returned from rapicorn_guard_register()
 *
 * Deregister a guard previously registered by a call to rapicorn_guard_register().
 * Deregistration is performed in constant time.
 */
static void RAPICORN_UNUSED
rapicorn_guard_deregister (volatile RapicornGuard *guard)
{
  guard = (volatile RapicornGuard*) values2guard (guard);
  RapicornThread *thread = ThreadTable.thread_self();
  g_return_if_fail (guard->thread == thread);
  memset ((guint8*) guard->values, 0, sizeof (guard->values[0]) * guard->n_values);
  /* FIXME: must we have a memory barrier here? */
  guard->cache_next = (volatile RapicornGuard*) thread->guard_cache;
  thread->guard_cache = guard;
}

static void
rapicorn_guard_deregister_all (RapicornThread *thread)
{
  volatile RapicornGuard *guard;
  thread->guard_cache = NULL;
  for (guard = (volatile RapicornGuard*) ThreadTable.atomic_pointer_get (&guard_list); guard; guard = guard->next)
    if (guard->thread == thread)
      {
        memset ((guint8*) guard->values, 0, sizeof (guard->values[0]) * guard->n_values);
        guard->cache_next = NULL;
        ThreadTable.atomic_pointer_cas (&guard->thread, thread, NULL); /* reset ->thread with memory barrier */
      }
}

/**
 * @param guard	a valid RapicornGuard as returned from rapicorn_guard_register()
 * @param nth_hazard	index of the hazard pointer to use for protection
 * @param value	a hazardous pointer value or NULL to reset protection
 *
 * Protect the node pointed to by @a value from being destroyed by another
 * thread and against the ABA problem caused by premature reuse.
 * For this to work, threads destroying nodes of the type pointed to by
 * @a value need to suspend destruction as long as nodes are protected,
 * which can by checked by calls to rapicorn_guard_is_protected() or by
 * searching the values returned from rapicorn_guard_snap_values().
 * Descriptions of safe memory reclamation and ABA problem detection
 * via hazard pointers guards can be found in
 * http://www.research.ibm.com/people/m/michael/podc-2002.pdf,
 * http://www.cs.brown.edu/people/mph/HerlihyLM02/smli_tr-2002-112.pdf,
 * http://research.sun.com/scalable/Papers/CATS2003.pdf and
 * http://www.research.ibm.com/people/m/michael/ieeetpds-2004.pdf.
 * The exact sequence of steps to protect and access a node is as follows:
 * @* 1) Store the adress of a node to be protected in a hazard pointer
 * @* 2) Verify that the hazard pointer points to a valid node
 * @* 3) Dereference the node only as long as it's protected by the hazard pointer.
 * @* For example:
 * @* 0: RapicornGuard *guard = rapicorn_guard_register (1);
 * @* 1: peek_head_label:
 * @* 2: auto GSList *node = shared_list_head;
 * @* 3: rapicorn_guard_protect (guard, 0, node);
 * @* 4: if (node != shared_list_head) goto peek_head_label;
 * @* 5: operate_on_protected_node (node);
 * @* 6: rapicorn_guard_deregister (guard);
 */
#if 0
static inline
void rapicorn_guard_protect (volatile RapicornGuard *guard,  /* defined in rapicornthreads.h */
                             guint     nth_hazard,
                             gpointer  value);
#endif

/**
 * @return		an upper bound on the number of registered hazard pointers
 *
 * Retrieve an upper bound on the number of hazard pointer value slots
 * currently required for a successfull call to rapicorn_guard_snap_values().
 * Note that a subsequent call to rapicorn_guard_snap_values() may still fail
 * due to addtional guards being registerted meanwhile. In such a case
 * rapicorn_guard_n_snap_values() and rapicorn_guard_snap_values() can simply be
 * called again.
 */
static guint RAPICORN_UNUSED
rapicorn_guard_n_snap_values (void)
{
  return Atomic::int_get (&guard_list_length);
}

/**
 * @param n_values	location of n_values variable
 * @param values	value array to fill in
 * @return		TRUE if @a values provided enough space and is filled
 *
 * Make a snapshot of all non-NULL hazard pointer values.
 * TRUE is returned if the number of non-NULL hazard pointer
 * values didn't exceed the size of the input value array provided
 * by @a n_values, and all values are returned in the array pointed
 * to by @a values.
 * The number of values filled in is returned in @a n_values.
 * FALSE is returned if not enough space was available to return
 * all non-NULL values. rapicorn_guard_n_snap_values() may be used to
 * retrieve the current upper bound on the number of registered
 * guards. Note that a successive call to rapicorn_guard_snap_values() with
 * the requested number of value slots supplied may still fail,
 * because additional guards may have been registered meanwhile.
 * In such a case rapicorn_guard_n_snap_values() and rapicorn_guard_snap_values()
 * can simply be called again.
 * This funciton will always walk the hazard pointer arrays supplied
 * by rapicorn_guard_register() in ascending order, to allow pointer migration
 * from lower to higher array indieces while retaining protection.
 * The returned pointer values are unordered, so in order to perform
 * multiple pointer lookups, we recommend sorting the returned array
 * and then doing binary lookups. However if only a single pointer
 * is to be looked up, calling rapicorn_guard_is_protected() should be
 * considered.
 */
static bool RAPICORN_UNUSED
rapicorn_guard_snap_values (guint          *n_values,
                            gpointer       *values)
{
  guint i, n = 0;
  volatile RapicornGuard *guard;
  for (guard = (volatile RapicornGuard*) ThreadTable.atomic_pointer_get (&guard_list); guard; guard = guard->next)
    if (guard->thread)
      for (i = 0; i < guard->n_values; i++)
        {
          gpointer v = guard->values[i];
          if (v)
            {
              n++;
              if (n > *n_values)
                return FALSE;   /* not enough space provided */
              *values++ = v;
            }
        }
  *n_values = n;                /* number of values used */
  return TRUE;
}

/**
 * @param value	hazard pointer value
 * @return		TRUE if a hazard pointer protecting @a value has been found
 *
 * Check whether @a value is protected by a hazard pointer guard.
 * If multiple pointer values are to be checked, use rapicorn_guard_snap_values()
 * instead, as this function has O(n_hazard_pointers) time complexity.
 * If only one pointer value needs to be looked up though,
 * calling rapicorn_guard_is_protected() will provide a result faster than
 * calling rapicorn_guard_snap_values() and looking up the pointer in the
 * filled-in array.
 * Lookup within hazard pointer arrays will always occour in ascending
 * order to allow pointer migration as described in rapicorn_guard_snap_values()
 * and rapicorn_guard_register().
 */
static bool RAPICORN_UNUSED
rapicorn_guard_is_protected (gpointer value)
{
  if (value)
    {
      volatile RapicornGuard *guard;
      guint i;
      for (guard = (volatile RapicornGuard*) ThreadTable.atomic_pointer_get (&guard_list); guard; guard = guard->next)
        if (guard->thread)
          for (i = 0; i < guard->n_values; i++)
            if (guard->values[i] == value)
              return TRUE;
    }
  return FALSE;
}

/* --- fallback (GLib) RapicornThreadTable --- */
static GPrivate *fallback_thread_table_key = NULL;

static void
fallback_thread_set_handle (RapicornThread *handle)
{
  g_private_set (fallback_thread_table_key, handle);
}

static RapicornThread*
fallback_thread_get_handle (void)
{
  return (RapicornThread*) g_private_get (fallback_thread_table_key);
}

static void
fallback_mutex_init (RapicornMutex *mutex)
{
  g_return_if_fail (mutex != NULL);
  mutex->mutex_pointer = g_mutex_new ();
}

static int
fallback_mutex_trylock (RapicornMutex *mutex)
{
  return g_mutex_trylock ((GMutex*) mutex->mutex_pointer) ? 0 : -1;
}

static void
fallback_mutex_lock (RapicornMutex *mutex)
{
  static gboolean is_smp_system = FALSE; // FIXME
  
  /* spin locks should be held only very short times,
   * so usually, we should succeed here.
   */
  if (g_mutex_trylock ((GMutex*) mutex->mutex_pointer))
    return;
  
  if (!is_smp_system)
    {
      /* on uni processor systems, there's no point in busy spinning */
      do
	{
	  ThreadTable.thread_yield();
	  if (g_mutex_trylock ((GMutex*) mutex->mutex_pointer))
	    return;
	}
      while (TRUE);
    }
  else
    {
      /* for multi processor systems, mutex_lock() is hopefully implemented
       * via spinning. note that we can't implement spinning ourselves with
       * mutex_trylock(), since on some architectures that'd block memory
       * bandwith due to constant bus locks
       */
      g_mutex_lock ((GMutex*) mutex->mutex_pointer);
    }
}

static void
fallback_mutex_unlock (RapicornMutex *mutex)
{
  g_mutex_unlock ((GMutex*) mutex->mutex_pointer);
}

static void
fallback_mutex_destroy (RapicornMutex *mutex)
{
  g_mutex_free ((GMutex*) mutex->mutex_pointer);
  memset (mutex, 0, sizeof (*mutex));
}

static void
fallback_rec_mutex_init (RapicornRecMutex *rec_mutex)
{
  rec_mutex->owner = NULL;
  rec_mutex->depth = 0;
  ThreadTable.mutex_init (&rec_mutex->mutex);
}

static int
fallback_rec_mutex_trylock (RapicornRecMutex *rec_mutex)
{
  RapicornThread *self = ThreadTable.thread_self ();
  
  if (rec_mutex->owner == self)
    {
      g_assert (rec_mutex->depth > 0);  /* paranoid */
      rec_mutex->depth += 1;
      return 0;
    }
  else
    {
      if (ThreadTable.mutex_trylock (&rec_mutex->mutex))
	{
	  g_assert (rec_mutex->owner == NULL && rec_mutex->depth == 0); /* paranoid */
	  rec_mutex->owner = self;
	  rec_mutex->depth = 1;
	  return 0;
	}
    }
  return -1;
}

static void
fallback_rec_mutex_lock (RapicornRecMutex *rec_mutex)
{
  RapicornThread *self = ThreadTable.thread_self ();
  
  if (rec_mutex->owner == self)
    {
      g_assert (rec_mutex->depth > 0);  /* paranoid */
      rec_mutex->depth += 1;
    }
  else
    {
      ThreadTable.mutex_lock (&rec_mutex->mutex);
      g_assert (rec_mutex->owner == NULL && rec_mutex->depth == 0); /* paranoid */
      rec_mutex->owner = self;
      rec_mutex->depth = 1;
    }
}

static void
fallback_rec_mutex_unlock (RapicornRecMutex *rec_mutex)
{
  RapicornThread *self = ThreadTable.thread_self ();
  
  if (rec_mutex->owner == self && rec_mutex->depth > 0)
    {
      rec_mutex->depth -= 1;
      if (!rec_mutex->depth)
	{
	  rec_mutex->owner = NULL;
	  ThreadTable.mutex_unlock (&rec_mutex->mutex);
	}
    }
  else
    g_warning ("unable to unlock recursive mutex with self %p != %p or depth %u < 1",
	       rec_mutex->owner, self, rec_mutex->depth);
}

static void
fallback_rec_mutex_destroy (RapicornRecMutex *rec_mutex)
{
  if (rec_mutex->owner || rec_mutex->depth)
    g_warning ("recursive mutex still locked during destruction");
  else
    {
      ThreadTable.mutex_destroy (&rec_mutex->mutex);
      g_assert (rec_mutex->owner == NULL && rec_mutex->depth == 0);
    }
}

static void
fallback_cond_init (RapicornCond *cond)
{
  cond->cond_pointer = g_cond_new ();
}

static void
fallback_cond_signal (RapicornCond *cond)
{
  g_cond_signal ((GCond*) cond->cond_pointer);
}

static void
fallback_cond_broadcast (RapicornCond *cond)
{
  g_cond_broadcast ((GCond*) cond->cond_pointer);
}

static void
fallback_cond_wait (RapicornCond  *cond,
                    RapicornMutex *mutex)
{
  /* infinite wait */
  g_cond_wait ((GCond*) cond->cond_pointer, (GMutex*) mutex->mutex_pointer);
}

static void
fallback_cond_wait_timed (RapicornCond  *cond,
                          RapicornMutex *mutex,
                          RapicornInt64  max_useconds)
{
  RapicornUInt64 abs_secs, abs_usecs;
  if (common_split_useconds (max_useconds, &abs_secs, &abs_usecs))
    {
      GTimeVal gtime;
      gtime.tv_sec = abs_secs;
      gtime.tv_usec = abs_usecs;
      g_cond_timed_wait ((GCond*) cond->cond_pointer, (GMutex*) mutex->mutex_pointer, &gtime);
    }
  else
    g_cond_wait ((GCond*) cond->cond_pointer, (GMutex*) mutex->mutex_pointer);
}

static void
fallback_cond_destroy (RapicornCond *cond)
{
  g_cond_free ((GCond*) cond->cond_pointer);
}

static void G_GNUC_NORETURN
fallback_thread_exit (gpointer retval)
{
  g_thread_exit (retval);
  abort(); /* silence compiler */
}

#ifdef g_atomic_int_get
static int
(g_atomic_int_get) (volatile int *atomic)
{
  return g_atomic_int_get (atomic);
}
#endif

#ifdef g_atomic_pointer_get
static void*
(g_atomic_pointer_get) (volatile void **atomic)
{
  return (void*) g_atomic_pointer_get (atomic);
}
#endif


static RapicornThreadTable fallback_thread_table = {
  NULL, /* mutex_chain4init */
  NULL, /* mutex_unchain */
  NULL, /* rec_mutex_chain4init */
  NULL, /* rec_mutex_unchain */
  NULL, /* cond_chain4init */
  NULL, /* cond_unchain */
  (void (*) (volatile void*, volatile void*))                 g_atomic_pointer_set,
  (void*(*) (volatile void*))                                 g_atomic_pointer_get,
  (int  (*) (volatile void*, volatile void*, volatile void*)) g_atomic_pointer_compare_and_exchange,
  g_atomic_int_set,
  g_atomic_int_get,
  (int  (*) (volatile  int*,  int,  int)) g_atomic_int_compare_and_exchange,
  (void (*) (volatile  int*,  int))       g_atomic_int_add,
  (int  (*) (volatile  int*,  int))       g_atomic_int_exchange_and_add,
  (void (*) (volatile uint*, uint))       g_atomic_int_set,
  (uint (*) (volatile uint*))             g_atomic_int_get,
  (int  (*) (volatile uint*, uint, uint)) g_atomic_int_compare_and_exchange,
  (void (*) (volatile uint*, uint))       g_atomic_int_add,
  (uint (*) (volatile uint*, uint))       g_atomic_int_exchange_and_add,
  common_thread_new,
  common_thread_ref,
  common_thread_ref_sink,
  common_thread_unref,
  common_thread_start,
  common_thread_self,
  common_thread_selfxx,
  common_thread_getxx,
  common_thread_setxx,
  common_thread_pid,
  common_thread_name,
  common_thread_set_name,
  common_thread_sleep,
  common_thread_wakeup,
  common_thread_awake_after,
  common_thread_emit_wakeups,
  common_thread_set_wakeup,
  common_thread_abort,
  common_thread_queue_abort,
  common_thread_aborted,
  common_thread_get_aborted,
  common_thread_get_running,
  common_thread_wait_for_exit,
  common_thread_yield,
  fallback_thread_exit,
  fallback_thread_set_handle,
  fallback_thread_get_handle,
  common_thread_info_collect,
  common_thread_info_free,
  common_thread_get_qdata,
  common_thread_set_qdata_full,
  common_thread_steal_qdata,
  fallback_mutex_init,
  fallback_mutex_lock,
  fallback_mutex_trylock,
  fallback_mutex_unlock,
  fallback_mutex_destroy,
  fallback_rec_mutex_init,
  fallback_rec_mutex_lock,
  fallback_rec_mutex_trylock,
  fallback_rec_mutex_unlock,
  fallback_rec_mutex_destroy,
  fallback_cond_init,
  fallback_cond_signal,
  fallback_cond_broadcast,
  fallback_cond_wait,
  fallback_cond_wait_timed,
  fallback_cond_destroy,
};

static RapicornThreadTable*
get_fallback_thread_table (void)
{
  fallback_thread_table_key = g_private_new ((GDestroyNotify) rapicorn_thread_handle_exit);
  return &fallback_thread_table;
}


/* --- POSIX threads table --- */
#if	(RAPICORN_HAVE_MUTEXATTR_SETTYPE > 0)
#include <pthread.h>
static pthread_key_t pth_thread_table_key = 0;
static void
pth_thread_set_handle (RapicornThread *handle)
{
  RapicornThread *tmp = (RapicornThread*) pthread_getspecific (pth_thread_table_key);
  pthread_setspecific (pth_thread_table_key, handle);
  if (tmp)
    rapicorn_thread_handle_exit (tmp);
}
static RapicornThread*
pth_thread_get_handle (void)
{
  return (RapicornThread*) pthread_getspecific (pth_thread_table_key);
}

static void
pth_mutex_init (RapicornMutex *mutex)
{
  /* need NULL attribute here, which is the fast mutex in glibc
   * and cannot be chosen through pthread_mutexattr_settype()
   */
  pthread_mutex_init ((pthread_mutex_t*) mutex, NULL);
}

static void
pth_rec_mutex_init (RapicornRecMutex *mutex)
{
  RAPICORN_STATIC_ASSERT (offsetof (RapicornRecMutex, mutex) == 0);
  pthread_mutexattr_t attr;
  pthread_mutexattr_init (&attr);
  pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init ((pthread_mutex_t*) mutex, &attr);
  pthread_mutexattr_destroy (&attr);
}

static void
pth_cond_init (RapicornCond *cond)
{
  pthread_cond_init ((pthread_cond_t*) cond, NULL);
}

static void
pth_cond_wait_timed (RapicornCond  *cond,
                     RapicornMutex *mutex,
                     RapicornInt64  max_useconds)
{
  RapicornUInt64 abs_secs, abs_usecs;
  if (common_split_useconds (max_useconds, &abs_secs, &abs_usecs))
    {
      struct timespec abstime;
      abstime.tv_sec = abs_secs;
      abstime.tv_nsec = abs_usecs * 1000;
      pthread_cond_timedwait ((pthread_cond_t*) cond, (pthread_mutex_t*) mutex, &abstime);
    }
  else
    pthread_cond_wait ((pthread_cond_t*) cond, (pthread_mutex_t    *) mutex);
}

static RapicornThreadTable pth_thread_table = {
  NULL, /* mutex_chain4init */
  NULL, /* mutex_unchain */
  NULL, /* rec_mutex_chain4init */
  NULL, /* rec_mutex_unchain */
  NULL, /* cond_chain4init */
  NULL, /* cond_unchain */
  (void (*) (volatile void*, volatile void*))                 g_atomic_pointer_set,
  (void*(*) (volatile void*))                                 g_atomic_pointer_get,
  (int  (*) (volatile void*, volatile void*, volatile void*)) g_atomic_pointer_compare_and_exchange,
  g_atomic_int_set,
  g_atomic_int_get,
  (int  (*) (volatile  int*,  int,  int)) g_atomic_int_compare_and_exchange,
  (void (*) (volatile  int*,  int))       g_atomic_int_add,
  (int  (*) (volatile  int*,  int))       g_atomic_int_exchange_and_add,
  (void (*) (volatile uint*, uint))       g_atomic_int_set,
  (uint (*) (volatile uint*))             g_atomic_int_get,
  (int  (*) (volatile uint*, uint, uint)) g_atomic_int_compare_and_exchange,
  (void (*) (volatile uint*, uint))       g_atomic_int_add,
  (uint (*) (volatile uint*, uint))       g_atomic_int_exchange_and_add,
  common_thread_new,
  common_thread_ref,
  common_thread_ref_sink,
  common_thread_unref,
  common_thread_start,
  common_thread_self,
  common_thread_selfxx,
  common_thread_getxx,
  common_thread_setxx,
  common_thread_pid,
  common_thread_name,
  common_thread_set_name,
  common_thread_sleep,
  common_thread_wakeup,
  common_thread_awake_after,
  common_thread_emit_wakeups,
  common_thread_set_wakeup,
  common_thread_abort,
  common_thread_queue_abort,
  common_thread_aborted,
  common_thread_get_aborted,
  common_thread_get_running,
  common_thread_wait_for_exit,
  common_thread_yield,
  pthread_exit,
  pth_thread_set_handle,
  pth_thread_get_handle,
  common_thread_info_collect,
  common_thread_info_free,
  common_thread_get_qdata,
  common_thread_set_qdata_full,
  common_thread_steal_qdata,
  pth_mutex_init,
  (void (*) (RapicornMutex*)) pthread_mutex_lock,
  (int  (*) (RapicornMutex*)) pthread_mutex_trylock,
  (void (*) (RapicornMutex*)) pthread_mutex_unlock,
  (void (*) (RapicornMutex*)) pthread_mutex_destroy,
  pth_rec_mutex_init,
  (void (*) (RapicornRecMutex*)) pthread_mutex_lock,
  (int  (*) (RapicornRecMutex*)) pthread_mutex_trylock,
  (void (*) (RapicornRecMutex*)) pthread_mutex_unlock,
  (void (*) (RapicornRecMutex*)) pthread_mutex_destroy,
  pth_cond_init,
  (void (*) (RapicornCond*))               pthread_cond_signal,
  (void (*) (RapicornCond*))               pthread_cond_broadcast,
  (void (*) (RapicornCond*, RapicornMutex*)) pthread_cond_wait,
  pth_cond_wait_timed,
  (void (*) (RapicornCond*))               pthread_cond_destroy,
};
static RapicornThreadTable*
get_pth_thread_table (void)
{
  if (pthread_key_create (&pth_thread_table_key, (void(*)(void*)) rapicorn_thread_handle_exit) != 0)
    {
      char buffer[1024];
      snprintf (buffer, 1024, "RapicornThread[%u]: failed to create pthread key, falling back to GLib threads.\n", getpid());
      fputs (buffer, stderr);
      return NULL;
    }
  return &pth_thread_table;
}
#else	/* !RAPICORN_HAVE_MUTEXATTR_SETTYPE */
#define	get_pth_thread_table()	NULL
#endif	/* !RAPICORN_HAVE_MUTEXATTR_SETTYPE */

/* ::Rapicorn::ThreadTable must be a RapicornThreadTable, not a reference for the C API wrapper to work */
RapicornThreadTable ThreadTable = {
  common_mutex_chain4init,
  common_mutex_unchain,
  common_rec_mutex_chain4init,
  common_rec_mutex_unchain,
  common_cond_chain4init,
  common_cond_unchain,
  (void (*) (volatile void*, volatile void*))                 g_atomic_pointer_set,
  (void*(*) (volatile void*))                                 g_atomic_pointer_get,
  (int  (*) (volatile void*, volatile void*, volatile void*)) g_atomic_pointer_compare_and_exchange,
  g_atomic_int_set,
  g_atomic_int_get,
  (int  (*) (volatile  int*,  int,  int)) g_atomic_int_compare_and_exchange,
  (void (*) (volatile  int*,  int))       g_atomic_int_add,
  (int  (*) (volatile  int*,  int))       g_atomic_int_exchange_and_add,
  (void (*) (volatile uint*, uint))       g_atomic_int_set,
  (uint (*) (volatile uint*))             g_atomic_int_get,
  (int  (*) (volatile uint*, uint, uint)) g_atomic_int_compare_and_exchange,
  (void (*) (volatile uint*, uint))       g_atomic_int_add,
  (uint (*) (volatile uint*, uint))       g_atomic_int_exchange_and_add,
  NULL,
};

void
_rapicorn_init_threads (void)
{
  RapicornThreadTable *table = get_pth_thread_table ();
  if (!table)
    table = get_fallback_thread_table ();
  ThreadTable = *table;
  
  while (mutex_init_chain)
    {
      RapicornMutex *mutex = mutex_init_chain;
      mutex_init_chain = (RapicornMutex*) mutex->mutex_pointer;
      ThreadTable.mutex_init (mutex);
    }
  while (rec_mutex_init_chain)
    {
      RapicornMutex *mutex = rec_mutex_init_chain;
      rec_mutex_init_chain = (RapicornMutex*) mutex->mutex_pointer;
      RAPICORN_STATIC_ASSERT (offsetof (RapicornRecMutex, mutex) == 0);
      ThreadTable.rec_mutex_init ((RapicornRecMutex*) mutex);
    }
  while (cond_init_chain)
    {
      RapicornCond *cond = cond_init_chain;
      cond_init_chain = (RapicornCond*) cond->cond_pointer;
      ThreadTable.cond_init (cond);
    }
  
  ThreadTable.thread_self ();
}

} // Rapicorn
