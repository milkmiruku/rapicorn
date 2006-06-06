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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
// #define TEST_VERBOSE
#include <birnet/birnettests.h>

namespace {
using namespace Birnet;

static volatile guint atomic_count = 0;
static BirnetMutex    atomic_mutex;
static BirnetCond     atomic_cond;

static void
atomic_up_thread (gpointer data)
{
  volatile int *ip = (int*) data;
  for (guint i = 0; i < 25; i++)
    birnet_atomic_int_add (ip, +3);
  birnet_mutex_lock (&atomic_mutex);
  atomic_count -= 1;
  birnet_cond_signal (&atomic_cond);
  birnet_mutex_unlock (&atomic_mutex);
  TASSERT (strcmp (birnet_thread_get_name (birnet_thread_self()), "AtomicTest") == 0);
}

static void
atomic_down_thread (gpointer data)
{
  volatile int *ip = (int*) data;
  for (guint i = 0; i < 25; i++)
    birnet_atomic_int_add (ip, -4);
  birnet_mutex_lock (&atomic_mutex);
  atomic_count -= 1;
  birnet_cond_signal (&atomic_cond);
  birnet_mutex_unlock (&atomic_mutex);
  TASSERT (strcmp (birnet_thread_get_name (birnet_thread_self()), "AtomicTest") == 0);
}

static void
test_atomic (void)
{
  TSTART ("AtomicThreading");
  int count = 60;
  BirnetThread *threads[count];
  volatile int atomic_counter = 0;
  birnet_mutex_init (&atomic_mutex);
  birnet_cond_init (&atomic_cond);
  atomic_count = count;
  for (int i = 0; i < count; i++)
    {
      threads[i] = birnet_thread_run ("AtomicTest", (i&1) ? atomic_up_thread : atomic_down_thread, (void*) &atomic_counter);
      TASSERT (threads[i]);
    }
  birnet_mutex_lock (&atomic_mutex);
  while (atomic_count > 0)
    {
      TACK();
      birnet_cond_wait (&atomic_cond, &atomic_mutex);
    }
  birnet_mutex_unlock (&atomic_mutex);
  int result = count / 2 * 25 * +3 + count / 2 * 25 * -4;
  // g_printerr ("{ %d ?= %d }", atomic_counter, result);
  for (int i = 0; i < count; i++)
    birnet_thread_unref (threads[i]);
  TASSERT (atomic_counter == result);
  TDONE ();
}

static void
plus1_thread (gpointer data)
{
  guint *tdata = (guint*) data;
  birnet_thread_sleep (-1);
  *tdata += 1;
  while (!birnet_thread_aborted ())
    birnet_thread_sleep (-1);
}

static void
test_threads (void)
{
  static BirnetMutex test_mutex;
  gboolean locked;
  TSTART ("Threading");
  birnet_mutex_init (&test_mutex);
  locked = birnet_mutex_trylock (&test_mutex);
  TASSERT (locked);
  locked = birnet_mutex_trylock (&test_mutex);
  TASSERT (!locked);
  birnet_mutex_unlock (&test_mutex);
  birnet_mutex_destroy (&test_mutex);
  guint thread_data1 = 0;
  BirnetThread *thread1 = birnet_thread_run ("plus1", plus1_thread, &thread_data1);
  guint thread_data2 = 0;
  BirnetThread *thread2 = birnet_thread_run ("plus2", plus1_thread, &thread_data2);
  guint thread_data3 = 0;
  BirnetThread *thread3 = birnet_thread_run ("plus3", plus1_thread, &thread_data3);
  TASSERT (thread1 != NULL);
  TASSERT (thread2 != NULL);
  TASSERT (thread3 != NULL);
  TASSERT (thread_data1 == 0);
  TASSERT (thread_data2 == 0);
  TASSERT (thread_data3 == 0);
  TASSERT (birnet_thread_get_running (thread1) == TRUE);
  TASSERT (birnet_thread_get_running (thread2) == TRUE);
  TASSERT (birnet_thread_get_running (thread3) == TRUE);
  birnet_thread_wakeup (thread1);
  birnet_thread_wakeup (thread2);
  birnet_thread_wakeup (thread3);
  birnet_thread_abort (thread1);
  birnet_thread_abort (thread2);
  birnet_thread_abort (thread3);
  TASSERT (thread_data1 > 0);
  TASSERT (thread_data2 > 0);
  TASSERT (thread_data3 > 0);
  birnet_thread_unref (thread1);
  birnet_thread_unref (thread2);
  birnet_thread_unref (thread3);
  TDONE ();
}

struct ThreadA : public virtual Birnet::Thread {
  int value;
  volatile int *counter;
  ThreadA (volatile int *counterp,
           int           v) :
    Thread ("ThreadA"),
    value (v), counter (counterp)
  {}
  virtual void
  run ()
  {
    TASSERT (this->name() == "ThreadA");
    TASSERT (this->name() == Thread::Self::name());
    for (int j = 0; j < 17905; j++)
      birnet_atomic_int_add (counter, value);
  }
};

static void
test_thread_cxx (void)
{
  TSTART ("C++Threading");
  volatile int atomic_counter = 0;
  int result = 0;
  int count = 60;
  Birnet::Thread *threads[count];
  for (int i = 0; i < count; i++)
    {
      int v = rand();
      for (int j = 0; j < 17905; j++)
        result += v;
      threads[i] = new ThreadA (&atomic_counter, v);
      TASSERT (threads[i]);
      ref_sink (threads[i]);
    }
  TASSERT (atomic_counter == 0);
  for (int i = 0; i < count; i++)
    threads[i]->start();
  for (int i = 0; i < count; i++)
    {
      threads[i]->wait_for_exit();
      unref (threads[i]);
    }
  TASSERT (atomic_counter == result);
  TDONE ();
}

template<class M> static bool
lockable (M &mutex)
{
  bool lockable = mutex.trylock();
  if (lockable)
    mutex.unlock();
  return lockable;
}

static void
test_simple_auto_lock (Mutex &mutex1,
                       Mutex &mutex2)
{
  TASSERT (lockable (mutex1) == true);
  TASSERT (lockable (mutex2) == true);

  AutoLocker locker1 (mutex1);

  TASSERT (lockable (mutex1) == false);
  TASSERT (lockable (mutex2) == true);

  AutoLocker locker2 (&mutex2);

  TASSERT (lockable (mutex1) == false);
  TASSERT (lockable (mutex2) == false);
}

static void
test_recursive_auto_lock (RecMutex &rec_mutex,
                          guint     depth)
{
  AutoLocker locker (rec_mutex);
  if (depth > 1)
    test_recursive_auto_lock (rec_mutex, depth - 1);
  else
    {
      locker.relock();
      locker.relock();
      locker.relock();
      bool lockable1 = rec_mutex.trylock();
      bool lockable2 = rec_mutex.trylock();
      TASSERT (lockable1 && lockable2);
      rec_mutex.unlock();
      rec_mutex.unlock();
      locker.unlock();
      locker.unlock();
      locker.unlock();
    }
}

// helper class for testing auto locking, which counts the lock() and unlock() calls
class LockCounter {
  guint m_lock_count;
public:
  LockCounter() :
    m_lock_count (0)
  {
  }
  void
  lock()
  {
    m_lock_count++;
  }
  void
  unlock()
  {
    TASSERT (m_lock_count > 0);
    m_lock_count--;
  }
  guint
  lock_count() const
  {
    return m_lock_count;
  }
};

class LockCountAssert {
  const LockCounter &m_lock_counter;
  const guint        m_required_lock_count;
public:
  LockCountAssert (const LockCounter& lock_counter,
		   guint              required_lock_count) :
    m_lock_counter (lock_counter),
    m_required_lock_count (required_lock_count)
  {
    TASSERT (m_lock_counter.lock_count() == m_required_lock_count);
  }
  ~LockCountAssert()
  {
    TASSERT (m_lock_counter.lock_count() == m_required_lock_count);
  }
};

/* Check that C++ constructors and destructors and the AutoLocker constructor
 * and destructor will be executed in the order we need, that is: an AutoLocker
 * that is created before an object should protect its constructor and
 * destructor, an AutoLocker created after an object should not affect its
 * constructor and destructor.
 */
static void
test_auto_locker_order()
{
  LockCounter lock_counter1;
  LockCounter lock_counter2;

  for (guint i = 0; i < 3; i++)
    {
      LockCountAssert lc_assert1 (lock_counter1, 0);
      LockCountAssert lc_assert2 (lock_counter2, 0);

      AutoLocker      auto_locker1 (lock_counter1);

      LockCountAssert lc_assert3 (lock_counter1, 1);
      LockCountAssert lc_assert4 (lock_counter2, 0);

      AutoLocker      auto_locker2 (lock_counter2);

      LockCountAssert lc_assert5 (lock_counter1, 1);
      LockCountAssert lc_assert6 (lock_counter2, 1);

      AutoLocker      auto_locker3 (lock_counter1);

      LockCountAssert lc_assert7 (lock_counter1, 2);
      LockCountAssert lc_assert8 (lock_counter2, 1);
    }
}

static void
test_auto_locker_cxx()
{
  TSTART ("C++AutoLocker");
  if (true)
    {
      struct CheckAutoLocker : public AutoLocker {
        CheckAutoLocker (Mutex &dummy) :
          AutoLocker (dummy)
        {}
        void
        assert_auto_locker_impl (Mutex &dummy)
        {
          assert_impl (dummy);
        }
      };
      Mutex dummy;
      CheckAutoLocker check_auto_locker (dummy);
      check_auto_locker.assert_auto_locker_impl (dummy);
      TICK();
    }
  Mutex mutex1, mutex2;
  RecMutex rec_mutex;

  TASSERT (lockable (mutex1) == true);
  TASSERT (lockable (mutex2) == true);
  test_simple_auto_lock (mutex1, mutex2);
  test_simple_auto_lock (mutex1, mutex2);
  TASSERT (lockable (mutex1) == true);
  TASSERT (lockable (mutex2) == true);

  test_recursive_auto_lock (rec_mutex, 30);
  AutoLocker locker (&rec_mutex);
  test_recursive_auto_lock (rec_mutex, 17);

  test_auto_locker_order();
  TDONE();
}

#define RUNS (500000)

class HeapLocker {
  struct Lockable {
    virtual void lock() = 0;
    virtual void unlock() = 0;
  };
  template<class L>
  struct Wrapper : public virtual Lockable {
    L &l;
    explicit     Wrapper  (L &o) : l (o) {}
    virtual void lock     () { l.lock(); }
    virtual void unlock   () { l.unlock(); }
  };
  Lockable &l;
public:
  template<class C>
  HeapLocker (C &c) :
    l (*new Wrapper<C> (c))
  {
    lock();
  }
  ~HeapLocker()
  {
    unlock();
    delete &l;
  }
  void lock     () { l.lock(); }
  void unlock   () { l.unlock(); }
};

static void
bench_heap_auto_locker()
{
  Mutex mutex;
  RecMutex rmutex;
  for (uint i = 0; i < RUNS; i++)
    {
      HeapLocker locker1 (mutex);
      HeapLocker locker2 (rmutex);
    }
}

static void
bench_direct_auto_locker()
{
  class AutoLocker2 {
    union {
      Mutex    *m_mutex;
      RecMutex *m_rec_mutex;
    };
    const bool m_recursive;
    BIRNET_PRIVATE_CLASS_COPY (AutoLocker2);
  public:
    AutoLocker2 (Mutex &mutex) :
      m_recursive (false)
    {
      m_mutex = &mutex;
      relock();
    }
    AutoLocker2 (RecMutex &mutex) :
      m_recursive (true)
    {
      m_rec_mutex = &mutex;
      relock();
    }
    AutoLocker2 (RecMutex *rec_mutex) :
      m_recursive (true) {
      BIRNET_ASSERT (rec_mutex != NULL);
      m_rec_mutex = rec_mutex;
      relock();
    }
    void          relock        () const { if (m_recursive) m_rec_mutex->lock(); else m_mutex->lock(); }
    void          unlock        () const { if (m_recursive) m_rec_mutex->unlock(); else m_mutex->unlock(); }
    /*Des*/       ~AutoLocker2   () { unlock(); }
  };
  Mutex mutex;
  RecMutex rmutex;
  for (uint i = 0; i < RUNS; i++)
    {
      AutoLocker2 locker1 (mutex);
      AutoLocker2 locker2 (rmutex);
    }
}

class PtrAutoLocker {
  struct Locker {
    virtual void lock   () = 0;
    virtual void unlock () = 0;
  };
  template<class Lockable>
  struct LockerImpl : public Locker {
    Lockable    *lockable;
    virtual void lock       () { lockable->lock(); }
    virtual void unlock     () { lockable->unlock(); }
    explicit     LockerImpl (Lockable *l) : lockable (l) {}
  };
  void  *space[2];
  Locker *locker;
  BIRNET_PRIVATE_CLASS_COPY (PtrAutoLocker);
public:
  template<class Lockable>      PtrAutoLocker  (Lockable *lockable) { locker = new (space) LockerImpl<Lockable> (lockable); relock(); }
  template<class Lockable>      PtrAutoLocker  (Lockable &lockable) { locker = new (space) LockerImpl<Lockable> (&lockable); relock(); }
  void                          relock         () { locker->lock(); }
  void                          unlock         () { locker->unlock(); }
  /*Des*/                       ~PtrAutoLocker () { unlock(); }
};


static void
bench_ptr_auto_locker()
{
  Mutex mutex;
  RecMutex rmutex;
  for (uint i = 0; i < RUNS; i++)
    {
      PtrAutoLocker locker1 (mutex);
      PtrAutoLocker locker2 (rmutex);
    }
}

static void
bench_birnet_auto_locker()
{
  Mutex mutex;
  RecMutex rmutex;
  for (uint i = 0; i < RUNS; i++)
    {
      AutoLocker locker1 (mutex);
      AutoLocker locker2 (rmutex);
    }
}

static void
bench_manual_locker()
{
  Mutex mutex;
  RecMutex rmutex;
  for (uint i = 0; i < RUNS; i++)
    {
      mutex.lock();
      rmutex.lock();
      mutex.unlock();
      rmutex.unlock();
    }
}

static void
bench_auto_locker_cxx()
{
  TSTART ("C++AutoLocker-Benchmark");
  GTimer *timer = g_timer_new();
  const guint n_repeatitions = 7;
  TICK();
  /* bench manual locker */
  guint xdups = TEST_CALIBRATION (35.0, bench_manual_locker());
  double xmin = 9e300;
  for (guint i = 0; i < n_repeatitions; i++)
    {
      g_timer_start (timer);
      for (guint j = 0; j < xdups; j++)
        bench_manual_locker();
      g_timer_stop (timer);
      double e = g_timer_elapsed (timer, NULL);
      if (e < xmin)
        xmin = e;
      TICK();
    }
  TACK();
  /* bench direct auto locker */
  guint sdups = TEST_CALIBRATION (60.0, bench_direct_auto_locker());
  double smin = 9e300;
  for (guint i = 0; i < n_repeatitions; i++)
    {
      g_timer_start (timer);
      for (guint j = 0; j < sdups; j++)
        bench_direct_auto_locker();
      g_timer_stop (timer);
      double e = g_timer_elapsed (timer, NULL);
      if (e < smin)
        smin = e;
      TICK();
    }
  TACK();
  /* bench birnet auto locker */
  guint bdups = TEST_CALIBRATION (60.0, bench_birnet_auto_locker());
  double bmin = 9e300;
  for (guint i = 0; i < n_repeatitions; i++)
    {
      g_timer_start (timer);
      for (guint j = 0; j < bdups; j++)
        bench_birnet_auto_locker();
      g_timer_stop (timer);
      double e = g_timer_elapsed (timer, NULL);
      if (e < bmin)
        bmin = e;
      TICK();
    }
  TACK();
  /* bench ptr auto locker */
  guint pdups = TEST_CALIBRATION (60.0, bench_ptr_auto_locker());
  double pmin = 9e300;
  for (guint i = 0; i < n_repeatitions; i++)
    {
      g_timer_start (timer);
      for (guint j = 0; j < pdups; j++)
        bench_ptr_auto_locker();
      g_timer_stop (timer);
      double e = g_timer_elapsed (timer, NULL);
      if (e < pmin)
        pmin = e;
      TICK();
    }
  TACK();
  /* bench heap auto locker */
  guint tdups = TEST_CALIBRATION (60.0, bench_heap_auto_locker());
  double tmin = 9e300;
  for (guint i = 0; i < n_repeatitions; i++)
    {
      g_timer_start (timer);
      for (guint j = 0; j < tdups; j++)
        bench_heap_auto_locker();
      g_timer_stop (timer);
      double e = g_timer_elapsed (timer, NULL);
      if (e < tmin)
        tmin = e;
      TICK();
    }
  TACK();
  /* done, report */
  TDONE();
  g_print ("Manual-Locker:      %7.3f nsecs\n", xmin / xdups / RUNS * 1000. * 1000. * 1000.);
  g_print ("Direct-AutoLocker:  %7.3f nsecs\n", smin / sdups / RUNS * 1000. * 1000. * 1000.);
  g_print ("Birnet-AutoLocker:  %7.3f nsecs\n", bmin / bdups / RUNS * 1000. * 1000. * 1000.);
  g_print ("Pointer-AutoLocker: %7.3f nsecs\n", pmin / pdups / RUNS * 1000. * 1000. * 1000.);
  g_print ("Heap-AutoLocker:    %7.3f nsecs\n", tmin / tdups / RUNS * 1000. * 1000. * 1000.);
}

static void
test_thread_atomic_cxx (void)
{
  TSTART ("C++AtomicThreading");
  /* integer functions */
  volatile int ai, r;
  Atomic::int_set (&ai, 17);
  TASSERT (ai == 17);
  r = Atomic::int_get (&ai);
  TASSERT (r == 17);
  Atomic::int_add (&ai, 9);
  r = Atomic::int_get (&ai);
  TASSERT (r == 26);
  Atomic::int_set (&ai, -1147483648);
  TASSERT (ai == -1147483648);
  r = Atomic::int_get (&ai);
  TASSERT (r == -1147483648);
  Atomic::int_add (&ai, 9);
  r = Atomic::int_get (&ai);
  TASSERT (r == -1147483639);
  Atomic::int_add (&ai, -20);
  r = Atomic::int_get (&ai);
  TASSERT (r == -1147483659);
  r = Atomic::int_cas (&ai, 17, 19);
  TASSERT (r == false);
  r = Atomic::int_get (&ai);
  TASSERT (r == -1147483659);
  r = Atomic::int_cas (&ai, -1147483659, 19);
  TASSERT (r == true);
  r = Atomic::int_get (&ai);
  TASSERT (r == 19);
  r = Atomic::int_swap_add (&ai, 1);
  TASSERT (r == 19);
  r = Atomic::int_get (&ai);
  TASSERT (r == 20);
  r = Atomic::int_swap_add (&ai, -20);
  TASSERT (r == 20);
  r = Atomic::int_get (&ai);
  TASSERT (r == 0);
  /* pointer functions */
  volatile void *ap, *p;
  Atomic::ptr_set (&ap, (void*) 119);
  TASSERT (ap == (void*) 119);
  p = Atomic::ptr_get (&ap);
  TASSERT (p == (void*) 119);
  r = Atomic::ptr_cas (&ap, (void*) 17, (void*) -42);
  TASSERT (r == false);
  p = Atomic::ptr_get (&ap);
  TASSERT (p == (void*) 119);
  r = Atomic::ptr_cas (&ap, (void*) 119, (void*) 4294967279U);
  TASSERT (r == true);
  p = Atomic::ptr_get (&ap);
  TASSERT (p == (void*) 4294967279U);
  TDONE ();
}

} // Anon

int
main (int   argc,
      char *argv[])
{
  birnet_init_test (&argc, &argv);

  test_threads();
  test_atomic();
  test_thread_cxx();
  test_thread_atomic_cxx();
  test_auto_locker_cxx();
  bench_auto_locker_cxx();
  
  return 0;
}

/* vim:set ts=8 sts=2 sw=2: */
