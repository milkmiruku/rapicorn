// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include <rcore/testutils.hh>
#include <stdlib.h>
#include <string.h>
#include <thread>

namespace {
using namespace Rapicorn;

// == atomicity tests ==
template<typename V> static void
atomic_counter_func (Atomic<V> &ai, int niters, V d)
{
  if (d < 0)
    for (int i = 0; i < niters; i++)
      ai -= -d;
  else
    for (int i = 0; i < niters; i++)
      ai += d;
}

template<typename V> static void
test_atomic_counter (const int nthreads, int niters, V a, V b)
{
  assert (0 == (nthreads & 1));
  std::thread threads[nthreads];
  Atomic<V> atomic_counter = 0;
  for (int i = 0; i < nthreads; i++)
    {
      if (i & 1)
        threads[i] = std::thread (atomic_counter_func<V>, std::ref (atomic_counter), niters, a);
      else
        threads[i] = std::thread (atomic_counter_func<V>, std::ref (atomic_counter), niters, b);
      TASSERT (threads[i].joinable());
    }
  for (int i = 0; i < nthreads; i++)
    threads[i].join();
  const int64 result = nthreads / 2 * niters * a + nthreads / 2 * niters * b;
  const int64 atomic_result = V (atomic_counter);
  TCMP (atomic_result, ==, result);
}

static void
test_atomic ()
{
  test_atomic_counter<char>  (4, 3, +4, -3);
  test_atomic_counter<int8>  (6, 5, +2, -1);
  test_atomic_counter<uint8> (6, 5, +2, +1);
  test_atomic_counter<int>   (44, 25, +5, -2);
  test_atomic_counter<uint>  (44, 25, 1, 6);
  test_atomic_counter<int64> (52, 125, -50, +37);
  test_atomic_counter<int64> (52, 125, +42, +77);
  test_atomic_counter<__int128> (68, 12500, +4200, +77000);
}
REGISTER_TEST ("Threads/Atomic Operations", test_atomic);

// == runonce tests ==
static Atomic<uint>    runonce_threadcount = 0;
static Mutex           runonce_mutex;
static Cond            runonce_cond;
static volatile size_t runonce_value = 0;

static void
runonce_thread (Atomic<uint> &runonce_counter)
{
  runonce_mutex.lock(); // syncronize
  runonce_mutex.unlock();
  if (once_enter (&runonce_value))
    {
      runonce_mutex.lock();
      runonce_cond.broadcast();
      runonce_mutex.unlock();
      usleep (1); // sched_yield replacement to force contention
      ++runonce_counter;
      usleep (500); // sched_yield replacement to force contention
      once_leave (&runonce_value, size_t (42));
    }
  TCMP (runonce_counter, ==, 1);
  TCMP (runonce_value, ==, 42);
  /* sinal thread end */
  --runonce_threadcount;
  runonce_mutex.lock();
  runonce_cond.signal();
  runonce_mutex.unlock();
}

static void
test_runonce()
{
  const int count = 44;
  std::thread threads[count];
  Atomic<uint> runonce_counter = 0;
  runonce_threadcount.store (count);
  runonce_mutex.lock();
  for (int i = 0; i < count; i++)
    {
      threads[i] = std::thread (runonce_thread, std::ref (runonce_counter));
      TASSERT (threads[i].joinable());
    }
  TCMP (runonce_value, ==, 0);
  runonce_mutex.unlock(); // syncronized thread start
  runonce_mutex.lock();
  while (runonce_threadcount.load() > 0)
    {
      TOK();
      runonce_cond.wait (runonce_mutex);
    }
  runonce_mutex.unlock();
  for (int i = 0; i < count; i++)
    threads[i].join();
  TCMP (runonce_counter, ==, 1);
  TCMP (runonce_value, ==, 42);
}
REGISTER_TEST ("Threads/RunOnceTest", test_runonce);

#if 0 // FIXME

/* --- basic threading tests --- */
class Thread_Plus1 : public Thread {
  void *data;
  virtual void
  run ()
  {
    uint *tdata = (uint*) data;
    Thread::Self::sleep (-1);
    *tdata += 1;
    while (!Thread::Self::aborted ())
      Thread::Self::sleep (-1);
  }
public:
  Thread_Plus1 (const String &name, void *udata) : Thread (name), data (udata) {}
};

static Mutex    static_mutex;
static RecMutex static_rec_mutex;
static Cond     static_cond;

static void
test_threads (void)
{
  static Mutex test_mutex;
  bool locked;
  /* test C mutex */
  locked = test_mutex.try_lock();
  TASSERT (locked);
  locked = test_mutex.try_lock();
  TASSERT (!locked);
  test_mutex.unlock();
  /* not initializing static_mutex */
  locked = static_mutex.try_lock();
  TASSERT (locked);
  locked = static_mutex.try_lock();
  TASSERT (!locked);
  static_mutex.unlock();
  locked = static_mutex.try_lock();
  TASSERT (locked);
  static_mutex.unlock();
  /* not initializing static_rec_mutex */
  locked = static_rec_mutex.try_lock();
  TASSERT (locked);
  static_rec_mutex.lock();
  locked = static_rec_mutex.try_lock();
  TASSERT (locked);
  static_rec_mutex.unlock();
  static_rec_mutex.unlock();
  static_rec_mutex.unlock();
  locked = static_rec_mutex.try_lock();
  TASSERT (locked);
  static_rec_mutex.unlock();
  /* not initializing static_cond */
  static_cond.signal();
  static_cond.broadcast();
  /* test C++ mutex */
  static Mutex mutex;
  static RecMutex rmutex;
  mutex.lock();
  rmutex.lock();
  mutex.unlock();
  rmutex.unlock();
  uint thread_data1 = 0;
  Thread *thread1 = new Thread_Plus1 ("plus1", &thread_data1);
  ref_sink (thread1);
  thread1->start();
  uint thread_data2 = 0;
  Thread *thread2 = new Thread_Plus1 ("plus2", &thread_data2);
  ref_sink (thread2);
  thread2->start();
  uint thread_data3 = 0;
  Thread *thread3 = new Thread_Plus1 ("plus3", &thread_data3);
  ref_sink (thread3);
  thread3->start();
  TCMP (thread1, !=, nullptr);
  TCMP (thread2, !=, nullptr);
  TCMP (thread3, !=, nullptr);
  TCMP (thread_data1, ==, 0);
  TCMP (thread_data2, ==, 0);
  TCMP (thread_data3, ==, 0);
  TCMP (thread1->running(), ==, TRUE);
  TCMP (thread2->running(), ==, TRUE);
  TCMP (thread3->running(), ==, TRUE);
  thread1->abort();
  thread2->abort();
  thread3->abort();
  TCMP (thread_data1, >, 0);
  TCMP (thread_data2, >, 0);
  TCMP (thread_data3, >, 0);
  unref (thread1);
  unref (thread2);
  unref (thread3);
}
REGISTER_TEST ("Threads/Threading", test_threads);

/* --- C++ threading tests --- */
struct ThreadA : public virtual Rapicorn::Thread {
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
    TCMP (this->name(), ==, "ThreadA");
    TCMP (this->name(), ==, Thread::Self::name());
    for (int j = 0; j < 17905; j++)
      Atomic0::add (counter, value);
  }
};

