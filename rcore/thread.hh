// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_THREAD_HH__
#define __RAPICORN_THREAD_HH__

#include <rcore/utilities.hh>
#include <rcore/threadlib.hh>
#include <thread>
#include <list>

namespace Rapicorn {

struct RECURSIVE_LOCK {} constexpr RECURSIVE_LOCK {}; ///< Flag for recursive Mutex initialization.

/**
 * The Mutex synchronization primitive is a thin wrapper around std::mutex.
 * This class supports static construction.
 */
class Mutex {
  pthread_mutex_t mutex_;
public:
  constexpr Mutex       () : mutex_ (PTHREAD_MUTEX_INITIALIZER) {}
  constexpr Mutex       (struct RECURSIVE_LOCK) : mutex_ (PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP) {}
  void      lock        ()      { pthread_mutex_lock (&mutex_); }
  void      unlock      ()      { pthread_mutex_unlock (&mutex_); }
  bool      try_lock    ()      { return 0 == pthread_mutex_trylock (&mutex_); }
  bool      debug_locked();
  typedef pthread_mutex_t* native_handle_type;
  native_handle_type native_handle() { return &mutex_; }
  /*ctor*/  Mutex       (const Mutex&) = delete;
  Mutex&    operator=   (const Mutex&) = delete;
};

/**
 * The Spinlock uses low-latency busy spinning to acquire locks.
 * It is a thin wrapper around pthread_spin_lock().
 * This class supports static construction.
 */
class Spinlock {
  pthread_spinlock_t spinlock_;
public:
  constexpr Spinlock    () : spinlock_ RAPICORN_SPINLOCK_INITIALIZER {}
  void      lock        ()      { pthread_spin_lock (&spinlock_); }
  void      unlock      ()      { pthread_spin_unlock (&spinlock_); }
  bool      try_lock    ()      { return 0 == pthread_spin_trylock (&spinlock_); }
  typedef pthread_spinlock_t* native_handle_type;
  native_handle_type native_handle() { return &spinlock_; }
  /*ctor*/  Spinlock    (const Spinlock&) = delete;
  Mutex&    operator=   (const Spinlock&) = delete;
};

/// Class keeping information per Thread.
struct ThreadInfo {
  /// @name Hazard Pointers
  typedef std::vector<void*> VoidPointers;
  void *volatile      hp[8];   ///< Hazard pointers variables, see: http://www.research.ibm.com/people/m/michael/ieeetpds-2004.pdf .
  static VoidPointers collect_hazards (); ///< Collect hazard pointers from all threads. Returns sorted vector of unique elements.
  static inline bool  lookup_pointer  (const std::vector<void*> &ptrs, void *arg); ///< Lookup pointers in a hazard pointer vector.
  /// @name Thread identification
  String                    ident       ();                             ///< Simple identifier for this thread, usually TID/PID.
  String                    name        ();                             ///< Get thread name.
  void                      name        (const String &newname);        ///< Change thread name.
  static inline ThreadInfo& self        ();  ///< Get ThreadInfo for the current thread, inlined, using fast thread local storage.
  /** @name Accessing custom data members
   * For further details, see DataListContainer.
   */
  template<typename T> inline T    get_data    (DataKey<T> *key)         { tdl(); T d = data_list_.get (key); tdu(); return d; }
  template<typename T> inline void set_data    (DataKey<T> *key, T data) { tdl(); data_list_.set (key, data); tdu(); }
  template<typename T> inline void delete_data (DataKey<T> *key)         { tdl(); data_list_.del (key); tdu(); }
  template<typename T> inline T    swap_data   (DataKey<T> *key)         { tdl(); T d = data_list_.swap (key); tdu(); return d; }
  template<typename T> inline T    swap_data   (DataKey<T> *key, T data) { tdl(); T d = data_list_.swap (key, data); tdu(); return d; }
private:
  ThreadInfo        *volatile next;
  pthread_t                   pth_thread_id;
  char                        pad[RAPICORN_CACHE_LINE_ALIGNMENT - sizeof hp - sizeof next - sizeof pth_thread_id];
  String                      name_;
  Mutex                       data_mutex_;
  DataList                    data_list_;
  static ThreadInfo __thread *self_cached;
  /*ctor*/              ThreadInfo      ();
  /*ctor*/              ThreadInfo      (const ThreadInfo&) = delete;
  /*dtor*/             ~ThreadInfo      ();
  ThreadInfo&           operator=       (const ThreadInfo&) = delete;
  static void           destroy_specific(void *vdata);
  void                  reset_specific  ();
  void                  setup_specific  ();
  static ThreadInfo*    create          ();
  void                  tdl             () { data_mutex_.lock(); }
  void                  tdu             () { data_mutex_.unlock(); }
};

struct AUTOMATIC_LOCK {} constexpr AUTOMATIC_LOCK {}; ///< Flag for automatic locking of a ScopedLock<Mutex>.
struct BALANCED_LOCK  {} constexpr BALANCED_LOCK  {}; ///< Flag for balancing unlock/lock in a ScopedLock<Mutex>.

/**
 * The ScopedLock is a scope based lock ownership wrapper.
 * Placing a ScopedLock object on the stack conveniently ensures that its mutex
 * will be automatically locked and properly unlocked when the scope is left,
 * the current function returns or throws an exception. Mutex obbjects to be used
 * by a ScopedLock need to provide the public methods lock() and unlock().
 * In AUTOMATIC_LOCK mode, the owned mutex is automatically locked upon construction
 * and unlocked upon destruction. Intermediate calls to unlock() and lock() on the
 * ScopedLock will be accounted for in the destructor.
 * In BALANCED_LOCK mode, the lock is not automatically acquired upon construction,
 * however the destructor will balance all intermediate unlock() and lock() calls.
 * So this mode can be used to manage ownership for an already locked mutex.
 */
template<class MUTEX>
class ScopedLock {
  MUTEX         &mutex_;
  volatile int   count_;
  RAPICORN_CLASS_NON_COPYABLE (ScopedLock);
public:
  inline     ~ScopedLock () { while (count_ < 0) lock(); while (count_ > 0) unlock(); }
  inline void lock       () { mutex_.lock(); count_++; }
  inline void unlock     () { count_--; mutex_.unlock(); }
  inline      ScopedLock (MUTEX &mutex, struct AUTOMATIC_LOCK = AUTOMATIC_LOCK) : mutex_ (mutex), count_ (0) { lock(); }
  inline      ScopedLock (MUTEX &mutex, struct BALANCED_LOCK) : mutex_ (mutex), count_ (0) {}
};

/**
 * The Cond synchronization primitive is a thin wrapper around pthread_cond_wait().
 * This class supports static construction.
 */
class Cond {
  pthread_cond_t cond_;
  static struct timespec abstime (int64);
  /*ctor*/      Cond        (const Cond&) = delete;
  Cond&         operator=   (const Cond&) = delete;
public:
  constexpr     Cond        () : cond_ (PTHREAD_COND_INITIALIZER) {}
  /*dtor*/     ~Cond        ()  { pthread_cond_destroy (&cond_); }
  void          signal      ()  { pthread_cond_signal (&cond_); }
  void          broadcast   ()  { pthread_cond_broadcast (&cond_); }
  void          wait        (Mutex &m)  { pthread_cond_wait (&cond_, m.native_handle()); }
  void          wait_timed  (Mutex &m, int64 max_usecs)
  { struct timespec abs = abstime (max_usecs); pthread_cond_timedwait (&cond_, m.native_handle(), &abs); }
  typedef pthread_cond_t* native_handle_type;
  native_handle_type native_handle() { return &cond_; }
};

/// The ThisThread namespace provides functions for the current thread of execution.
namespace ThisThread {

String  name            ();             ///< Get thread name.
int     online_cpus     ();             ///< Get the number of available CPUs.
int     affinity        ();             ///< Get the current CPU affinity.
void    affinity        (int cpu);      ///< Set the current CPU affinity.
int     thread_pid      ();             ///< Get the current threads's thread ID (TID). For further details, see gettid().
int     process_pid     ();             ///< Get the process ID (PID). For further details, see getpid().

#ifdef  RAPICORN_DOXYGEN // parts reused from std::this_thread
/// Relinquish the processor to allow execution of other threads. For further details, see std::this_thread::yield().
void                                       yield       ();
/// Returns the pthread_t id for the current thread. For further details, see std::this_thread::get_id().
std::thread::id                            get_id      ();
/// Sleep for @a sleep_duration has been reached. For further details, see std::this_thread::sleep_for().
template<class Rep, class Period>     void sleep_for   (std::chrono::duration<Rep,Period> sleep_duration);
/// Sleep until @a sleep_time has been reached. For further details, see std::this_thread::sleep_until().
template<class Clock, class Duration> void sleep_until (const std::chrono::time_point<Clock,Duration> &sleep_time);
#else // !RAPICORN_DOXYGEN
using namespace std::this_thread;
#endif // !RAPICORN_DOXYGEN

} // ThisThread

#ifdef RAPICORN_CONVENIENCE

/** The @e do_once statement preceeds code blocks to ensure that a critical section is executed atomically and at most once.
 *  Example: @snippet rcore/tests/threads.cc do_once-EXAMPLE
 */
#define do_once                         RAPICORN_DO_ONCE

#endif  // RAPICORN_CONVENIENCE

/// Atomic types are race free integer and pointer types, similar to std::atomic.
/// All atomic types support load(), store(), cas() and additional type specific accessors.
template<typename T> class Atomic;

/// Atomic char type.
template<> struct Atomic<char> : Lib::Atomic<char> {
  constexpr Atomic<char> (char i = 0) : Lib::Atomic<char> (i) {}
  using Lib::Atomic<char>::operator=;
};

/// Atomic int8 type.
template<> struct Atomic<int8> : Lib::Atomic<int8> {
  constexpr Atomic<int8> (int8 i = 0) : Lib::Atomic<int8> (i) {}
  using Lib::Atomic<int8>::operator=;
};

/// Atomic uint8 type.
template<> struct Atomic<uint8> : Lib::Atomic<uint8> {
  constexpr Atomic<uint8> (uint8 i = 0) : Lib::Atomic<uint8> (i) {}
  using Lib::Atomic<uint8>::operator=;
};

/// Atomic int32 type.
template<> struct Atomic<int32> : Lib::Atomic<int32> {
  constexpr Atomic<int32> (int32 i = 0) : Lib::Atomic<int32> (i) {}
  using Lib::Atomic<int32>::operator=;
};

/// Atomic uint32 type.
template<> struct Atomic<uint32> : Lib::Atomic<uint32> {
  constexpr Atomic<uint32> (uint32 i = 0) : Lib::Atomic<uint32> (i) {}
  using Lib::Atomic<uint32>::operator=;
};

/// Atomic int64 type.
template<> struct Atomic<int64> : Lib::Atomic<int64> {
  constexpr Atomic<int64> (int64 i = 0) : Lib::Atomic<int64> (i) {}
  using Lib::Atomic<int64>::operator=;
};

/// Atomic uint64 type.
template<> struct Atomic<uint64> : Lib::Atomic<uint64> {
  constexpr Atomic<uint64> (uint64 i = 0) : Lib::Atomic<uint64> (i) {}
  using Lib::Atomic<uint64>::operator=;
};

/// Atomic pointer type.
template<typename V> class Atomic<V*> : protected Lib::Atomic<V*> {
  typedef Lib::Atomic<V*> A;
public:
  constexpr Atomic    (V *p = nullptr) : A (p) {}
  using A::store;
  using A::load;
  using A::cas;
  using A::operator=;
  V*       operator+= (ptrdiff_t d) volatile { return A::operator+= ((V*) d); }
  V*       operator-= (ptrdiff_t d) volatile { return A::operator-= ((V*) d); }
  operator V* () const volatile { return load(); }
  void     push_link (V*volatile *nextp, V *newv) { do { *nextp = load(); } while (!cas (*nextp, newv)); }
};

// == AsyncBlockingQueue ==
/**
 * This is a thread-safe asyncronous queue which blocks in pop() until data is provided through push().
 */
template<class Value>
class AsyncBlockingQueue {
  Mutex            mutex_;
  Cond             cond_;
  std::list<Value> list_;
public:
  void  push    (const Value &v);
  Value pop     ();
  bool  pending ();
  void  swap    (std::list<Value> &list);
};

// == AsyncNotifyingQueue ==
/**
 * This is a thread-safe asyncronous queue which returns 0 from pop() until data is provided through push().
 */
template<class Value>
class AsyncNotifyingQueue {
  Mutex                 mutex_;
  std::function<void()> notifier_;
  std::list<Value>      list_;
public:
  void  push     (const Value &v);
  Value pop      (Value fallback = 0);
  bool  pending  ();
  void  swap     (std::list<Value> &list);
  void  notifier (const std::function<void()> &notifier);
};

// == Implementation Bits ==
template<class Value> void
AsyncBlockingQueue<Value>::push (const Value &v)
{
  ScopedLock<Mutex> sl (mutex_);
  const bool notify = list_.empty();
  list_.push_back (v);
  if (RAPICORN_UNLIKELY (notify))
    cond_.broadcast();
}

template<class Value> Value
AsyncBlockingQueue<Value>::pop ()
{
  ScopedLock<Mutex> sl (mutex_);
  while (list_.empty())
    cond_.wait (mutex_);
  Value v = list_.front();
  list_.pop_front();
  return v;
}

template<class Value> bool
AsyncBlockingQueue<Value>::pending()
{
  ScopedLock<Mutex> sl (mutex_);
  return !list_.empty();
}

template<class Value> void
AsyncBlockingQueue<Value>::swap (std::list<Value> &list)
{
  ScopedLock<Mutex> sl (mutex_);
  const bool notify = list_.empty();
  list_.swap (list);
  if (notify && !list_.empty())
    cond_.broadcast();
}

template<class Value> void
AsyncNotifyingQueue<Value>::push (const Value &v)
{
  ScopedLock<Mutex> sl (mutex_);
  const bool notify = list_.empty();
  list_.push_back (v);
  if (RAPICORN_UNLIKELY (notify) && notifier_)
    notifier_();
}

template<class Value> Value
AsyncNotifyingQueue<Value>::pop (Value fallback)
{
  ScopedLock<Mutex> sl (mutex_);
  if (RAPICORN_UNLIKELY (list_.empty()))
    return fallback;
  Value v = list_.front();
  list_.pop_front();
  return v;
}

template<class Value> bool
AsyncNotifyingQueue<Value>::pending()
{
  ScopedLock<Mutex> sl (mutex_);
  return !list_.empty();
}

template<class Value> void
AsyncNotifyingQueue<Value>::swap (std::list<Value> &list)
{
  ScopedLock<Mutex> sl (mutex_);
  const bool notify = list_.empty();
  list_.swap (list);
  if (notify && !list_.empty() && notifier_)
    notifier_();
}

template<class Value> void
AsyncNotifyingQueue<Value>::notifier (const std::function<void()> &notifier)
{
  ScopedLock<Mutex> sl (mutex_);
  notifier_ = notifier;
}

inline ThreadInfo&
ThreadInfo::self()
{
  if (RAPICORN_UNLIKELY (!self_cached))
    self_cached = create();
  return *self_cached;
}

inline bool
ThreadInfo::lookup_pointer (const std::vector<void*> &ptrs, void *arg)
{
  size_t n_elements = ptrs.size(), offs = 0;
  while (offs < n_elements)
    {
      size_t i = (offs + n_elements) >> 1;
      void *current = ptrs[i];
      if (arg == current)
        return true;    // match
      else if (arg < current)
        n_elements = i;
      else // (arg > current)
        offs = i + 1;
    }
  return false; // unmatched
}

} // Rapicorn

#endif // __RAPICORN_THREAD_HH__
