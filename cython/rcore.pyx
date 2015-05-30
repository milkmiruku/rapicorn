# This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0   -*-mode:python;-*-
from libcpp cimport *
from cython.operator cimport dereference as deref
from libc.stdint cimport *
from cpython.object cimport Py_LT, Py_LE, Py_EQ, Py_NE, Py_GT, Py_GE

# == Standard Typedefs ==
ctypedef uint32_t uint
ctypedef int64_t  int64
ctypedef uint64_t uint64

# == Utilities from std:: ==
cdef extern from "memory" namespace "std":
  cppclass shared_ptr[T]:
    shared_ptr     ()
    shared_ptr     (T)
    shared_ptr     (shared_ptr[T]&)
    void reset     ()
    void swap      (shared_ptr&)
    long use_count () const
    bool unique    () const
    T*   get       ()

# == Utilities inlined ==
cdef inline int richcmp_op (ssize_t cmpv, int op): # cmpv==0 euqals, cmpv<0 lesser, cmpv>0 greater
  if   op == Py_LT: return cmpv <  0            # <     0
  elif op == Py_LE: return cmpv <= 0            # <=    1
  elif op == Py_EQ: return cmpv == 0            # ==    2
  elif op == Py_NE: return cmpv != 0            # !=    3
  elif op == Py_GT: return cmpv >  0            # >     4
  elif op == Py_GE: return cmpv >= 0            # >=    5

cdef inline int ssize_richcmp (ssize_t s1, ssize_t s2, int op):
  cdef ssize_t cmpv = -1 if s1 < s2 else s1 > s2
  return richcmp_op (cmpv, op)

def _richcmp (object1, object2, op):
  if   op == 0: return object1 <  object2
  elif op == 1: return object1 <= object2
  elif op == 2: return object1 == object2
  elif op == 3: return object1 != object2
  elif op == 4: return object1 >  object2
  elif op == 5: return object1 >= object2

# == Rapicorn C++ Core ==
# callback types
ctypedef PyxxCaller0[bool]                             Caller0__bool_
ctypedef PyxxCaller1[bool, const Rapicorn__LoopState&] Caller1__bool__LoopState
# marshal functions for callback types
cdef bool pyx_marshal__bool_ (object pycallable) except *:
  return pycallable ()
cdef bool pyx_marshal__bool__LoopState (object pycallable, const Rapicorn__LoopState &a1) except *:
  b1 = Rapicorn__LoopState__wrap (a1)
  return pycallable (b1)
# namespaced declarations
cdef extern from "rapicorn-core.hh"   namespace "Rapicorn":
  cppclass Rapicorn__LoopState                  "Rapicorn::LoopState"
  cppclass Rapicorn__EventSource                "Rapicorn::EventSource"
  cppclass Rapicorn__EventLoop                  "Rapicorn::EventLoop"
  cppclass Rapicorn__MainLoop                   "Rapicorn::MainLoop"
  cdef enum Rapicorn__LoopState__Phase          "Rapicorn::LoopState::Phase":
    LoopState__NONE                             "Rapicorn::LoopState::NONE"
    LoopState__COLLECT                          "Rapicorn::LoopState::COLLECT"
    LoopState__PREPARE                          "Rapicorn::LoopState::PREPARE"
    LoopState__CHECK                            "Rapicorn::LoopState::CHECK"
    LoopState__DISPATCH                         "Rapicorn::LoopState::DISPATCH"
    LoopState__DESTROY                          "Rapicorn::LoopState::DESTROY"
  # C++ LoopState
  cppclass Rapicorn__LoopState:
    Rapicorn__LoopState__Phase  phase
    uint64                      current_time_usecs
    bool                        seen_primary
  # C++ EventLoop
  cppclass Rapicorn__EventLoop:
    cppclass BoolPollFunctor                    "std::function<bool (PollFD&)>"
    shared_ptr[Rapicorn__EventLoop]     shared_from_this()
    uint add             (shared_ptr[Rapicorn__EventSource], int)
    void wakeup          ()
    void remove          (uint)
    bool try_remove      (uint)
    void destroy_loop    ()
    bool has_primary     ()
    bool flag_primary    (bool)
    Rapicorn__MainLoop* main_loop  () const
    uint exec_now        (Caller0__bool_)
    uint exec_callback   (Caller0__bool_, int priority)
    uint exec_idle       (Caller0__bool_)
    uint exec_timer      (Caller0__bool_, uint delay_ms, uint repeat_ms, int priority)
    uint exec_dispatcher (Caller1__bool__LoopState, int priority)
  int Rapicorn__EventLoop__PRIORITY_LOW         "Rapicorn::EventLoop::PRIORITY_LOW"
  int Rapicorn__EventLoop__PRIORITY_NOW         "Rapicorn::EventLoop::PRIORITY_NOW"
  int Rapicorn__EventLoop__PRIORITY_ASCENT      "Rapicorn::EventLoop::PRIORITY_ASCENT"
  int Rapicorn__EventLoop__PRIORITY_HIGH        "Rapicorn::EventLoop::PRIORITY_HIGH"
  int Rapicorn__EventLoop__PRIORITY_NEXT        "Rapicorn::EventLoop::PRIORITY_NEXT"
  int Rapicorn__EventLoop__PRIORITY_NORMAL      "Rapicorn::EventLoop::PRIORITY_NORMAL"
  int Rapicorn__EventLoop__PRIORITY_UPDATE      "Rapicorn::EventLoop::PRIORITY_UPDATE"
  int Rapicorn__EventLoop__PRIORITY_IDLE        "Rapicorn::EventLoop::PRIORITY_IDLE"
  # C++ MainLoop
  cppclass Rapicorn__MainLoop (Rapicorn__EventLoop):
    int                                 run             () except *
    bool                                running         () except *
    bool                                finishable      () except *
    void                                quit_ "quit"    (int quit_code) except *
    bool                                pending         () except *
    bool                                iterate         (bool blocking) except *
    void                                iterate_pending () except *
    shared_ptr[Rapicorn__EventLoop]     create_slave ()
  shared_ptr[Rapicorn__MainLoop] MainLoop__create "Rapicorn::MainLoop::create" ()