template<class M> static bool
lockable (M &mutex)
{
  bool lockable = mutex.try_lock();
  if (lockable)
    mutex.unlock();
  return lockable;
}

static void
test_thread_cxx (void)
{
  TCMP (nullptr, !=, &Thread::self());
  volatile int atomic_counter = 0;
  int result = 0;
  int count = 35;
  Rapicorn::Thread *threads[count];
  for (int i = 0; i < count; i++)
    {
      int v = rand();
      for (int j = 0; j < 17905; j++)
        result += v;
      threads[i] = new ThreadA (&atomic_counter, v);
      TASSERT (threads[i]);
      ref_sink (threads[i]);
    }
  TCMP (atomic_counter, ==, 0);
  for (int i = 0; i < count; i++)
    threads[i]->start();
  for (int i = 0; i < count; i++)
    {
      threads[i]->wait_for_exit();
      unref (threads[i]);
    }
  TCMP (atomic_counter, ==, result);
  TDONE ();

  TSTART ("Threads/C++OwnedMutex");
  static OwnedMutex static_omutex;
  TCMP (static_omutex.mine(), ==, false);
  static_omutex.lock();
  TCMP (static_omutex.mine(), ==, true);
  static_omutex.unlock();
  TCMP (static_omutex.mine(), ==, false);
  static_omutex.lock();
  TCMP (static_omutex.mine(), ==, true);
  static_omutex.lock();
  TCMP (static_omutex.mine(), ==, true);
  static_omutex.unlock();
  TCMP (static_omutex.mine(), ==, true);
  static_omutex.unlock();
  TCMP (static_omutex.mine(), ==, false);
  TCMP (nullptr, !=, &Thread::self());
  OwnedMutex omutex;
  TCMP (omutex.owner(), ==, nullptr);
  TCMP (omutex.mine(), ==, false);
  omutex.lock();
  TCMP (omutex.owner(), ==, &Thread::self());
  TCMP (omutex.mine(), ==, true);
  TCMP (lockable (omutex), ==, true);
  bool locked = omutex.try_lock();
  TCMP (locked, ==, true);
  omutex.unlock();
  omutex.unlock();
  TCMP (omutex.owner(), ==, nullptr);
  TCMP (lockable (omutex), ==, true);
  TCMP (omutex.owner(), ==, nullptr);
  locked = omutex.try_lock();
  TCMP (locked, ==, true);
  TCMP (omutex.owner(), ==, &Thread::self());
  TCMP (lockable (omutex), ==, true);
  omutex.unlock();
  TCMP (omutex.owner(), ==, nullptr);
}
REGISTER_TEST ("Threads/C++Threading", test_thread_cxx);

