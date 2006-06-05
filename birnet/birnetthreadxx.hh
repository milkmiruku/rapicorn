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
#ifndef __BIRNET_THREAD_XX_HH__
#define __BIRNET_THREAD_XX_HH__

#include <birnet/birnetthread.h>
#include <birnet/birnetutilsxx.hh>

namespace Birnet {

class Mutex {
  BirnetMutex mutex;
  friend class Cond;
  BIRNET_PRIVATE_CLASS_COPY (Mutex);
public:
  explicit      Mutex   ()                      { birnet_thread_table.mutex_init (&mutex); }
  void          lock    ()                      { birnet_thread_table.mutex_lock (&mutex); }
  void          unlock  ()                      { birnet_thread_table.mutex_unlock (&mutex); }
  bool          trylock ()                      { return 0 == birnet_thread_table.mutex_trylock (&mutex); /* TRUE indicates success */ }
  /*Des*/       ~Mutex  ()                      { birnet_thread_table.mutex_destroy (&mutex); }
};

class RecMutex {
  BirnetRecMutex rmutex;
  BIRNET_PRIVATE_CLASS_COPY (RecMutex);
public:
  explicit      RecMutex  ()                    { birnet_thread_table.rec_mutex_init (&rmutex); }
  void          lock      ()                    { birnet_thread_table.rec_mutex_lock (&rmutex); }
  void          unlock    ()                    { birnet_thread_table.rec_mutex_unlock (&rmutex); }
  bool          trylock   ()                    { return 0 == birnet_thread_table.rec_mutex_trylock (&rmutex); /* TRUE indicates success */ }
  /*Des*/       ~RecMutex ()                    { birnet_thread_table.rec_mutex_destroy (&rmutex); }
};

class Cond {
  BirnetCond cond;
  BIRNET_PRIVATE_CLASS_COPY (Cond);
public:
  explicit      Cond          ()                { birnet_thread_table.cond_init (&cond); }
  void          signal        ()                { birnet_thread_table.cond_signal (&cond); }
  void          broadcast     ()                { birnet_thread_table.cond_broadcast (&cond); }
  void          wait          (Mutex &m)        { birnet_thread_table.cond_wait (&cond, &m.mutex); }
  void          wait_timed    (Mutex &m,
                               int64 max_usecs) { birnet_cond_wait_timed (&cond, &m.mutex, max_usecs); }
  /*Des*/       ~Cond         ()                { birnet_thread_table.cond_destroy (&cond); }
};

/**
 * The AutoLocker class locks a mutex on construction, and automatically
 * unlocks it on destruction, so that putting an AutoLocker object on
 * the stack conveniently ensures that the mutex will be properly unlocked
 * for instance when the function returns or an exception gets thrown.
 */
class AutoLocker {
  union {
    Mutex    *m_mutex;
    RecMutex *m_rec_mutex;
  };
  const bool m_recursive;
  BIRNET_PRIVATE_CLASS_COPY (AutoLocker);
public:
  AutoLocker (Mutex &mutex) :
    m_recursive (false)
  {
    m_mutex = &mutex;
    relock();
  }
  AutoLocker (Mutex *mutex) :
    m_recursive (false)
  {
    BIRNET_ASSERT (mutex != NULL);
    m_mutex = mutex;
    relock();
  }
  AutoLocker (RecMutex &mutex) :
    m_recursive (true)
  {
    m_rec_mutex = &mutex;
    relock();
  }
  AutoLocker (RecMutex *rec_mutex) :
    m_recursive (true)
  {
    BIRNET_ASSERT (rec_mutex != NULL);
    m_rec_mutex = rec_mutex;
    relock();
  }
  void		relock	      () const		    { if (m_recursive) m_rec_mutex->lock(); else m_mutex->lock(); }
  void		unlock	      () const		    { if (m_recursive) m_rec_mutex->unlock(); else m_mutex->unlock(); }
  /*Des*/	~AutoLocker   ()		    { unlock(); }
};

namespace Atomic {
/* atomic integers */
inline void     int_set      (volatile int *iptr, int value)    { birnet_atomic_int_set (iptr, value); }
inline int      int_get      (volatile int *iptr)               { return birnet_atomic_int_get (iptr); }
inline bool     int_cas      (volatile int *iptr, int o, int n) { return birnet_atomic_int_compare_and_swap (iptr, o, n); }
inline void     int_add      (volatile int *iptr, int diff)     { birnet_atomic_int_add (iptr, diff); }
inline int      int_swap_add (volatile int *iptr, int diff)     { return birnet_atomic_int_swap_and_add (iptr, diff); }
/* atomic pointers */
template<class V>
inline void     ptr_set      (volatile V **ptr_addr, V *n)      { birnet_atomic_set (void*, (void**) ptr_addr, (void*) n); }
template<class V>
inline V*       ptr_get      (volatile V **ptr_addr)            { return birnet_atomic_get (void*, (void**) ptr_addr); }
template<class V>
inline bool     ptr_cas      (volatile V **ptr_adr, V *o, V *n) { return birnet_atomic_compare_and_swap (void*, (void**) ptr_adr, (void*) o, (void*) n); }
};

class Thread : public virtual ReferenceCountImpl {
  static void           trampoline      (void                   *thread_data);
  BIRNET_PRIVATE_CLASS_COPY (Thread);
  explicit              Thread          (BirnetThread           *thread);
protected:
  BirnetThread         *bthread;
  static Thread*        thread_from_c   (BirnetThread           *bthread);
  virtual void          run             () = 0;
public:
  explicit              Thread          (const String           &name);
  void                  start           ();
  int                   pid             () const;
  String                name            () const;
  void                  queue_abort     ();
  void                  abort           ();
  bool                  aborted         ();
  void                  wakeup          ();
  bool                  running         ();
  void                  wait_for_exit   ();
  virtual               ~Thread         ();
  /* global method */
  static void           emit_wakeups    (uint64                  stamp);
  static Thread&        self            ();
  /* self thread */
  struct Self {
    static String       name            ();
    static void         name            (const String           &name);
    static bool         sleep           (long                    max_useconds);
    static bool         aborted         ();
    static int          pid             ();
    static void         awake_after     (uint64                  stamp);
    static void         set_wakeup      (BirnetThreadWakeup      wakeup_func,
                                         void                   *wakeup_data,
                                         GDestroyNotify          destroy);
  };
  /* event loop */
  void                  exec_loop       ();
  void                  quit_loop       ();
#if 0 // FIXME
  /* mimick DataListContainer */
private:
  DataList data_list;
  void     thread_lock     ();
  bool     thread_trylock  ();
  void     thread_unlock   ();
public: /* generic data API */
  template<typename Type> inline void set_data    (DataKey<Type> *key,
                                                   Type           data) { thread_lock(); data_list.set (key, data); thread_unlock(); }
  template<typename Type> inline Type get_data    (DataKey<Type> *key)  { thread_lock(); Type d = data_list.get (key); thread_unlock(); return d; }
  template<typename Type> inline Type swap_data   (DataKey<Type> *key)  { thread_lock(); Type d = data_list.swap (key); thread_unlock(); return d; }
  template<typename Type> inline Type swap_data   (DataKey<Type> *key,
                                                   Type           data) { thread_lock(); Type d = data_list.swap (key, data); thread_unlock(); return d; }
  template<typename Type> inline void delete_data (DataKey<Type> *key)  { thread_lock(); data_list.del (key); thread_unlock(); }
#endif
};

} // Birnet

#endif /* __BIRNET_THREAD_XX_HH__ */

/* vim:set ts=8 sts=2 sw=2: */