# == Rapicorn Python Core ==
cdef class EventSource:
  cdef shared_ptr[Rapicorn__EventSource] thisp
  def __cinit__ (self, do_not_construct_manually = False):
    assert (do_not_construct_manually)

cdef class LoopState (object):
  cdef Rapicorn__LoopState__Phase  phase
  cdef uint64                      current_time_usecs
  cdef bool                        seen_primary
  NONE     = LoopState__NONE
  COLLECT  = LoopState__COLLECT
  PREPARE  = LoopState__PREPARE
  CHECK    = LoopState__CHECK
  DISPATCH = LoopState__DISPATCH
  DESTROY  = LoopState__DESTROY
  def __init__ (self):
    if Rapicorn__LoopState__ctarg == NULL:
      raise TypeError ("cannot create '%s' instances" % self.__class__.__name__)
    self.current_time_usecs = Rapicorn__LoopState__ctarg.current_time_usecs
    self.phase = Rapicorn__LoopState__ctarg.phase
    self.seen_primary = Rapicorn__LoopState__ctarg.seen_primary
  property current_time_usecs:
    def __get__ (self):         return self.current_time_usecs
    def __set__ (self, val):    self.current_time_usecs = val
  property phase:
    def __get__ (self):         return self.phase
    def __set__ (self, val):    self.phase = val
  property seen_primary:
    def __get__ (self):         return self.seen_primary
    def __set__ (self, val):    self.seen_primary = val
cdef object Rapicorn__LoopState__wrap (const Rapicorn__LoopState &c_arg1):
  global Rapicorn__LoopState__ctarg
  Rapicorn__LoopState__ctarg = &c_arg1
  cdef object pyo = LoopState()
  Rapicorn__LoopState__ctarg = NULL
  return pyo
cdef const Rapicorn__LoopState *Rapicorn__LoopState__ctarg = NULL