#endif

// == simple spin lock test ==
static void
test_spin_lock_simple (void)
{
  Spinlock sp;
  bool l;
  l = sp.try_lock();
  TASSERT (l);
  l = sp.try_lock();
  TASSERT (!l);
  sp.unlock();
  l = sp.try_lock();
  TASSERT (l);
  l = sp.try_lock();
  TASSERT (!l);
  sp.unlock();
  sp.lock();
  l = sp.try_lock();
  TASSERT (!l);
  sp.unlock();
}
REGISTER_TEST ("Threads/C++SpinLock", test_spin_lock_simple);

// == ScopedLock test ==
template<class M> static bool
lockable (M &mutex)
{
  bool lockable = mutex.try_lock();
  if (lockable)
    mutex.unlock();
  return lockable;
}

template<typename XMutex> static void
test_recursive_scoped_lock (XMutex &rec_mutex, uint depth)
{
  ScopedLock<XMutex> locker (rec_mutex);
  if (depth > 1)
    test_recursive_scoped_lock (rec_mutex, depth - 1);
  else
    {
      locker.lock();
      locker.lock();
      locker.lock();
      bool lockable1 = rec_mutex.try_lock();
      bool lockable2 = rec_mutex.try_lock();
      TASSERT (lockable1 && lockable2);
      rec_mutex.unlock();
      rec_mutex.unlock();
      locker.unlock();
      locker.unlock();
      locker.unlock();
    }
}

