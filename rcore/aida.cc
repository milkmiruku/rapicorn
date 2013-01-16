// CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/
#include "aida.hh"
#include "aidaprops.hh"
#include "thread.hh"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <semaphore.h>
#include <poll.h>
#include <sys/eventfd.h>        // defines EFD_SEMAPHORE
#include <stddef.h>             // ptrdiff_t
#include <stdexcept>
#include <deque>

// == Auxillary macros ==
#ifndef __GNUC__
#define __PRETTY_FUNCTION__                     __func__
#endif
#define AIDA_CPP_PASTE2i(a,b)                   a ## b // indirection required to expand __LINE__ etc
#define AIDA_CPP_PASTE2(a,b)                    AIDA_CPP_PASTE2i (a,b)
#define AIDA_STATIC_ASSERT_NAMED(expr,asname)   typedef struct { char asname[(expr) ? 1 : -1]; } AIDA_CPP_PASTE2 (Aida_StaticAssertion_LINE, __LINE__)
#define AIDA_STATIC_ASSERT(expr)                AIDA_STATIC_ASSERT_NAMED (expr, compile_time_assertion_failed)
#define ALIGN4(sz,unit)                         (sizeof (unit) * ((sz + sizeof (unit) - 1) / sizeof (unit)))
#define AIDA_THROW_IF_FAIL(expr)                do { if (AIDA_LIKELY (expr)) break; AIDA_THROW ("failed to assert (" + #expr + ")"); } while (0)
#define AIDA_THROW(msg)                         throw std::runtime_error (std::string() + __PRETTY_FUNCTION__ + ": " + msg)

