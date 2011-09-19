// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html

#include "uithread.hh"
#include "internal.hh"
#include <semaphore.h>
#include <stdlib.h>
#include <sys/eventfd.h>
#include <deque>
#include <set>

namespace { // Anon
static void wrap_test_runner  (void);
} // Anon

namespace Rapicorn {

class Channel { // Channel for cross-thread FieldBuffer IO
  pthread_spinlock_t        msg_spinlock;
  std::vector<FieldBuffer*> msg_vector;
  std::vector<FieldBuffer*> msg_queue;
  size_t                    msg_index, msg_last_size;
  virtual void  data_notify () = 0;
  virtual void  data_wait   () = 0;
  virtual void  flush_waits () = 0;
public:
  FieldBuffer*
  fetch_msg (const int blockpop)
  {
    do
      {
        // fetch new messages
        if (msg_index >= msg_queue.size())      // no buffered messages left
          {
            if (msg_index)
              msg_queue.resize (0);             // get rid of stale pointers
            msg_index = 0;
            flush_waits();
            pthread_spin_lock (&msg_spinlock);
            msg_vector.swap (msg_queue);        // actual cross-thread fetching
            pthread_spin_unlock (&msg_spinlock);
          }
        // hand out message
        if (msg_index < msg_queue.size())       // have buffered messages
          {
            const size_t index = msg_index;
            if (blockpop > 0) // advance
              msg_index++;
            return msg_queue[index];
          }
        // no messages available
        if (blockpop < 0)
          data_wait();
      }
    while (blockpop < 0);
    return NULL;
  }
  Channel () :
    msg_index (0), msg_last_size (0)
  {
    pthread_spin_init (&msg_spinlock, 0 /* pshared */);
  }
  ~Channel ()
  {
    pthread_spin_destroy (&msg_spinlock);
  }
  void
  push_msg (FieldBuffer *fb) // takes fb ownership
  {
    pthread_spin_lock (&msg_spinlock);
    msg_vector.push_back (fb);
    msg_last_size = msg_vector.size();
    pthread_spin_unlock (&msg_spinlock);
    data_notify();
  }
  bool          has_msg          () { return fetch_msg (0) != NULL; }
  FieldBuffer*  pop_msg          () { wait_msg(); return fetch_msg (1); } // passes fbmsg ownership
  void          wait_msg         () { while (!fetch_msg (-1)); }
  FieldBuffer*  pop_msg_ndelay   () { return fetch_msg (1); } // passes fbmsg ownership
};

class ChannelS : public Channel { // Channel with semaphore for syncronization
  sem_t         msg_sem;
  virtual void  data_notify ()  { sem_post (&msg_sem); }
  virtual void  data_wait   ()  { sem_wait (&msg_sem); }
  virtual void  flush_waits ()  { /*sem_trywait (&msg_sem);*/ }
public:
  explicit      ChannelS    ()  { sem_init (&msg_sem, 0 /* unshared */, 0 /* init */); }
  /*dtor*/     ~ChannelS    ()  { sem_destroy (&msg_sem); }
};

class ChannelE : public Channel, public EventFd { // Channel with EventFd for syncronization
  virtual void  data_notify ()  { wakeup(); }
  virtual void  data_wait   ()  { pollin(); }
  virtual void  flush_waits ()  { flush(); }
public:
  ChannelE ()
  {
    if (open() < 0)
      pfatal ("failed to open pipe for thread communication");
  }
};

class ServerConnectionSource : public virtual EventLoop::Source {
  const char            *WHERE;
  ServerConnection       m_connection;
  PollFD                 pollfd;
  bool                   last_seen_primary, need_check_primary;
public:
  ServerConnectionSource (EventLoop        &loop,
                          ServerConnection  scon) :
    WHERE ("Rapicorn::UIThread::ServerConnection"),
    m_connection (scon), last_seen_primary (false), need_check_primary (false)
  {
    primary (false);
    loop.add (this, EventLoop::PRIORITY_NORMAL);
    pollfd.fd = m_connection.notify_fd();
    pollfd.events = PollFD::IN;
    pollfd.revents = 0;
    add_poll (&pollfd);
  }
  void
  wakeup () // allow external wakeups
  {
    // evil kludge, we're assuming ServerConnection.notify_fd() is an eventfd
    eventfd_write (pollfd.fd, 1);
  }
private:
  ~ServerConnectionSource ()
  {
    remove_poll (&pollfd);
    loop_remove();
  }
  virtual bool
  prepare (const EventLoop::State &state, int64*)
  {
    return need_check_primary || m_connection.pending();
  }
  virtual bool
  check (const EventLoop::State &state)
  {
    if (UNLIKELY (last_seen_primary && !state.seen_primary && !need_check_primary))
      need_check_primary = true;
    last_seen_primary = state.seen_primary;
    return need_check_primary || m_connection.pending();
  }
  virtual bool
  dispatch (const EventLoop::State &state)
  {
    m_connection.dispatch();
    if (need_check_primary)
      {
        need_check_primary = false;
        m_loop->exec_background (check_primaries);
      }
    return true;
  }
  static void
  check_primaries()
  {
    // seen_primary is merely a hint, this handler checks the real loop state
    if (uithread_main_loop()->finishable())
      ApplicationImpl::the().lost_primaries();
  }
};

struct Initializer {
  int *argcp; char **argv; const StringVector *args;
  Plic::ServerConnection server_connection;
  Mutex mutex; Cond cond; uint64 app_id;
};

class UIThread : public Thread {
  static UIThread  *the_uithread;
  Initializer      *m_idata;
  ServerConnectionSource *m_server_connection;
  MainLoop         &m_main_loop;
  RecMutex          m_lifetime;
public:
  UIThread (Initializer *idata) :
    Thread ("Rapicorn::UIThread"), m_idata (idata), m_server_connection (NULL),
    m_main_loop (*ref_sink (MainLoop::_new()))
  {
    assert (the_uithread == NULL);
    the_uithread = ref_sink (this); // keep around undestructed as singleton
  }
  static UIThread*  uithread()    { return Atomic::ptr_get (&the_uithread); }
  MainLoop*         main_loop()   { return &m_main_loop; }
  void
  stop()
  {
    m_main_loop.quit();
    m_lifetime.lock(); // syncronize with thread shutdown
    m_lifetime.unlock();
  }
  bool
  unstopped()
  {
    if (m_lifetime.trylock())
      {
        m_lifetime.unlock();
        return false;
      }
    return true;
  }
private:
  ~UIThread ()
  {
    fatal ("UIThread singleton in dtor");
  }
  static void
  trigger_wakeup (void*)
  {
    if (the_uithread)
      the_uithread->m_main_loop.wakeup();
  }
  void
  initialize ()
  {
    return_if_fail (m_idata != NULL);
    // stay inside rapicorn_thread_enter/rapicorn_thread_leave while not polling
    assert (rapicorn_thread_entered() == true);
    MainLoop::LockHooks lock_hooks = { rapicorn_thread_entered, rapicorn_thread_enter, rapicorn_thread_leave };
    m_main_loop.set_lock_hooks (lock_hooks);
    // idata_core() already called
    affinity (string_to_int (string_vector_find (*m_idata->args, "cpu-affinity=", "-1")));
    // initialize ui_thread loop before components
    m_server_connection = ref_sink (new ServerConnectionSource (m_main_loop, m_idata->server_connection));
    // initialize sub systems
    struct InitHookCaller : public InitHook {
      static void  invoke (const String &kind, int *argcp, char **argv, const StringVector &args)
      { invoke_hooks (kind, argcp, argv, args); }
    };
    // UI library core parts
    InitHookCaller::invoke ("ui-core/", m_idata->argcp, m_idata->argv, *m_idata->args);
    // Application Singleton
    InitHookCaller::invoke ("ui-thread/", m_idata->argcp, m_idata->argv, *m_idata->args);
    return_if_fail (NULL != &ApplicationImpl::the());
    // Initializations after Application Singleton
    InitHookCaller::invoke ("ui-app/", m_idata->argcp, m_idata->argv, *m_idata->args);
    // initialize uithread connection handling
    uithread_serverglue (m_idata->server_connection);
    // Complete initialization by signalling caller
    m_idata->mutex.lock();
    m_idata->app_id = connection_object2id (ApplicationImpl::the());
    m_idata->cond.signal();
    m_idata->mutex.unlock();
    m_idata = NULL;
  }
  virtual void
  run ()
  {
    m_lifetime.lock();
    rapicorn_thread_enter();
    Self::set_wakeup (trigger_wakeup, NULL, NULL); // allow external wakeups (unused)
    initialize();
    return_if_fail (m_idata == NULL);
    m_main_loop.run();
    Self::set_wakeup (NULL, NULL, NULL);        // main loop canot be woken up further
    m_main_loop.kill_loops();
    rapicorn_thread_leave();
    m_lifetime.unlock(); // can be used for external lifetime syncronization
  }
};
UIThread *UIThread::the_uithread = NULL;

MainLoop*
uithread_main_loop ()
{
  return UIThread::uithread() ? UIThread::uithread()->main_loop() : NULL;
}

static void
uithread_uncancelled_atexit()
{
  if (UIThread::uithread() && UIThread::uithread()->unstopped())
    {
      /* For proper shutdown, the ui-thread needs to stop running before global
       * dtors or any atexit() handlers are being executed. C9x and C++03 leave
       * this unsolved, so we provide explicit API for the user, like
       * Rapicorn::exit() and Application::shutdown(). In case these are omitted,
       * we're not 100% safe, some earlier atexit() handler could have shot some
       * of our required resources already, however we do our best to start a
       * graceful shutdown at this point.
       */
      FIXME ("UI-Thread still running during exit(), call Application::shutdown()");
      uithread_shutdown();
    }
}

static ClientConnection client_connection_handle;

ClientConnection
uithread_connection (void)
{
  return UIThread::uithread() && UIThread::uithread()->unstopped() ? client_connection_handle : ClientConnection();
}

uint64
uithread_bootup (int *argcp, char **argv, const StringVector &args)
{
  return_val_if_fail (UIThread::uithread() == NULL, 0);
  // catch exit() while UIThread is still running
  atexit (uithread_uncancelled_atexit);
  // setup client/server connection pair
  Initializer idata;
  idata.server_connection = ServerConnection::create_threaded();
  client_connection_handle = ClientConnection (idata.server_connection);
  // initialize and create UIThread
  idata.argcp = argcp; idata.argv = argv; idata.args = &args; idata.app_id = 0;
  UIThread *uithread = new UIThread (&idata);
  assert (uithread == UIThread::uithread());
  // start and syncronize with thread
  idata.mutex.lock();
  uithread->start();
  while (idata.app_id == 0)
    idata.cond.wait (idata.mutex);
  uint64 app_id = idata.app_id;
  idata.mutex.unlock();
  assert (UIThread::uithread() && UIThread::uithread()->unstopped());
  // install handler for UIThread test cases
  wrap_test_runner();
  return app_id;
}

void
uithread_shutdown (void)
{
  if (UIThread::uithread() && UIThread::uithread()->unstopped())
    UIThread::uithread()->stop(); // stops ui thread main loop
}

} // Rapicorn