static void
test_scoped_locks()
{
  Mutex mutex1;
  TCMP (lockable (mutex1), ==, true);
  {
    ScopedLock<Mutex> locker1 (mutex1);
    TCMP (lockable (mutex1), ==, false);
  }
  TCMP (lockable (mutex1), ==, true);
  {
    ScopedLock<Mutex> locker0 (mutex1, BALANCED_LOCK);
    TCMP (lockable (mutex1), ==, true);
    locker0.lock();
    TCMP (lockable (mutex1), ==, false);
    locker0.unlock();
    TCMP (lockable (mutex1), ==, true);
  }
  TCMP (lockable (mutex1), ==, true);
  {
    ScopedLock<Mutex> locker2 (mutex1, AUTOMATIC_LOCK);
    TCMP (lockable (mutex1), ==, false);
    locker2.unlock();
    TCMP (lockable (mutex1), ==, true);
    locker2.lock();
    TCMP (lockable (mutex1), ==, false);
  }
  TCMP (lockable (mutex1), ==, true);
  // test ScopedLock balancing unlock + lock
  mutex1.lock();
  {
    TCMP (lockable (mutex1), ==, false);
    ScopedLock<Mutex> locker (mutex1, BALANCED_LOCK);
    locker.unlock();
    TCMP (lockable (mutex1), ==, true);
  } // ~ScopedLock (BALANCED_LOCK) now does locker.lock()
  TCMP (lockable (mutex1), ==, false);
  {
    ScopedLock<Mutex> locker (mutex1, BALANCED_LOCK);
  } // ~ScopedLock (BALANCED_LOCK) now does nothing
  TCMP (lockable (mutex1), ==, false);
  mutex1.unlock();
  // test ScopedLock balancing lock + unlock
  {
    TCMP (lockable (mutex1), ==, true);
    ScopedLock<Mutex> locker (mutex1, BALANCED_LOCK);
    locker.lock();
    TCMP (lockable (mutex1), ==, false);
  } // ~ScopedLock (BALANCED_LOCK) now does locker.unlock()
  TCMP (lockable (mutex1), ==, true);
  {
    ScopedLock<Mutex> locker (mutex1, BALANCED_LOCK);
  } // ~ScopedLock (BALANCED_LOCK) now does nothing
  TCMP (lockable (mutex1), ==, true);
}
REGISTER_TEST ("Threads/Scoped Locks", test_scoped_locks);

// == C++ atomicity tests ==
static void
test_thread_atomic_cxx (void)
{
  /* integer functions */
  Atomic<int> ai;
  int r;
  ai.store (17);
  TCMP (ai, ==, 17);
  r = ai;
  TCMP (r, ==, 17);
  ai += 9;
  r = ai.load();
  TCMP (r, ==, 26);
  ai = -1147483648;
  TCMP (ai, ==, -1147483648);
  r = ai.load();
  TCMP (r, ==, -1147483648);
  ai += 9;
  r = ai;
  TCMP (r, ==, -1147483639);
  ai += -20;
  r = ai.load();
  TCMP (r, ==, -1147483659);
  r = ai.cas (17, 19);
  TCMP (r, ==, false);
  r = ai.load();
  TCMP (r, ==, -1147483659);
  r = ai.cas (-1147483659, 19);
  TCMP (r, ==, true);
  r = ai.load();
  TCMP (r, ==, 19);
  r = ai++;
  TCMP (r, ==, 19); TCMP (ai, ==, 20);
  r = ai--;
  TCMP (r, ==, 20); TCMP (ai, ==, 19);
  r = ++ai;
  TCMP (r, ==, 20); TCMP (ai, ==, 20);
  r = --ai;
  TCMP (r, ==, 19); TCMP (ai, ==, 19);
  r = (ai += 1);
  TCMP (r, ==, 20);
  r = ai.load();
  TCMP (r, ==, 20);
  r = (ai -= 20);
  TCMP (r, ==, 0);
  r = ai.load();
  TCMP (r, ==, 0);
  /* pointer functions */
  Atomic<void*> ap;
  void *p;
  ap = (void*) 119;
  TCMP (ap, ==, (void*) 119);
  p = ap;
  TCMP (p, ==, (void*) 119);
  r = ap.cas ((void*) 17, (void*) -42);
  TCMP (r, ==, false);
  p = ap.load();
  TCMP (p, ==, (void*) 119);
  r = ap.cas ((void*) 119, (void*) 4294967279U);
  TCMP (r, ==, true);
  p = ap;
  TCMP (p, ==, (void*) 4294967279U);
}
REGISTER_TEST ("Threads/C++AtomicThreading", test_thread_atomic_cxx);

#if 0 // FIXME

/* --- thread_yield --- */
static inline void
handle_contention ()
{
  /* we're waiting for our contention counterpart if we got here:
   * - sched_yield(3posix) will immediately give up the CPU and let another
   *   task run. but if the contention counterpart is running on another
   *   CPU this will lead to scheduler trashing on our CPU. and if other
   *   bacground tasks are running, they could get all our CPU time,
   *   because sched_yield() effectively discards the current time slice.
   * - busy spinning is useful if the contention counterpart runs on a
   *   different CPU, as long as the loop doesn't involve syncronization
   *   primitives which cause IO bus trashing ("lock" prefix in x86 asm).
   * - usleep(3posix) is a way to give up the CPU without discarding our
   *   time slices and avoids scheduler or bus trashing. allthough it is
   *   not the perfect or optimum syncronization/timing primitive, it
   *   avoids most ill effects and still allows for a sufficient number
   *   of task switches.
   */
  usleep (500); // 1usec is the minimum value to cause an effect
}