namespace Rapicorn {
/// The Aida namespace provides all IDL functionality exported to C++.
namespace Aida {

// == FieldUnion ==
AIDA_STATIC_ASSERT (sizeof (FieldUnion::smem) <= sizeof (FieldUnion::bytes));
AIDA_STATIC_ASSERT (sizeof (FieldUnion::bmem) <= 2 * sizeof (FieldUnion::bytes)); // FIXME

/* === Prototypes === */
static std::string string_cprintf (const char *format, ...) AIDA_PRINTF (1, 2);

// === Utilities ===
static std::string
string_vcprintf (const char *format, va_list vargs, bool force_newline = false)
{
  static locale_t volatile clocale = NULL;
  if (!clocale)
    {
      locale_t tmploc = newlocale (LC_ALL_MASK, "C", NULL);
      if (!__sync_bool_compare_and_swap (&clocale, NULL, tmploc))
        freelocale (tmploc);
    }
  locale_t olocale = uselocale (clocale);
  va_list pargs;
  char buffer[1024];
  buffer[0] = 0;
  va_copy (pargs, vargs);
  const int l = vsnprintf (buffer, sizeof (buffer), format, pargs);
  va_end (pargs);
  std::string string;
  if (l < 0)
    string = format; // error?
  else if (size_t (l) < sizeof (buffer))
    string = std::string (buffer, l);
  else
    {
      string.resize (l + 1);
      va_copy (pargs, vargs);
      const int j = vsnprintf (&string[0], string.size(), format, pargs);
      va_end (pargs);
      string.resize (std::min (l, std::max (j, 0)));
    }
  uselocale (olocale);
  if (force_newline && (string.empty() || string[string.size() - 1] != '\n'))
    string += "\n";
  return string;
}

static std::string
string_cprintf (const char *format, ...)
{
  va_list args;
  va_start (args, format);
  std::string s = string_vcprintf (format, args);
  va_end (args);
  return s;
}

void
error_vprintf (const char *format, va_list args)
{
  std::string s = string_vcprintf (format, args, true);
  fprintf (stderr, "Aida: error: %s", s.c_str());
  fflush (stderr);
  abort();
}

void
error_printf (const char *format, ...)
{
  va_list args;
  va_start (args, format);
  error_vprintf (format, args);
  va_end (args);
}

void
warning_printf (const char *format, ...)
{
  va_list args;
  va_start (args, format);
  std::string s = string_vcprintf (format, args, true);
  va_end (args);
  fprintf (stderr, "Aida: warning: %s", s.c_str());
  fflush (stderr);
}

// == Any ==
Any::Any() :
  type_code (TypeMap::notype())
{
  memset (&u, 0, sizeof (u));
}

Any::Any (const Any &clone) :
  type_code (TypeMap::notype())
{
  this->operator= (clone);
}

Any&
Any::operator= (const Any &clone)
{
  if (this == &clone)
    return *this;
  reset();
  type_code = clone.type_code;
  switch (kind())
    {
    case STRING:        new (&u) String (*(String*) &clone.u);          break;
    case SEQUENCE:      new (&u) AnyVector (*(AnyVector*) &clone.u);    break;
    case RECORD:        new (&u) AnyVector (*(AnyVector*) &clone.u);    break;
    case ANY:           u.vany = new Any (*clone.u.vany);               break;
    default:            u = clone.u;                                    break;
    }
  return *this;
}

void
Any::reset()
{
  switch (kind())
    {
    case STRING:        ((String*) &u)->~String();              break;
    case SEQUENCE:      ((AnyVector*) &u)->~AnyVector();        break;
    case RECORD:        ((AnyVector*) &u)->~AnyVector();        break;
    case ANY:           delete u.vany;                          break;
    default: ;
    }
  type_code = TypeMap::notype();
  memset (&u, 0, sizeof (u));
}

void
Any::rekind (TypeKind _kind)
{
  reset();
  const char *type = NULL;
  switch (_kind)
    {
    case UNTYPED:     type = NULL;                                      break;
    case BOOL:        type = "bool";                                    break;
    case INT32:       type = "int32";                                   break;
      // case UINT32: type = "uint32";                                  break;
    case INT64:       type = "int64";                                   break;
    case FLOAT64:     type = "float64";                                 break;
    case ENUM:        type = "int";                                     break;
    case STRING:      type = "String";          new (&u) String();      break;
    case ANY:         type = "Any";             u.vany = new Any();     break;
    case SEQUENCE:    type = "Aida::AnySeq";    new (&u) AnyVector();   break;
    case RECORD:      type = "Aida::AnyRec";    new (&u) AnyVector();   break; // FIXME: mising details
    case INSTANCE:    type = "Aida::Instance";                          break; // FIXME: missing details
    default:
      error_printf ("Aida::Any:rekind: invalid type kind: %s", type_kind_name (_kind));
    }
  type_code = TypeMap::lookup (type);
  if (type_code.untyped() && type != NULL)
    error_printf ("Aida::Any:rekind: invalid type name: %s", type);
  if (kind() != _kind)
    error_printf ("Aida::Any:rekind: mismatch: %s -> %s (%u)", type_kind_name (_kind), type_kind_name (kind()), kind());
}

bool
Any::operator== (const Any &clone) const
{
  if (type_code != clone.type_code)
    return false;
  switch (kind())
    {
    case UNTYPED:     break;
      // case UINT: // chain
    case BOOL: case ENUM: // chain
    case INT32:
    case INT64:       if (u.vint64 != clone.u.vint64) return false;                     break;
    case FLOAT64:     if (u.vdouble != clone.u.vdouble) return false;                   break;
    case STRING:      if (*(String*) &u != *(String*) &clone.u) return false;           break;
    case SEQUENCE:    if (*(AnyVector*) &u != *(AnyVector*) &clone.u) return false;     break;
    case RECORD:      if (*(AnyVector*) &u != *(AnyVector*) &clone.u) return false;     break;
    case INSTANCE:    if (memcmp (&u, &clone.u, sizeof (u)) != 0) return false;         break; // FIXME
    case ANY:         if (*u.vany != *clone.u.vany) return false;                       break;
    default:
      error_printf ("Aida::Any:operator==: invalid type kind: %s", type_kind_name (kind()));
    }
  return true;
}

bool
Any::operator!= (const Any &clone) const
{
  return !operator== (clone);
}

Any::~Any ()
{
  reset();
}

void
Any::retype (const TypeCode &tc)
{
  if (type_code.untyped())
    reset();
  else
    {
      rekind (tc.kind());
      type_code = tc;
    }
}

void
Any::swap (Any &other)
{
  const size_t usize = sizeof (this->u);
  char *buffer[usize];
  memcpy (buffer, &other.u, usize);
  memcpy (&other.u, &this->u, usize);
  memcpy (&this->u, buffer, usize);
  type_code.swap (other.type_code);
}

bool
Any::to_int (int64_t &v, char b) const
{
  if (kind() != INT32 && kind() != INT64)
    return false;
  bool s = 0;
  switch (b)
    {
    case 1:     s =  u.vint64 >=         0 &&  u.vint64 <= 1;        break;
    case 7:     s =  u.vint64 >=      -128 &&  u.vint64 <= 127;      break;
    case 8:     s =  u.vint64 >=         0 &&  u.vint64 <= 256;      break;
    case 47:    s = sizeof (long) == sizeof (int64_t); // chain
    case 31:    s |= u.vint64 >=   INT_MIN &&  u.vint64 <= INT_MAX;  break;
    case 48:    s = sizeof (long) == sizeof (int64_t); // chain
    case 32:    s |= u.vint64 >=         0 &&  u.vint64 <= UINT_MAX; break;
    case 63:    s = 1; break;
    case 64:    s = 1; break;
    default:    s = 0; break;
    }
  if (s)
    v = u.vint64;
  return s;
}

int64_t
Any::as_int () const
{
  switch (kind())
    {
    case BOOL:          return u.vint64;
    case INT32:
    case INT64:         return u.vint64;
    case FLOAT64:       return u.vdouble;
    case ENUM:          return u.vint64;
    case STRING:        return !((String*) &u)->empty();
    default:            return 0;
    }
}

double
Any::as_float () const
{
  switch (kind())
    {
    case BOOL:          return u.vint64;
    case INT32:
    case INT64:         return u.vint64;
    case FLOAT64:       return u.vdouble;
    case ENUM:          return u.vint64;
    case STRING:        return !((String*) &u)->empty();
    default:            return 0;
    }
}

std::string
Any::as_string() const
{
  switch (kind())
    {
    case BOOL: case ENUM:
    case INT32:
    case INT64:         return string_cprintf ("%lli", u.vint64);
    case FLOAT64:       return string_cprintf ("%.17g", u.vdouble);
    case STRING:        return *(String*) &u;
    default:            return "";
    }
}

bool
Any::operator>>= (int64_t &v) const
{
  const bool r = to_int (v, 63);
  return r;
}

bool
Any::operator>>= (double &v) const
{
  if (kind() != FLOAT64)
    return false;
  v = u.vdouble;
  return true;
}

bool
Any::operator>>= (std::string &v) const
{
  if (kind() != STRING)
    return false;
  v = *(String*) &u;
  return true;
}

bool
Any::operator>>= (const Any *&v) const
{
  if (kind() != ANY)
    return false;
  v = u.vany;
  return true;
}

void
Any::operator<<= (uint64_t v)
{
  // ensure (UINT);
  operator<<= (int64_t (v));
}

void
Any::operator<<= (int64_t v)
{
  if (kind() == BOOL && v >= 0 && v <= 1)
    {
      u.vint64 = v;
      return;
    }
  ensure (INT64);
  u.vint64 = v;
}

void
Any::operator<<= (double v)
{
  ensure (FLOAT64);
  u.vdouble = v;
}

void
Any::operator<<= (const String &v)
{
  ensure (STRING);
  ((String*) &u)->assign (v);
}

void
Any::operator<<= (const Any &v)
{
  ensure (ANY);
  if (u.vany != &v)
    {
      Any *old = u.vany;
      u.vany = new Any (v);
      if (old)
        delete old;
    }
}

void
Any::resize (size_t n)
{
  ensure (SEQUENCE);
  ((AnyVector*) &u)->resize (n);
}

// == OrbObject ==
OrbObject::OrbObject (ptrdiff_t orbid) :
  orbid_ (orbid)
{}

// == OrbObjectImpl ==
struct OrbObjectImpl : public OrbObject {
  OrbObjectImpl (ptrdiff_t obid) : OrbObject (obid) {}
};

static const OrbObjectImpl aida_orb_object_null (0);

// == SmartHandle ==
static OrbObject* smart_handle_null_orb_object () { return &const_cast<OrbObjectImpl&> (aida_orb_object_null); }

SmartHandle::SmartHandle (OrbObject &orbo) :
  orbo_ (&orbo)
{
  assert (&orbo);
}

SmartHandle::SmartHandle() :
  orbo_ (smart_handle_null_orb_object())
{}

void
SmartHandle::reset ()
{
  if (orbo_ != smart_handle_null_orb_object())
    {
      orbo_ = smart_handle_null_orb_object();
    }
}

void
SmartHandle::assign (const SmartHandle &src)
{
  if (orbo_ == src.orbo_)
    return;
  if (!_is_null())
    reset();
  orbo_ = src.orbo_;
}

SmartHandle::~SmartHandle()
{}

// == ObjectBroker ==
typedef std::map<ptrdiff_t, OrbObject*> OrboMap;
static OrboMap orbo_map;
static Mutex   orbo_mutex;

void
ObjectBroker::pop_handle (FieldReader &fr, SmartHandle &sh)
{
  assert (sh._is_null() == true);
  const uint64_t orbid = fr.pop_object();
  ScopedLock<Mutex> locker (orbo_mutex);
  OrbObject *orbo = orbo_map[orbid];
  if (AIDA_UNLIKELY (!orbo))
    orbo_map[orbid] = orbo = new OrbObjectImpl (orbid);
  sh.assign (SmartHandle (*orbo));
}

void
ObjectBroker::dup_handle (const ptrdiff_t fake[2], SmartHandle &sh)
{
  assert (sh._is_null() == true);
  if (fake[1])
    return;
  const uint64_t orbid = fake[0];
  ScopedLock<Mutex> locker (orbo_mutex);
  OrbObject *orbo = orbo_map[orbid];
  if (AIDA_UNLIKELY (!orbo))
    orbo_map[orbid] = orbo = new OrbObjectImpl (orbid);
  sh.assign (SmartHandle (*orbo));
}

// == FieldBuffer ==
FieldBuffer::FieldBuffer (uint _ntypes) :
  buffermem (NULL)
{
  AIDA_STATIC_ASSERT (sizeof (FieldBuffer) <= sizeof (FieldUnion));
  // buffermem layout: [{n_types,nth}] [{type nibble} * n_types]... [field]...
  const uint _offs = 1 + (_ntypes + 7) / 8;
  buffermem = new FieldUnion[_offs + _ntypes];
  wmemset ((wchar_t*) buffermem, 0, sizeof (FieldUnion[_offs]) / sizeof (wchar_t));
  buffermem[0].capacity = _ntypes;
  buffermem[0].index = 0;
}

FieldBuffer::FieldBuffer (uint32_t    _ntypes,
                          FieldUnion *_bmem,
                          uint32_t    _bmemlen) :
  buffermem (_bmem)
{
  const uint32_t _offs = 1 + (_ntypes + 7) / 8;
  assert (_bmem && _bmemlen >= sizeof (FieldUnion[_offs + _ntypes]));
  wmemset ((wchar_t*) buffermem, 0, sizeof (FieldUnion[_offs]) / sizeof (wchar_t));
  buffermem[0].capacity = _ntypes;
  buffermem[0].index = 0;
}

FieldBuffer::~FieldBuffer()
{
  reset();
  if (buffermem)
    delete [] buffermem;
}

void
FieldBuffer::check_internal ()
{
  if (size() > capacity())
    {
      String msg = string_cprintf ("FieldBuffer(this=%p): capacity=%u size=%u",
                                   this, capacity(), size());
      throw std::out_of_range (msg);
    }
}

void
FieldReader::check_request (int type)
{
  if (m_nth >= n_types())
    {
      String msg = string_cprintf ("FieldReader(this=%p): size=%u requested-index=%u",
                                   this, n_types(), m_nth);
      throw std::out_of_range (msg);
    }
  if (get_type() != type)
    {
      String msg = string_cprintf ("FieldReader(this=%p): size=%u index=%u type=%s requested-type=%s",
                                   this, n_types(), m_nth,
                                  FieldBuffer::type_name (get_type()).c_str(), FieldBuffer::type_name (type).c_str());
      throw std::invalid_argument (msg);
    }
}

std::string
FieldBuffer::first_id_str() const
{
  uint64_t fid = first_id();
  return string_cprintf ("%016llx", fid);
}

static std::string
strescape (const std::string &str)
{
  std::string buffer;
  for (std::string::const_iterator it = str.begin(); it != str.end(); it++)
    {
      uint8_t d = *it;
      if (d < 32 || d > 126 || d == '?')
        buffer += string_cprintf ("\\%03o", d);
      else if (d == '\\')
        buffer += "\\\\";
      else if (d == '"')
        buffer += "\\\"";
      else
        buffer += d;
    }
  return buffer;
}

std::string
FieldBuffer::type_name (int field_type)
{
  const char *tkn = type_kind_name (TypeKind (field_type));
  if (tkn)
    return tkn;
  return string_cprintf ("<invalid:%d>", field_type);
}

std::string
FieldBuffer::to_string() const
{
  String s = string_cprintf ("Aida::FieldBuffer(%p)={", this);
  s += string_cprintf ("size=%u, capacity=%u", size(), capacity());
  FieldReader fbr (*this);
  for (size_t i = 0; i < size(); i++)
    {
      const String tname = type_name (fbr.get_type());
      const char *tn = tname.c_str();
      switch (fbr.get_type())
        {
        case UNTYPED:
        case FUNC:
        case TYPE_REFERENCE:
        case VOID:      s += string_cprintf (", %s", tn); fbr.skip();                               break;
        case BOOL: case ENUM:
        case INT32:
        case INT64:     s += string_cprintf (", %s: 0x%llx", tn, fbr.pop_int64());                  break;
        case FLOAT64:   s += string_cprintf (", %s: %.17g", tn, fbr.pop_double());                  break;
        case STRING:    s += string_cprintf (", %s: %s", tn, strescape (fbr.pop_string()).c_str()); break;
        case SEQUENCE:  s += string_cprintf (", %s: %p", tn, &fbr.pop_seq());                       break;
        case RECORD:    s += string_cprintf (", %s: %p", tn, &fbr.pop_rec());                       break;
        case INSTANCE:  s += string_cprintf (", %s: %p", tn, (void*) fbr.pop_object());             break;
        case ANY:       s += string_cprintf (", %s: %p", tn, &fbr.pop_any());                       break;
        default:        s += string_cprintf (", %u: <unknown>", fbr.get_type()); fbr.skip();        break;
        }
    }
  s += '}';
  return s;
}

FieldBuffer*
FieldBuffer::new_error (const String &msg,
                        const String &domain)
{
  FieldBuffer *fr = FieldBuffer::_new (3 + 2);
  const uint64_t MSGID_ERROR = 0x8000000000000000ULL;
  fr->add_header (MSGID_ERROR, 0, 0, 0);
  fr->add_string (msg);
  fr->add_string (domain);
  return fr;
}

FieldBuffer*
FieldBuffer::new_result (uint64_t h, uint64_t l, uint32_t n)
{
  FieldBuffer *fr = FieldBuffer::_new (3 + n);
  const uint64_t MSGID_RESULT_MASK = 0x9000000000000000ULL;
  fr->add_header (MSGID_RESULT_MASK, 0, h, l);
  return fr;
}

class OneChunkFieldBuffer : public FieldBuffer {
  virtual ~OneChunkFieldBuffer () { reset(); buffermem = NULL; }
  explicit OneChunkFieldBuffer (uint32_t    _ntypes,
                                FieldUnion *_bmem,
                                uint32_t    _bmemlen) :
    FieldBuffer (_ntypes, _bmem, _bmemlen)
  {}
public:
  static OneChunkFieldBuffer*
  _new (uint32_t _ntypes)
  {
    const uint32_t _offs = 1 + (_ntypes + 7) / 8;
    size_t bmemlen = sizeof (FieldUnion[_offs + _ntypes]);
    size_t objlen = ALIGN4 (sizeof (OneChunkFieldBuffer), int64_t);
    uint8_t *omem = (uint8_t*) operator new (objlen + bmemlen);
    FieldUnion *bmem = (FieldUnion*) (omem + objlen);
    return new (omem) OneChunkFieldBuffer (_ntypes, bmem, bmemlen);
  }
};

FieldBuffer*
FieldBuffer::_new (uint32_t _ntypes)
{
  return OneChunkFieldBuffer::_new (_ntypes);
}

// == EventFd ==
class EventFd {
  void     operator= (const EventFd&); // no assignments
  explicit EventFd   (const EventFd&); // no copying
  int      efd;
public:
  int inputfd() const { return efd; }
  ~EventFd()
  {
    close (efd);
    efd = -1;
  }
  EventFd() :
    efd (-1)
  {
    do
      efd = eventfd (0 /*initval*/, 0 /*flags*/);
    while (efd < 0 && (errno == EAGAIN || errno == EINTR));
    if (efd >= 0)
      {
        int err;
        long nflags = fcntl (efd, F_GETFL, 0);
        nflags |= O_NONBLOCK;
        do
          err = fcntl (efd, F_SETFL, nflags);
        while (err < 0 && (errno == EINTR || errno == EAGAIN));
      }
    if (efd < 0)
      error_printf ("failed to open eventfd: %s", strerror (errno));
  }
  void
  wakeup()
  {
    int err;
    do
      err = eventfd_write (efd, 1);
    while (err < 0 && errno == EINTR);
    // EAGAIN occours if too many wakeups are pending
  }
  bool
  pollin ()
  {
    struct pollfd pfd = { efd, POLLIN, 0 };
    int presult;
    do
      presult = poll (&pfd, 1, -1);
    while (presult < 0 && (errno == EAGAIN || errno == EINTR));
    return pfd.revents != 0;
  }
  void
  flush () // clear pending wakeups
  {
    eventfd_t bytes8;
    int err;
    do
      err = eventfd_read (efd, &bytes8);
    while (err < 0 && errno == EINTR);
    // EAGAIN occours if no wakeups are pending
  }
};

// == lock-free, single-consumer queue ==
template<class Data> struct MpScQueueF {
  struct Node { Node *next; Data data; };
  MpScQueueF() :
    m_head (NULL), m_local (NULL)
  {}
  bool
  push (Data data)
  {
    Node *node = new Node;
    node->data = data;
    Node *last_head;
    do
      node->next = last_head = m_head;
    while (!__sync_bool_compare_and_swap (&m_head, node->next, node));
    return last_head == NULL; // was empty
  }
  Data
  pop()
  {
    Node *node = pop_node();
    if (node)
      {
        Data d = node->data;
        delete node;
        return d;
      }
    else
      return Data();
  }
protected:
  Node*
  pop_node()
  {
    if (AIDA_UNLIKELY (!m_local))
      {
        Node *prev, *next, *node;
        do
          node = m_head;
        while (!__sync_bool_compare_and_swap (&m_head, node, NULL));
        for (prev = NULL; node; node = next)
          {
            next = node->next;
            node->next = prev;
            prev = node;
          }
        m_local = prev;
      }
    if (m_local)
      {
        Node *node = m_local;
        m_local = node->next;
        return node;
      }
    else
      return NULL;
  }
private:
  Node  *m_head __attribute__ ((aligned (64)));
  // we pad/align with CPU_CACHE_LINE_SIZE to avoid false sharing between pushing and popping threads
  Node  *m_local __attribute__ ((aligned (64)));
} __attribute__ ((aligned (64)));


// == TransportChannel ==
class TransportChannel : public EventFd { // Channel for cross-thread FieldBuffer IO
  MpScQueueF<FieldBuffer*> msg_queue;
  FieldBuffer             *last_fb;
  enum Op { PEEK, POP, POP_BLOCKED };
  FieldBuffer*
  get_msg (const Op op)
  {
    if (!last_fb)
      do
        {
          // fetch new messages
          last_fb = msg_queue.pop();
          if (!last_fb)
            {
              flush();                          // flush stale wakeups, to allow blocking until an empty => full transition
              last_fb = msg_queue.pop();        // retry, to ensure we've not just discarded a real wakeup
            }
          if (last_fb)
            break;
          // no messages available
          if (op == POP_BLOCKED)
            pollin();
        }
      while (op == POP_BLOCKED);
    FieldBuffer *fb = last_fb;
    if (op != PEEK) // advance
      last_fb = NULL;
    return fb; // may be NULL
  }
public:
  void
  send_msg (FieldBuffer *fb, // takes fb ownership
            bool         may_wakeup)
  {
    const bool was_empty = msg_queue.push (fb);
    if (may_wakeup && was_empty)
      wakeup();                                 // wakeups are needed to catch empty => full transition
  }
  FieldBuffer*  fetch_msg()     { return get_msg (POP); }
  bool          has_msg()       { return get_msg (PEEK); }
  FieldBuffer*  pop_msg()       { return get_msg (POP_BLOCKED); }
  ~TransportChannel ()
  {}
  TransportChannel () :
    last_fb (NULL)
  {}
};

// == ClientConnection ==
ClientConnection::ClientConnection (Connector &connector) :
  m_connector (&connector)
{
  pthread_spin_init (&signal_spin_, 0 /* pshared */);
  m_connector->ref();
}

ClientConnection::~ClientConnection ()
{
  m_connector->unref();
  m_connector = NULL;
  pthread_spin_destroy (&signal_spin_);
}

int          ClientConnection::notify_fd ()                  { return m_connector->notify_fd(); }
bool         ClientConnection::pending ()                    { return m_connector->pending(); }
FieldBuffer* ClientConnection::call_remote (FieldBuffer *fb) { return m_connector->call_remote (fb); }

void
ClientConnection::dispatch ()
{
  FieldBuffer *fb = m_connector->pop();
  return_if (fb == NULL);
  FieldReader fbr (*fb);
  const MessageId msgid = MessageId (fbr.pop_int64());
  const uint64_t hashhigh = fbr.pop_int64(), hashlow = fbr.pop_int64();
  if (msgid_is_event (msgid))
    {
      const uint64_t handler_id = fbr.pop_int64();
      // ClientConnection::EventHandler *evh = find_event_handler (handler_id);
      SignalHandler *shandler = signal_lookup (handler_id);
      if (shandler)
        {
          FieldBuffer *fr = shandler->seh (*this, fb, shandler->data);
          if (fr)
            {
              FieldReader frr (*fr);
              const MessageId retid = MessageId (frr.pop_int64());
              frr.skip(); // msgid high
              frr.skip(); // msgid low
              if (msgid_is_error (retid))
                {
                  std::string msg = frr.pop_string(), domain = frr.pop_string();
                  if (domain.size())
                    domain += ": ";
                  msg = domain + msg;
                  warning_printf ("%s", msg.c_str());
                }
              delete fr;
            }
        }
      else
        warning_printf ("invalid signal handler id in %s: %llu", "event", handler_id);
    }
  else if (msgid_is_discon (msgid))
    {
      const uint64_t handler_id = fbr.pop_int64();
      const bool deleted = true; // FIXME: currently broken : connection.delete_event_handler (handler_id);
      if (!deleted)
        warning_printf ("invalid signal handler id in %s: %llu", "disconnect", handler_id);
    }
  else
    warning_printf ("unknown message (%016lx, %016llx%016llx)", msgid, hashhigh, hashlow);
  delete fb;
}

uint64_t
ClientConnection::register_event_handler (EventHandler *evh)
{
  pthread_spin_lock (&signal_spin_);
  const std::pair<UIntSet::iterator,bool> ipair = ehandler_set.insert (ptrdiff_t (evh));
  pthread_spin_unlock (&signal_spin_);
  return ipair.second ? ptrdiff_t (evh) : 0; // unique insertion
}

ClientConnection::EventHandler*
ClientConnection::find_event_handler (uint64_t handler_id)
{
  pthread_spin_lock (&signal_spin_);
  UIntSet::iterator iter = ehandler_set.find (handler_id);
  pthread_spin_unlock (&signal_spin_);
  if (iter == ehandler_set.end())
    return NULL; // unknown handler_id
  EventHandler *evh = (EventHandler*) ptrdiff_t (handler_id);
  return evh;
}

bool
ClientConnection::delete_event_handler (uint64_t handler_id)
{
  pthread_spin_lock (&signal_spin_);
  size_t nerased = ehandler_set.erase (handler_id);
  pthread_spin_unlock (&signal_spin_);
  return nerased > 0; // deletion successful?
}

ClientConnection::EventHandler::~EventHandler()
{}

uint64_t
ClientConnection::signal_connect (uint64_t hhi, uint64_t hlo, uint64_t handle_id, SignalEmitHandler seh, void *data)
{
  assert_return (handle_id > 0, 0);
  assert_return (hhi > 0, 0);   // FIXME: check for signal id
  assert_return (hlo > 0, 0);
  assert_return (seh != NULL, 0);
  SignalHandler *shandler = new SignalHandler;
  shandler->hhi = hhi;
  shandler->hlo = hlo;
  shandler->oid = handle_id;                    // emitting object
  shandler->cid = 0;
  shandler->seh = seh;
  shandler->data = data;
  pthread_spin_lock (&signal_spin_);
  const uint64_t handler_id = ptrdiff_t (shandler);
  signal_handler_map_[handler_id] = shandler;
  pthread_spin_unlock (&signal_spin_);
  Aida::FieldBuffer &fb = *Aida::FieldBuffer::_new (3 + 1 + 2);
  fb.add_header (Rapicorn::Aida::MSGID_SIGCON, 0, shandler->hhi, shandler->hlo);
  fb.add_object (shandler->oid);                // emitting object
  fb <<= handler_id;                            // handler connection request id
  fb <<= 0;                                     // disconnection request id
  Aida::FieldBuffer *connection_result = call_remote (&fb); // deletes fb
  assert_return (connection_result != NULL, 0);
  Aida::FieldReader frr (*connection_result);
  frr.skip_header();
  pthread_spin_lock (&signal_spin_);
  frr >>= shandler->cid;
  pthread_spin_unlock (&signal_spin_);
  delete connection_result;
  return handler_id;
}

bool
ClientConnection::signal_disconnect (uint64_t signal_handler_id)
{
  assert_return (signal_handler_id > 0, 0);
  SignalHandler *shandler = NULL;
  pthread_spin_lock (&signal_spin_);
  auto it = signal_handler_map_.find (signal_handler_id);
  if (it != signal_handler_map_.end())
    {
      shandler = it->second;
      signal_handler_map_.erase (it);
    }
  pthread_spin_unlock (&signal_spin_);
  return_if (!shandler, false);
  Aida::FieldBuffer &fb = *Aida::FieldBuffer::_new (3 + 1 + 2);
  fb.add_header (Rapicorn::Aida::MSGID_SIGCON, 0, shandler->hhi, shandler->hlo);
  fb.add_object (shandler->oid);                // emitting object
  fb <<= 0;                                     // handler connection request id
  fb <<= shandler->cid;                         // disconnection request id
  Aida::FieldBuffer *connection_result = call_remote (&fb); // deletes fb
  if (connection_result)
    {
      // FIXME: handle error message connection_result
      delete connection_result;
      critical_unless (connection_result == NULL);
    }
  shandler->seh (*this, NULL, shandler->data);
  delete shandler;
  return true;
}

ClientConnection::SignalHandler*
ClientConnection::signal_lookup (uint64_t signal_handler_id)
{
  SignalHandler *shandler = NULL;
  pthread_spin_lock (&signal_spin_);
  auto it = signal_handler_map_.find (signal_handler_id);
  if (it != signal_handler_map_.end())
    shandler = it->second;
  pthread_spin_unlock (&signal_spin_);
  return shandler;
}

// == ServerConnectionImpl ==
/// Transport and dispatch layer for messages sent between ClientConnection and ServerConnection.
class ServerConnectionImpl : public ServerConnection {
  TransportChannel         server_queue_;       // messages sent to server
  TransportChannel         client_queue_;       // messages sent to client
  std::deque<FieldBuffer*> client_events_;      // messages pending for client
  sem_t                    result_sem_;         // signal result from server to client
  int                      ref_count;
  RAPICORN_CLASS_NON_COPYABLE (ServerConnectionImpl);
public:
  ServerConnectionImpl () :
    ref_count (1), connector_ (*this)
  {
    sem_init (&result_sem_, 0 /* unshared */, 0 /* init */);
  }
  void
  ref_impl()
  {
    int last_ref = __sync_fetch_and_add (&ref_count, 1);
    assert (last_ref > 0);
  }
  void
  unref_impl()
  {
    int last_ref = __sync_fetch_and_add (&ref_count, -1);
    assert (last_ref > 0);
    if (last_ref == 1)
      delete this;
  }
  virtual
  ~ServerConnectionImpl ()
  {
    assert (ref_count == 0);
    sem_destroy (&result_sem_);
  }
  void  block_for_result        () { sem_wait (&result_sem_); }
  void  notify_for_result       () { sem_post (&result_sem_); }
public: // ServerConnection
  virtual void          send_event (FieldBuffer *fb) { return client_queue_.send_msg (fb, true); } // FIXME: check type
  virtual int           notify_fd  ()    { return server_queue_.inputfd(); }
  virtual bool          pending    ()    { return server_queue_.has_msg(); }
  virtual void          dispatch   ();
  virtual Connector&    connector  ()    { return connector_; }
private: // client Connector
  struct ServerConnector : Connector {
    ServerConnectionImpl &self_;
    explicit              ServerConnector (ServerConnectionImpl &self) : self_ (self) {}
    virtual void          ref         ()             { self_.ref_impl(); }
    virtual void          unref       ()             { self_.unref_impl(); }
    virtual int           notify_fd   ()             { return self_.client_queue_.inputfd(); }
    virtual bool          pending     ()             { return !self_.client_events_.empty() || self_.client_queue_.has_msg(); }
    virtual FieldBuffer*  call_remote (FieldBuffer*);
    virtual FieldBuffer*  pop         ();
  };
  ServerConnector connector_;
};

FieldBuffer*
ServerConnectionImpl::ServerConnector::pop () // Connector::pop
{
  if (self_.client_events_.empty())
    return self_.client_queue_.fetch_msg();
  FieldBuffer *fb = self_.client_events_.front();
  self_.client_events_.pop_front();
  return fb;
}

FieldBuffer*
ServerConnectionImpl::ServerConnector::call_remote (FieldBuffer *fb) // Connector::call_remote
{
  AIDA_THROW_IF_FAIL (fb != NULL);
  // enqueue method call message
  const Aida::MessageId msgid = Aida::MessageId (fb->first_id());
  self_.server_queue_.send_msg (fb, true);
  // wait for result
  const bool needsresult = Aida::msgid_has_result (msgid);
  if (needsresult)
    self_.block_for_result ();
  while (needsresult)
    {
      FieldBuffer *fr = self_.client_queue_.pop_msg();
      Aida::MessageId retid = Aida::MessageId (fr->first_id());
      if (Aida::msgid_is_error (retid))
        {
          FieldReader fbr (*fr);
          fbr.skip_header();
          std::string msg = fbr.pop_string();
          std::string dom = fbr.pop_string();
          warning_printf ("%s: %s", dom.c_str(), msg.c_str());
          delete fr;
        }
      else if (Aida::msgid_is_result (retid))
        return fr;
      else
        self_.client_events_.push_back (fr);
    }
  return NULL;
}

void
ServerConnectionImpl::dispatch ()
{
  FieldBuffer *fb = server_queue_.fetch_msg();
  if (!fb)
    return;
  FieldReader fbr (*fb);
  const MessageId msgid = MessageId (fbr.pop_int64());
  const uint64_t hashhigh = fbr.pop_int64(), hashlow = fbr.pop_int64();
  const bool needsresult = msgid_has_result (msgid);
  struct Wrapper : ServerConnection { using ServerConnection::find_method; };
  const DispatchFunc method = Wrapper::find_method (hashhigh, hashlow);
  FieldBuffer *fr = NULL;
  if (method)
    {
      fbr.reset (*fb);
      fr = method (fbr);
      const MessageId retid = MessageId (fr ? fr->first_id() : 0);
      if (fr && (!needsresult || !msgid_is_result (retid)))
        {
          warning_printf ("bogus result from method (%016lx, %016llx%016llx)", msgid, hashhigh, hashlow);
          delete fr;
          fr = NULL;
        }
    }
  else
    warning_printf ("unknown message (%016lx, %016llx%016llx)", msgid, hashhigh, hashlow);
  delete fb;
  if (needsresult)
    {
      client_queue_.send_msg (fr ? fr : FieldBuffer::new_result (hashhigh, hashlow), false);
      notify_for_result();
    }
}

// == ServerConnection ==
ServerConnection::ServerConnection ()
{}

ServerConnection::~ServerConnection ()
{}

ServerConnection*
ServerConnection::create_threaded ()
{
  ServerConnectionImpl *simpl = new ServerConnectionImpl(); // ref_count=1
  return simpl;
}

// == MethodRegistry ==
static inline bool
operator< (const TypeHash &a, const TypeHash &b)
{
  return AIDA_UNLIKELY (a.typehi == b.typehi) ? a.typelo < b.typelo : a.typehi < b.typehi;
}
typedef std::map<TypeHash, DispatchFunc> DispatcherMap;
static DispatcherMap                    *dispatcher_map = NULL;
static pthread_mutex_t                   dispatcher_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool                              dispatcher_map_frozen = false;

static inline void
ensure_dispatcher_map()
{
  if (AIDA_UNLIKELY (dispatcher_map == NULL))
    {
      pthread_mutex_lock (&dispatcher_mutex);
      if (!dispatcher_map)
        dispatcher_map = new DispatcherMap();
      pthread_mutex_unlock (&dispatcher_mutex);
    }
}

DispatchFunc
ServerConnection::find_method (uint64_t hashhi, uint64_t hashlo)
{
  TypeHash typehash (hashhi, hashlo);
  ensure_dispatcher_map();
#if 1 // avoid costly mutex locking
  if (AIDA_UNLIKELY (dispatcher_map_frozen == false))
    dispatcher_map_frozen = true;
  return (*dispatcher_map)[typehash];
#else
  pthread_mutex_lock (&dispatcher_mutex);
  DispatchFunc dispatcher_func = (*dispatcher_map)[typehash];
  pthread_mutex_unlock (&dispatcher_mutex);
  return dispatcher_func;
#endif
}

void
ServerConnection::MethodRegistry::register_method (const MethodEntry &mentry)
{
  ensure_dispatcher_map();
  AIDA_THROW_IF_FAIL (dispatcher_map_frozen == false);
  pthread_mutex_lock (&dispatcher_mutex);
  DispatcherMap::size_type size_before = dispatcher_map->size();
  TypeHash typehash (mentry.hashhi, mentry.hashlo);
  (*dispatcher_map)[typehash] = mentry.dispatcher;
  DispatcherMap::size_type size_after = dispatcher_map->size();
  pthread_mutex_unlock (&dispatcher_mutex);
  // simple hash collision check (sanity check, see below)
  if (AIDA_UNLIKELY (size_before == size_after))
    {
      errno = EKEYREJECTED;
      perror (string_cprintf ("%s:%u: Aida::ServerConnection::MethodRegistry::register_method: "
                              "duplicate hash registration (%016llx%016llx)",
                              __FILE__, __LINE__, mentry.hashhi, mentry.hashlo).c_str());
      abort();
    }
}

} } // Rapicorn::Aida

#include "aidamap.cc"