#include <rcore/testutils.hh>

namespace { // Anon
using namespace Rapicorn;

// === UI-Thread Syscalls ===
struct Callable {
  virtual int64 operator() () = 0;
  virtual      ~Callable   () {}
};
static std::deque<Callable*> syscall_queue;
static Mutex                 syscall_mutex;

static Plic::FieldBuffer*
ui_thread_syscall_twoway (Plic::FieldReader &fbr)
{
  Plic::FieldBuffer &rb = *Plic::FieldBuffer::new_result();
  int64 result = -1;
  syscall_mutex.lock();
  while (syscall_queue.size())
    {
      Callable &callable = *syscall_queue.front();
      syscall_queue.pop_front();
      syscall_mutex.unlock();
      result = callable ();
      delete &callable;
      syscall_mutex.lock();
    }
  syscall_mutex.unlock();
  rb.add_int64 (result);
  return &rb;
}

static const ServerConnection::MethodEntry ui_thread_call_entries[] = {
  { Plic::MSGID_TWOWAY | 0x0c0ffee01, 0x52617069636f726eULL, ui_thread_syscall_twoway, },
};
static ServerConnection::MethodRegistry ui_thread_call_registry (ui_thread_call_entries);

static int64
ui_thread_syscall (Callable *callable)
{
  Plic::FieldBuffer *fb = Plic::FieldBuffer::_new (2);
  fb->add_msgid (Plic::MSGID_TWOWAY | 0x0c0ffee01, 0x52617069636f726eULL); // ui_thread_syscall_twoway
  syscall_mutex.lock();
  syscall_queue.push_back (callable);
  syscall_mutex.unlock();
  FieldBuffer *fr = uithread_connection().call_remote (fb); // deletes fb
  Plic::FieldReader frr (*fr);
  const Plic::MessageId msgid = Plic::MessageId (frr.pop_int64());
  frr.skip(); // FIXME: check full msgid
  int64 result = 0;
  if (Plic::msgid_is_result (msgid))
    result = frr.pop_int64();
  delete fr;
  return result;
}

// === UI-Thread Test Runs ===
class SyscallTestRunner : public Callable {
  void (*test_runner) (void);
public:
  SyscallTestRunner (void (*func) (void)) : test_runner (func) {}
  int64 operator()  ()                    { test_runner(); return 0; }
};

static void
wrap_test_runner (void)
{
  Test::RegisterTest::test_set_trigger (uithread_test_trigger);
}

} // Anon

// === UI-Thread Test Trigger ===
namespace Rapicorn {
class SyscallTestTrigger : public Callable {
  void (*test_func) (void);
public:
  SyscallTestTrigger (void (*tfunc) (void)) : test_func (tfunc) {}
  int64 operator()  ()                      { test_func(); return 0; }
};
void
uithread_test_trigger (void (*test_func) ())
{
  return_if_fail (test_func != NULL);
  return_if_fail (UIThread::uithread() != NULL);
  // run tests from ui-thread
  ui_thread_syscall (new SyscallTestTrigger (test_func));
  // ensure ui-thread shutdown
  uithread_shutdown();
}
} // Rapicorn