/* --- ring buffer --- */
typedef Atomic0::RingBuffer<int> IntRingBuffer;
class IntSequence {
  uint32 accu;
public:
  explicit      IntSequence() : accu (123456789) {}
  inline int32  gen_int    () { accu = 1664525 * accu + 1013904223; return accu; }
};
#define CONTENTION_PRINTF       while (0) printerr
struct RingBufferWriter : public virtual Rapicorn::Thread, IntSequence {
  IntRingBuffer *ring;
  uint           ring_buffer_test_length;
  RingBufferWriter (IntRingBuffer *rb,
                    uint           rbtl) :
    Thread ("RingBufferWriter"),
    ring (rb), ring_buffer_test_length (rbtl)
  {}
  virtual void
  run ()
  {
    TINFO ("%s start.", Thread::Self::name().c_str());
    for (uint l = 0; l < ring_buffer_test_length;)
      {
        uint k, n = Test::rand_int() % MIN (ring_buffer_test_length - l + 1, 65536 * 2);
        int buffer[n], *b = buffer;
        for (uint i = 0; i < n; i++)
          b[i] = gen_int();
        uint j = n;
        while (j)
          {
            k = ring->write (j, b);
            TCMP (k, <=, j);
            j -= k;
            b += k;
            if (!k)     // waiting for reader thread
              handle_contention();
            CONTENTION_PRINTF (k ? "*" : "/");
          }
        if (l / 499999 != (l + n) / 499999)
          TOK();
        l += n;
      }
    TINFO ("%s done.", Thread::Self::name().c_str());
  }
};
struct RingBufferReader : public virtual Rapicorn::Thread, IntSequence {
  IntRingBuffer *ring;
  uint           ring_buffer_test_length;
  RingBufferReader (IntRingBuffer *rb,
                    uint           rbtl) :
    Thread ("RingBufferReader"),
    ring (rb), ring_buffer_test_length (rbtl)
  {}
  virtual void
  run ()
  {
    TINFO ("%s start.", Thread::Self::name().c_str());
    for (uint l = 0; l < ring_buffer_test_length;)
      {
        uint k, n = ring->n_readable();
        n = lrand48() % MIN (n + 1, 65536 * 2);
        int buffer[n], *b = buffer;
        if (rand() & 1)
          {
            k = ring->read (n, b, false);
            TCMP (n, ==, k);
            if (k)
              CONTENTION_PRINTF ("+");
          }
        else
          {
            k = ring->read (n, b, true);
            TCMP (k, <=, n);
            if (!k)         // waiting for writer thread
              handle_contention();
            CONTENTION_PRINTF (k ? "+" : "\\");
          }
        for (uint i = 0; i < k; i++)
          TCMP (b[i], ==, gen_int());
        if (l / 499999 != (l + k) / 499999)
          TOK();
        l += k;
      }
    TINFO ("%s done.", Thread::Self::name().c_str());
  }
};

