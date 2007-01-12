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

#include <birnet/birnetutils.hh>

namespace Birnet {

class Thread;

class Mutex {
  BirnetMutex mutex;
  friend class Cond;
  BIRNET_PRIVATE_CLASS_COPY (Mutex);
public:
  explicit      Mutex   ();
  void          lock    ()                      { ThreadTable.mutex_lock (&mutex); }
  void          unlock  ()                      { ThreadTable.mutex_unlock (&mutex); }
  bool          trylock ()                      { return 0 == ThreadTable.mutex_trylock (&mutex); /* TRUE indicates success */ }
  /*Des*/       ~Mutex  ();
};

class RecMutex {
  BirnetRecMutex rmutex;
  BIRNET_PRIVATE_CLASS_COPY (RecMutex);
public:
  explicit      RecMutex  ();
  void          lock      ()                    { ThreadTable.rec_mutex_lock (&rmutex); }
  void          unlock    ()                    { ThreadTable.rec_mutex_unlock (&rmutex); }
  bool          trylock   ()                    { return 0 == ThreadTable.rec_mutex_trylock (&rmutex); /* TRUE indicates success */ }
  /*Des*/       ~RecMutex ();
};

class Cond {
  BirnetCond cond;
  BIRNET_PRIVATE_CLASS_COPY (Cond);
public:
  explicit      Cond          ();
  void          signal        ()                { ThreadTable.cond_signal (&cond); }
  void          broadcast     ()                { ThreadTable.cond_broadcast (&cond); }
  void          wait          (Mutex &m)        { ThreadTable.cond_wait (&cond, &m.mutex); }
  void          wait_timed    (Mutex &m,
                               int64 max_usecs) { ThreadTable.cond_wait_timed (&cond, &m.mutex, max_usecs); }
  /*Des*/       ~Cond         ();
};

namespace Atomic {
/* atomic integers */
inline void    int_set       (volatile int  *iptr, int value)      { ThreadTable.atomic_int_set (iptr, value); }
inline int     int_get       (volatile int  *iptr)                 { return ThreadTable.atomic_int_get (iptr); }
inline bool    int_cas       (volatile int  *iptr, int o, int n)   { return ThreadTable.atomic_int_cas (iptr, o, n); }
inline void    int_add       (volatile int  *iptr, int diff)       { ThreadTable.atomic_int_add (iptr, diff); }
inline int     int_swap_add  (volatile int  *iptr, int diff)       { return ThreadTable.atomic_int_swap_add (iptr, diff); }
/* atomic unsigned integers */
inline void    uint_set      (volatile uint *uptr, uint value)     { ThreadTable.atomic_uint_set (uptr, value); }
inline uint    uint_get      (volatile uint *uptr)                 { return ThreadTable.atomic_uint_get (uptr); }
inline bool    uint_cas      (volatile uint *uptr, uint o, uint n) { return ThreadTable.atomic_uint_cas (uptr, o, n); }
inline void    uint_add      (volatile uint *uptr, uint diff)      { ThreadTable.atomic_uint_add (uptr, diff); }
inline uint    uint_swap_add (volatile uint *uptr, uint diff)      { return ThreadTable.atomic_uint_swap_add (uptr, diff); }
/* atomic pointers */
template<class V>
inline void    ptr_set       (volatile V **ptr_addr, V *n)      { ThreadTable.atomic_pointer_set ((void**) ptr_addr, (void*) n); }
template<class V>
inline V*      ptr_get       (volatile V **ptr_addr)            { return (V*) ThreadTable.atomic_pointer_get ((void**) ptr_addr); }
template<class V>
inline V*      ptr_get       (volatile V *const *ptr_addr)      { return (V*) ThreadTable.atomic_pointer_get ((void**) ptr_addr); }
template<class V>
inline bool    ptr_cas       (volatile V **ptr_adr, V *o, V *n) { return ThreadTable.atomic_pointer_cas ((void**) ptr_adr, (void*) o, (void*) n); }
};

class OwnedMutex {
  BirnetRecMutex   m_rec_mutex;
  volatile Thread *m_owner;
  BIRNET_PRIVATE_CLASS_COPY (OwnedMutex);
public:
  explicit       OwnedMutex ();
  inline void    lock       ();
  inline bool    trylock    ();
  inline void    unlock     ();
  inline Thread* owner      ();
  inline bool    mine       ();
  /*Des*/       ~OwnedMutex ();
};

class Thread : public virtual ReferenceCountImpl {
protected:
  explicit              Thread          (const String      &name);
  virtual void          run             () = 0;
  virtual               ~Thread         ();
public:
  void                  start           ();
  int                   pid             () const;
  String                name            () const;
  void                  queue_abort     ();
  void                  abort           ();
  bool                  aborted         ();
  void                  wakeup          ();
  bool                  running         ();
  void                  wait_for_exit   ();
  /* event loop */
  void                  exec_loop       ();
  void                  quit_loop       ();
  /* global methods */
  static void           emit_wakeups    (uint64             stamp);
  static Thread&        self            ();
  /* Self thread */
  struct Self {
    static String       name            ();
    static void         name            (const String      &name);
    static bool         sleep           (long               max_useconds);
    static bool         aborted         ();
    static int          pid             ();
    static void         awake_after     (uint64             stamp);
    static void         set_wakeup      (BirnetThreadWakeup wakeup_func,
                                         void              *wakeup_data,
                                         void             (*destroy_data) (void*));
    static OwnedMutex&  owned_mutex     ();
    static void         exit            (void              *retval = NULL) BIRNET_NORETURN;
  };
  /* DataListContainer API */
  template<typename Type> inline void set_data    (DataKey<Type> *key,
                                                   Type           data) { thread_lock(); data_list.set (key, data); thread_unlock(); }
  template<typename Type> inline Type get_data    (DataKey<Type> *key)  { thread_lock(); Type d = data_list.get (key); thread_unlock(); return d; }
  template<typename Type> inline Type swap_data   (DataKey<Type> *key)  { thread_lock(); Type d = data_list.swap (key); thread_unlock(); return d; }
  template<typename Type> inline Type swap_data   (DataKey<Type> *key,
                                                   Type           data) { thread_lock(); Type d = data_list.swap (key, data); thread_unlock(); return d; }
  template<typename Type> inline void delete_data (DataKey<Type> *key)  { thread_lock(); data_list.del (key); thread_unlock(); }
  /* implementaiton details */
private:
  DataList              data_list;
  BirnetThread         *bthread;
  OwnedMutex            m_omutex;
  explicit              Thread          (BirnetThread      *thread);
  void                  thread_lock     ()                              { m_omutex.lock(); }
  bool                  thread_trylock  ()                              { return m_omutex.trylock(); }
  void                  thread_unlock   ()                              { m_omutex.unlock(); }
  BIRNET_PRIVATE_CLASS_COPY (Thread);
protected:
  class ThreadWrapperInternal;
  static void threadxx_wrap   (BirnetThread *cthread);
  static void threadxx_delete (void         *cxxthread);
};

/**
 * The AutoLocker class locks mutex like objects on construction, and automatically
 * unlocks on destruction. So putting an AutoLocker object on the stack conveniently
 * ensures that a mutex will be automatically locked and properly unlocked when
 * the function returns or throws an exception.
 * Objects intended to be used by an AutoLocker need to provide the public methods
 * lock() and unlock().
 */
class AutoLocker {
  struct Locker {
    virtual void lock    () const = 0;
    virtual void unlock  () const = 0;
    virtual     ~Locker  () {}
  };
  template<class Lockable>
  struct LockerImpl : public Locker {
    Lockable    *lockable;
    virtual void lock       () const      { lockable->lock(); }
    virtual void unlock     () const      { lockable->unlock(); }
    virtual     ~LockerImpl () {}
    explicit     LockerImpl (Lockable *l) : lockable (l) {}
  };
  void         *space[2];
  volatile uint lcount;
  BIRNET_PRIVATE_CLASS_COPY (AutoLocker);
  inline const Locker*          locker      () const             { return static_cast<const Locker*> ((const void*) &space); }
protected:
  /* assert implicit assumption of the AutoLocker implementation */
  template<class Lockable> void
  assert_impl (Lockable &lockable)
  {
    BIRNET_ASSERT (sizeof (LockerImpl<Lockable>) <= sizeof (space));
    Locker *laddr = new (space) LockerImpl<Lockable> (&lockable);
    BIRNET_ASSERT (laddr == locker());
  }
public:
  template<class Lockable>      AutoLocker  (Lockable *lockable) : lcount (0) { new (space) LockerImpl<Lockable> (lockable); relock(); }
  template<class Lockable>      AutoLocker  (Lockable &lockable) : lcount (0) { new (space) LockerImpl<Lockable> (&lockable); relock(); }
  void                          relock      ()                                { locker()->lock(); lcount++; }
  void                          unlock      ()                                { BIRNET_ASSERT (lcount > 0); lcount--; locker()->unlock(); }
  /*Des*/                       ~AutoLocker ()                                { while (lcount) unlock(); }
};

/* --- implementation --- */
inline void
OwnedMutex::lock ()
{
  ThreadTable.rec_mutex_lock (&m_rec_mutex);
  Atomic::ptr_set (&m_owner, &Thread::self());
}

inline bool
OwnedMutex::trylock ()
{
  if (ThreadTable.rec_mutex_trylock (&m_rec_mutex) == 0)
    {
      Atomic::ptr_set (&m_owner, &Thread::self());
      return true; /* TRUE indicates success */
    }
  else
    return false;
}

inline void
OwnedMutex::unlock ()
{
  Atomic::ptr_set (&m_owner, (Thread*) 0);
  ThreadTable.rec_mutex_unlock (&m_rec_mutex);
}

inline Thread*
OwnedMutex::owner ()
{
  return Atomic::ptr_get (&m_owner);
}

inline bool
OwnedMutex::mine ()
{
  return Atomic::ptr_get (&m_owner) == &Thread::self();
}

} // Birnet

#endif /* __BIRNET_THREAD_XX_HH__ */

/* vim:set ts=8 sts=2 sw=2: */