cdef class EventLoop:
  cdef shared_ptr[Rapicorn__EventLoop] thisp    # wrapped C++ instance
  def __init__ (self):
    global EventLoop__internal_ctarg
    if EventLoop__internal_ctarg == NULL:
      raise TypeError ("cannot create '%s' instances" % self.__class__.__name__)
    EventLoop__internal_ctarg.swap (self.thisp)
    del EventLoop__internal_ctarg
    EventLoop__internal_ctarg = NULL
  property PRIORITY_LOW:
    def __get__ (self):  return Rapicorn__EventLoop__PRIORITY_LOW
  property PRIORITY_NOW:
    def __get__ (self):  return Rapicorn__EventLoop__PRIORITY_NOW
  property PRIORITY_ASCENT:
    def __get__ (self):  return Rapicorn__EventLoop__PRIORITY_ASCENT
  property PRIORITY_HIGH:
    def __get__ (self):  return Rapicorn__EventLoop__PRIORITY_HIGH
  property PRIORITY_NEXT:
    def __get__ (self):  return Rapicorn__EventLoop__PRIORITY_NEXT
  property PRIORITY_NORMAL:
    def __get__ (self):  return Rapicorn__EventLoop__PRIORITY_NORMAL
  property PRIORITY_UPDATE:
    def __get__ (self):  return Rapicorn__EventLoop__PRIORITY_UPDATE
  property PRIORITY_IDLE:
    def __get__ (self):  return Rapicorn__EventLoop__PRIORITY_IDLE
  def __richcmp__ (EventLoop self, other, int op):
    if self.__class__ != other.__class__:
      return _richcmp (self.__class__, other.__class__, op)
    cdef size_t p1 = <size_t> self.thisp.get()
    cdef size_t p2 = <size_t> (<EventLoop> other).thisp.get()
    r = ssize_richcmp (p1, p2, op)
    return r
  def add (self, EventSource source, priority = Rapicorn__EventLoop__PRIORITY_NORMAL):
    if source.thisp.get() == NULL:
      raise ValueError ("Argument '%s' required to be non empty" % "source")
    return self.thisp.get().add (source.thisp, priority)
  def wakeup (self):                    self.thisp.get().wakeup()
  def remove (self, id):                self.thisp.get().remove (id)
  def try_remove (self, id):            return self.thisp.get().try_remove (id)
  def destroy_loop (self):              self.thisp.get().destroy_loop()
  def has_primary (self):               return self.thisp.get().has_primary()
  def flag_primary (self, on):          return self.thisp.get().flag_primary (on)
  def main_loop (self):
    global EventLoop__internal_ctarg
    cdef Rapicorn__MainLoop *ml = self.thisp.get().main_loop()
    if ml == NULL:
      return None
    EventLoop__internal_ctarg = new shared_ptr[Rapicorn__EventLoop] (ml.shared_from_this())
    return MainLoop()
  # note, we use bool_functor with fallback=1 to keep handlers connected across exceptions
  def exec_now (self, callable):
    return self.thisp.get().exec_now (Caller0__bool_ (callable, &pyx_marshal__bool_))
  def exec_callback (self, callable, priority = Rapicorn__EventLoop__PRIORITY_NORMAL):
    return self.thisp.get().exec_callback (Caller0__bool_ (callable, &pyx_marshal__bool_), priority)
  def exec_idle (self, callable):
    return self.thisp.get().exec_idle (Caller0__bool_ (callable, &pyx_marshal__bool_))
  def exec_timer (self, pycallable, delay_ms, repeat_ms = -1, priority = Rapicorn__EventLoop__PRIORITY_NORMAL):
    if repeat_ms < 0: repeat_ms = delay_ms
    return self.thisp.get().exec_timer (Caller0__bool_ (pycallable, &pyx_marshal__bool_), delay_ms, repeat_ms, priority)
  def exec_dispatcher (self, pycallable, priority = Rapicorn__EventLoop__PRIORITY_NORMAL):
    return self.thisp.get().exec_dispatcher (Caller1__bool__LoopState (pycallable, &pyx_marshal__bool__LoopState), priority)
cdef shared_ptr[Rapicorn__EventLoop] *EventLoop__internal_ctarg

cdef class MainLoop (EventLoop):
  cdef Rapicorn__MainLoop *mainp                # wrapped C++ instance
  def __init__ (self):
    global EventLoop__internal_ctarg
    if EventLoop__internal_ctarg == NULL:
      EventLoop__internal_ctarg = new shared_ptr[Rapicorn__EventLoop] (<shared_ptr[Rapicorn__EventLoop]> MainLoop__create())
    self.mainp = dynamic_cast_MainLoopPtr (EventLoop__internal_ctarg.get()) # MainLoop*
    assert self.mainp != NULL   # need successful dynamic_cast<MainLoop*>
    super (MainLoop, self).__init__()           # EventLoop.__init__ sets up thisp
    pyxx_main_loop_add_watchdog (deref (self.mainp))
  def __dealloc__ (self):
    self.thisp.reset()
  def run (self):                       return self.mainp.run()
  def running (self):                   return self.mainp.running()
  def finishable (self):                return self.mainp.finishable()
  def quit (self, quit_code = 0):       self.mainp.quit_ (quit_code)
  def pending (self):                   return self.mainp.pending()
  def iterate (self, blocking):         return self.mainp.iterate (blocking)
  def iterate_pending (self):           self.mainp.iterate_pending()
  def create_slave (self):
    global EventLoop__internal_ctarg
    EventLoop__internal_ctarg = new shared_ptr[Rapicorn__EventLoop] (self.mainp.create_slave())
    return EventLoop()