static void
test_ring_buffer ()
{
  static const char *testtext = "Ring Buffer test Text (47\xff)";
  uint n, ttl = strlen (testtext);
  Atomic0::RingBuffer<char> rb1 (ttl);
  TCMP (rb1.n_writable(), ==, ttl);
  n = rb1.write (ttl, testtext);
  TCMP (n, ==, ttl);
  TCMP (rb1.n_writable(), ==, 0);
  TCMP (rb1.n_readable(), ==, ttl);
  char buffer[8192];
  n = rb1.read (8192, buffer);
  TCMP (n, ==, ttl);
  TCMP (rb1.n_readable(), ==, 0);
  TCMP (rb1.n_writable(), ==, ttl);
  TCMP (strncmp (buffer, testtext, n), ==, 0);
  TDONE();

  /* check lower end ring buffer sizes (high contention test) */
  for (uint step = 1; step < 8; step++)
    {
      uint ring_buffer_test_length = 17 * step + (rand() % 19);
      TSTART ("Threads/AsyncRingBuffer-%d-%d", step, ring_buffer_test_length);
      IntRingBuffer irb (step);
      RingBufferReader *rbr = new RingBufferReader (&irb, ring_buffer_test_length);
      ref_sink (rbr);
      RingBufferWriter *rbw = new RingBufferWriter (&irb, ring_buffer_test_length);
      ref_sink (rbw);
      TASSERT (rbr && rbw);
      rbr->start();
      rbw->start();
      rbw->wait_for_exit();
      rbr->wait_for_exit();
      TASSERT (rbr && rbw);
      unref (rbr);
      unref (rbw);
      TDONE();
    }

  /* check big ring buffer sizes */
  TSTART ("Threads/AsyncRingBuffer-big");
  uint ring_buffer_test_length = 999999 * (Test::slow() ? 20 : 1);
  IntRingBuffer irb (16384 + (lrand48() % 8192));
  RingBufferReader *rbr = new RingBufferReader (&irb, ring_buffer_test_length);
  ref_sink (rbr);
  RingBufferWriter *rbw = new RingBufferWriter (&irb, ring_buffer_test_length);
  ref_sink (rbw);
  TASSERT (rbr && rbw);
  rbr->start();
  rbw->start();
  rbw->wait_for_exit();
  rbr->wait_for_exit();
  TASSERT (rbr && rbw);
  unref (rbr);
  unref (rbw);
}
REGISTER_TEST ("Threads/RingBuffer", test_ring_buffer);
REGISTER_SLOWTEST ("Threads/RingBuffer (slow)", test_ring_buffer);

#endif

/* --- late deletable destruction --- */
static bool deletable_destructor = false;
struct MyDeletable : public virtual Deletable {
  virtual
  ~MyDeletable()
  {
    deletable_destructor = true;
  }
  void
  force_deletion_hooks()
  {
    invoke_deletion_hooks();
  }
};
struct MyDeletableHook : public Deletable::DeletionHook {
  Deletable *deletable;
  explicit     MyDeletableHook () :
    deletable (NULL)
  {}
  virtual void
  monitoring_deletable (Deletable &deletable_obj)
  {
    TCMP (deletable, ==, nullptr);
    deletable = &deletable_obj;
  }
  virtual void
  dismiss_deletable ()
  {
    if (deletable)
      deletable = NULL;
  }
  virtual
  ~MyDeletableHook ()
  {
    // g_printerr ("~MyDeletableHook(): deletable=%p\n", deletable);
    if (deletable)
      deletable_remove_hook (deletable);
    deletable = NULL;
  }
};

static MyDeletable early_deletable __attribute__ ((init_priority (101)));
static MyDeletable late_deletable __attribute__ ((init_priority (65535)));

static void
test_deletable_destruction ()
{
  {
    MyDeletable test_deletable;
    TOK();
    MyDeletableHook dhook1;
    // g_printerr ("TestHook=%p\n", (Deletable::DeletionHook*) &dhook1);
    dhook1.deletable_add_hook (&test_deletable);
    TOK();
    dhook1.deletable_remove_hook (&test_deletable);
    dhook1.dismiss_deletable();
    TOK();
    MyDeletableHook dhook2;
    dhook2.deletable_add_hook (&test_deletable);
    test_deletable.force_deletion_hooks ();
    TOK();
    MyDeletableHook dhook3;
    dhook3.deletable_add_hook (&test_deletable);
    TOK();
    /* automatic deletion hook invocation */
    /* FIXME: deletable destructor is called first and doesn't auto-remove
     * - if deletion hooks were ring-linked, we could at least catch this case in ~DeletionHook
     */
  }
  MyDeletable *deletable2 = new MyDeletable;
  TCMP (deletable2, !=, nullptr);
  deletable_destructor = false;
  delete deletable2;
  TCMP (deletable_destructor, ==, true);
  /* early_deletable and late_deletable are only tested at program end */
}
REGISTER_TEST ("Threads/Deletable destruction", test_deletable_destruction);

} // Anon
