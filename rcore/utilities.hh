// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_CORE_UTILITIES_HH__
#define __RAPICORN_CORE_UTILITIES_HH__

#include <rcore/rapicorncdefs.h>
#include <string>
#include <vector>
#include <map>
#include <tr1/memory>   // shared_ptr
#include <plic/runtime.hh>

#if !defined __RAPICORN_CORE_HH__ && !defined __RAPICORN_BUILD__
#error Only <rapicorn-core.hh> can be included directly.
#endif

/* --- internally used headers and macros --- */
#ifdef RAPICORN_CONVENIENCE
/* macro shorthands */
#define DIR_SEPARATOR                   RAPICORN_DIR_SEPARATOR
#define DIR_SEPARATOR_S                 RAPICORN_DIR_SEPARATOR_S
#define SEARCHPATH_SEPARATOR            RAPICORN_SEARCHPATH_SEPARATOR
#define SEARCHPATH_SEPARATOR_S          RAPICORN_SEARCHPATH_SEPARATOR_S
#define CODELOC()                       RAPICORN_CODELOC()
#endif // RAPICORN_CONVENIENCE

namespace Rapicorn {
using namespace Plic;

/* --- short integer types --- */
typedef RapicornUInt8   uint8;
typedef RapicornUInt16  uint16;
typedef RapicornUInt32  uint32;
typedef RapicornUInt64  uint64;
typedef RapicornInt8    int8;
typedef RapicornInt16   int16;
typedef RapicornInt32   int32;
typedef RapicornInt64   int64;
typedef RapicornUnichar unichar;

/* --- convenient stdc++ types --- */
using std::tr1::bad_weak_ptr;
using std::tr1::enable_shared_from_this;
using std::tr1::shared_ptr;
using std::tr1::weak_ptr;
using std::vector;
using std::map;
typedef std::string String;
typedef vector<String> StringVector;

/* --- common (stdc++) utilities --- */
using ::std::swap;
using ::std::min;
using ::std::max;
#undef abs
template<typename T> inline const T&
abs (const T &value)
{
  return max (value, -value);
}
#undef clamp
template<typename T> inline const T&
clamp (const T &value, const T &minimum, const T &maximum)
{
  if (minimum > value)
    return minimum;
  if (maximum < value)
    return maximum;
  return value;
}
template <class T, size_t S> inline std::vector<T>
vector_from_array (const T (&array_entries)[S]) /// Construct a std::vector<T> from a C array of type T[].
{
  std::vector<T> result;
  for (size_t i = 0; i < S; i++)
    result.push_back (array_entries[i]);
  return result;
}

/* --- template utilities --- */
template<class X, class Y> class TraitConvertible {
  static bool f (...);
  static int  f (X*);
public:
  enum { TRUTH = sizeof (f()) != sizeof (f ((Y*) 0)), };
};

/* --- helper macros --- */
#define RAPICORN_STRINGIFY(macro_or_string)     RAPICORN_STRINGIFY_ARG (macro_or_string)
#define RAPICORN_STRINGIFY_ARG(arg)             #arg
#define RAPICORN_CODELOC_STRING()               std::string (std::string (__FILE__) + ":" + RAPICORN_STRINGIFY (__LINE__) + ":" + __FUNCTION__ + "()")
#define RAPICORN_CODELOC()                      (RAPICORN_CODELOC_STRING().c_str())

/* --- typeid base type --- */
class VirtualTypeid {
protected:
  virtual      ~VirtualTypeid      ();
public:
  String        typeid_name        ();
  String        typeid_pretty_name ();
  static String cxx_demangle       (const char *mangled_identifier);
};

/* --- NonCopyable --- */
class NonCopyable {
  NonCopyable& operator=   (const NonCopyable&);
  /*Copy*/     NonCopyable (const NonCopyable&);
protected:
  /*Con*/      NonCopyable () {}
  /*Des*/     ~NonCopyable () {}
};

/* --- private class copies, class ClassDoctor --- */
#ifdef  __RAPICORN_BUILD__
class ClassDoctor;
#else
class ClassDoctor {};
#endif

/* === printing, errors, warnings, debugging === */
void        printerr   (const char   *format, ...) RAPICORN_PRINTF (1, 2);
void        printerr   (const std::string &msg);
void        printout   (const char   *format, ...) RAPICORN_PRINTF (1, 2);
inline void breakpoint ();
String      process_handle ();

/* === internal convenience === */
#ifdef  RAPICORN_CONVENIENCE
#define CHECK              RAPICORN_CHECK              // (condition)
#define PCHECK             RAPICORN_PCHECK             // (condition)
#define ASSERT             RAPICORN_ASSERT             // (condition)
#define PASSERT            RAPICORN_PASSERT            // (condition)
#define DEBUG              RAPICORN_DEBUG
#define PDEBUG             RAPICORN_PDEBUG
#define KEY_DEBUG          RAPICORN_KEY_DEBUG
#define KEY_PDEBUG         RAPICORN_KEY_PDEBUG
#define warn_if_fail       RAPICORN_CHECK              // (condition)
#define throw_if_fail      RAPICORN_THROW_IF_FAIL      // (condition)
#define return_if_fail     RAPICORN_RETURN_IF_FAIL     // (condition)
#define return_val_if_fail RAPICORN_RETURN_VAL_IF_FAIL // (condition, value)
#define assert_unreached   RAPICORN_ASSERT_UNREACHED   // ()
#define assert_not_reached RAPICORN_ASSERT_UNREACHED   // ()
#define BREAKPOINT         Rapicorn::breakpoint        // ()
#define FIXME              RAPICORN_FIXME
#ifndef assert
#define assert            ASSERT
#endif
#endif // RAPICORN_CONVENIENCE

#if (defined __i386__ || defined __x86_64__) && defined __GNUC__ && __GNUC__ >= 2
inline void breakpoint() { __asm__ __volatile__ ("int $03"); }
#elif defined __alpha__ && !defined __osf__ && defined __GNUC__ && __GNUC__ >= 2
inline void breakpoint() { __asm__ __volatile__ ("bpt"); }
#else   /* !__i386__ && !__alpha__ */
inline void breakpoint() { __builtin_trap(); }
#endif
#ifdef  __SOURCE_COMPONENT__
#define RAPICORN__SOURCE_COMPONENT__    __SOURCE_COMPONENT__
#elif   defined __BASE_FILE__
#define RAPICORN__SOURCE_COMPONENT__    __BASE_FILE__
#else
#define RAPICORN__SOURCE_COMPONENT__    __FILE__
#endif

// == Source Location ==
class SourceLocation {
  String m_file, m_line, m_func, m_pretty, m_component;
  uint   m_location_bits;
public:
  enum Bits { NONE = 0, LOCATION = 1, FUNCTION = 2, COMPONENT = 4, KEY = 8 };
  SourceLocation (const char *file, int line, const char *func, const char *pretty_func, const char *component);
  SourceLocation (const char *file, const char *component, const char *key);
  String where () const;
  String where (int bits) const;
  String debug_key (bool explicit_key = false) const;
  String debug_prefix () const;
};
#define RAPICORN_SOURCE_LOCATION        Rapicorn::SourceLocation::SourceLocation (__FILE__, __LINE__, __func__, __PRETTY_FUNCTION__, RAPICORN__SOURCE_COMPONENT__)
#define RAPICORN_SOURCE_COMPONENT       Rapicorn::SourceLocation::SourceLocation (__FILE__, RAPICORN__SOURCE_COMPONENT__, "")
#define RAPICORN_SOURCE_KEY(key)        Rapicorn::SourceLocation::SourceLocation (__FILE__, RAPICORN__SOURCE_COMPONENT__, key)

// == Logging and Assertions ==
class Logging {
  static bool m_debugging;      // cached for speedups
public:
  static void   configure       (const char *option);
  static int    conftest        (const char *option, int vdefault = 0);
  static bool   debugging       ()                      { return RAPICORN_UNLIKELY (m_debugging); }
  static void   message         (const char *kind, const SourceLocation &sloc,
                                 const char *format, ...) RAPICORN_PRINTF (3, 4);
  static void   messagev        (const char *kind, const SourceLocation &sloc, const char *format, va_list vargs);
  static void   abort           () RAPICORN_NORETURN;
  static String help            ();
};

static inline void fatal (const char *format, ...)     RAPICORN_PRINTF (1, 2) RAPICORN_NORETURN;
static inline void pfatal (const char *format, ...)    RAPICORN_PRINTF (1, 2) RAPICORN_NORETURN;
static inline void critical (const char *format, ...)  RAPICORN_PRINTF (1, 2);
static inline void pcritical (const char *format, ...) RAPICORN_PRINTF (1, 2);
static inline void fatal (const char *format, ...)     { va_list a; va_start (a, format); Logging::messagev ("FATAL",     RAPICORN_SOURCE_COMPONENT, format, a); va_end (a); while (1); }
static inline void pfatal (const char *format, ...)    { va_list a; va_start (a, format); Logging::messagev ("PFATAL",    RAPICORN_SOURCE_COMPONENT, format, a); va_end (a); while (1); }
static inline void critical (const char *format, ...)  { va_list a; va_start (a, format); Logging::messagev ("CRITICAL",  RAPICORN_SOURCE_COMPONENT, format, a); va_end (a); }
static inline void pcritical (const char *format, ...) { va_list a; va_start (a, format); Logging::messagev ("PCRITICAL", RAPICORN_SOURCE_COMPONENT, format, a); va_end (a); }
#define RAPICORN_ASSERT_NOT_REACHED             RAPICORN_ASSERT_UNREACHED
#define RAPICORN_ASSERT_UNREACHED()             do { Rapicorn::Logging::message ("FATAL", RAPICORN_SOURCE_LOCATION, "encountered unreachable assertion"); Rapicorn::Logging::abort(); } while (0)
#define RAPICORN_THROW_IF_FAIL(expr)            do { if (RAPICORN_LIKELY (expr)) break; throw Rapicorn::AssertionError (#expr, __FILE__, __LINE__); } while (0)
#define RAPICORN_RETURN_IF_FAIL(expr)           do { if (RAPICORN_LIKELY (expr)) break; Rapicorn::Logging::message ("CHECK", RAPICORN_SOURCE_LOCATION, "check failed: %s", #expr); return; } while (0)
#define RAPICORN_RETURN_VAL_IF_FAIL(expr,rv)    do { if (RAPICORN_LIKELY (expr)) break; Rapicorn::Logging::message ("CHECK", RAPICORN_SOURCE_LOCATION, "check failed: %s", #expr); return rv; } while (0)
#define RAPICORN_ASSERT(expr)  do { if (RAPICORN_LIKELY (expr)) break; Rapicorn::Logging::message ("ABORT",  RAPICORN_SOURCE_LOCATION, "assertion failed: %s",   #expr); } while (0)
#define RAPICORN_PASSERT(expr) do { if (RAPICORN_LIKELY (expr)) break; Rapicorn::Logging::message ("PABORT", RAPICORN_SOURCE_LOCATION, "assertion failed (%s)",  #expr); } while (0)
#define RAPICORN_CHECK(expr)   do { if (RAPICORN_LIKELY (expr)) break; Rapicorn::Logging::message ("CHECK",  RAPICORN_SOURCE_LOCATION, "check failed: %s",  #expr); } while (0)
#define RAPICORN_PCHECK(expr)  do { if (RAPICORN_LIKELY (expr)) break; Rapicorn::Logging::message ("PCHECK", RAPICORN_SOURCE_LOCATION, "check failed (%s)", #expr); } while (0)
#define RAPICORN_DEBUG(...)    do { if (RAPICORN_UNLIKELY (Rapicorn::Logging::debugging())) Rapicorn::Logging::message ("DEBUG",  RAPICORN_SOURCE_LOCATION, __VA_ARGS__); } while (0)
#define RAPICORN_PDEBUG(...)   do { if (RAPICORN_UNLIKELY (Rapicorn::Logging::debugging())) Rapicorn::Logging::message ("PDEBUG", RAPICORN_SOURCE_LOCATION, __VA_ARGS__); } while (0)
#define RAPICORN_FIXME(...)    do { Rapicorn::Logging::message ("DEBUG",  RAPICORN_SOURCE_LOCATION, __VA_ARGS__); } while (0)
#define RAPICORN_KEY_DEBUG(k,...)  do { if (RAPICORN_UNLIKELY (Rapicorn::Logging::debugging())) Rapicorn::Logging::message ("DEBUG",  RAPICORN_SOURCE_KEY (k), __VA_ARGS__); } while (0)
#define RAPICORN_KEY_PDEBUG(k,...) do { if (RAPICORN_UNLIKELY (Rapicorn::Logging::debugging())) Rapicorn::Logging::message ("PDEBUG", RAPICORN_SOURCE_KEY (k), __VA_ARGS__); } while (0)

// == AssertionError ==
class AssertionError : public std::exception /// Exception type, thrown from RAPICORN_THROW_IF_FAIL() and throw_if_fail().
{
  const String m_msg;
public:
  explicit            AssertionError  (const String &expr, const String &file = "", size_t line = 0);
  virtual            ~AssertionError  () throw();
  virtual const char* what            () const throw(); ///< Obtain a string describing the assertion error.
};

/* --- timestamp handling --- */
uint64  timestamp_startup    ();        // µseconds
uint64  timestamp_realtime   ();        // µseconds
uint64  timestamp_benchmark  ();        // nseconds
uint64  timestamp_resolution ();        // nseconds

/* --- file/path functionality --- */
namespace Path {
String  dirname         (const String &path);
String  basename        (const String &path);
String  abspath         (const String &path, const String &incwd = "");
bool    isabs           (const String &path);
bool    isdirname       (const String &path);
String  skip_root       (const String &path);
String  join            (const String &frag0, const String &frag1,
                         const String &frag2 = "", const String &frag3 = "",
                         const String &frag4 = "", const String &frag5 = "",
                         const String &frag6 = "", const String &frag7 = "",
                         const String &frag8 = "", const String &frag9 = "",
                         const String &frag10 = "", const String &frag11 = "",
                         const String &frag12 = "", const String &frag13 = "",
                         const String &frag14 = "", const String &frag15 = "");
bool    check           (const String &file,
                         const String &mode);
bool    equals          (const String &file1,
                         const String &file2);
char*   memread         (const String &filename,
                         size_t       *lengthp);
void    memfree         (char         *memread_mem);
String  cwd             ();
String       vpath_find        (const String &file, const String &mode = "e");
String       searchpath_find  (const String &searchpath, const String &file, const String &mode = "e");
StringVector searchpath_split (const String &searchpath);
extern const String     dir_separator;         /* 1char */
extern const String     searchpath_separator;  /* 1char */
} // Path

/* --- url handling --- */
void url_show                   (const char           *url);
void url_show_with_cookie       (const char           *url,
                                 const char           *url_title,
                                 const char           *cookie);
bool url_test_show              (const char           *url);
bool url_test_show_with_cookie  (const char	      *url,
                                 const char           *url_title,
                                 const char           *cookie);

/* --- cleanup registration --- */
uint cleanup_add                (uint                  timeout_ms,
                                 void                (*destroy_data) (void*),
                                 void                 *data);
void cleanup_force_handlers     (void);

/* --- memory utils --- */
void* malloc_aligned            (size_t                total_size,
                                 size_t                alignment,
                                 uint8               **free_pointer);

/* --- Id Allocator --- */
class IdAllocator : protected NonCopyable {
protected:
  explicit            IdAllocator ();
public:
  virtual            ~IdAllocator ();
  virtual uint        alloc_id    () = 0;
  virtual void        release_id  (uint unique_id) = 0;
  virtual bool        seen_id     (uint unique_id) = 0;
  static IdAllocator* _new        (uint startval = 1);
};

/* --- C++ demangling --- */
char*   cxx_demangle	        (const char  *mangled_identifier);

/* --- zintern support --- */
uint8*  zintern_decompress      (unsigned int          decompressed_size,
                                 const unsigned char  *cdata,
                                 unsigned int          cdata_size);
void    zintern_free            (uint8                *dc_data);

/* --- template errors --- */
namespace TEMPLATE_ERROR {
// to error out, call invalid_type<YourInvalidType>();
template<typename Type> void invalid_type () { bool force_compiler_error = void (0); }
// to error out, derive from InvalidType<YourInvalidType>
template<typename Type> class InvalidType;
}

/* --- Deletable --- */
/**
 * Deletable is a virtual base class that can be derived from (usually with
 * public virtual) to ensure an object has a vtable and a virtual destructor.
 * Also, it allows deletion hooks to be called during the objects destructor,
 * by deriving from Rapicorn::Deletable::DeletionHook. No extra per-object space is
 * consumed to allow deletion hooks, which makes Deletable a suitable base
 * type for classes that may or may not need this feature (e.g. objects that
 * can but often aren't used for signal handler connections).
 */
struct Deletable : public virtual VirtualTypeid {
  /**
   * DeletionHook is the base implementation class for hooks which are hooked
   * up into the deletion phase of a Rapicorn::Deletable.
   */
  class DeletionHook {
    DeletionHook    *prev;
    DeletionHook    *next;
    friend class Deletable;
  protected:
    virtual     ~DeletionHook          (); /* { if (deletable) deletable_remove_hook (deletable); deletable = NULL; } */
    virtual void monitoring_deletable  (Deletable &deletable) = 0;
    virtual void dismiss_deletable     () = 0;
  public:
    explicit     DeletionHook          () : prev (NULL), next (NULL) {}
    bool         deletable_add_hook    (void      *any)              { return false; }
    bool         deletable_add_hook    (Deletable *deletable);
    bool         deletable_remove_hook (void      *any)              { return false; }
    bool         deletable_remove_hook (Deletable *deletable);
  };
private:
  void           add_deletion_hook     (DeletionHook *hook);
  void           remove_deletion_hook  (DeletionHook *hook);
protected:
  void           invoke_deletion_hooks ();
  virtual       ~Deletable             ();
};

/* --- ReferenceCountable --- */
class ReferenceCountable : public virtual Deletable {
  volatile mutable uint32 ref_field;
  static const uint32     FLOATING_FLAG = 1 << 31;
  static void             stackcheck (const void*);
  inline bool             ref_cas (uint32 oldv, uint32 newv) const
  { return __sync_bool_compare_and_swap (&ref_field, oldv, newv); }
  inline uint32           ref_get() const
  { return __sync_fetch_and_add (&ref_field, 0); }
protected:
  inline uint32
  ref_count() const
  {
    return ref_get() & ~FLOATING_FLAG;
  }
public:
  ReferenceCountable (uint allow_stack_magic = 0) :
    ref_field (FLOATING_FLAG + 1)
  {
    if (allow_stack_magic != 0xbadbad)
      stackcheck (this);
  }
  bool
  floating() const
  {
    return 0 != (ref_get() & FLOATING_FLAG);
  }
  void
  ref() const
  {
    // fast-path: use one atomic op and deferred checks
    uint32 old_ref = __sync_fetch_and_add (&ref_field, 1);
    uint32 new_ref = old_ref + 1;                       // ...and_add (,1)
    RAPICORN_ASSERT (old_ref & ~FLOATING_FLAG);         // check dead objects
    RAPICORN_ASSERT (new_ref & ~FLOATING_FLAG);         // check overflow
  }
  void
  ref_sink() const
  {
    ref();
    uint32 old_ref = ref_get();
    uint32 new_ref = old_ref & ~FLOATING_FLAG;
    if (RAPICORN_UNLIKELY (old_ref != new_ref))
      {
        while (RAPICORN_UNLIKELY (!ref_cas (old_ref, new_ref)))
          {
            old_ref = ref_get();
            new_ref = old_ref & ~FLOATING_FLAG;
          }
        if (old_ref & FLOATING_FLAG)
          unref();
      }
  }
  bool
  finalizing() const
  {
    return ref_count() < 1;
  }
  void
  unref() const
  {
    uint32 old_ref = ref_field; // skip read-barrier for fast-path
    if (RAPICORN_LIKELY (old_ref & ~(FLOATING_FLAG | 1)) && // old_ref >= 2
        RAPICORN_LIKELY (ref_cas (old_ref, old_ref - 1)))
      return; // trying fast-path with single atomic op
    old_ref = ref_get();
    if (RAPICORN_UNLIKELY (1 == (old_ref & ~FLOATING_FLAG)))
      {
        ReferenceCountable *self = const_cast<ReferenceCountable*> (this);
        self->pre_finalize();
        old_ref = ref_get();
      }
    RAPICORN_ASSERT (old_ref & ~FLOATING_FLAG);         // old_ref > 1 ?
    while (RAPICORN_UNLIKELY (!ref_cas (old_ref, old_ref - 1)))
      {
        old_ref = ref_get();
        RAPICORN_ASSERT (old_ref & ~FLOATING_FLAG);     // catch underflow
      }
    if (RAPICORN_UNLIKELY (1 == (old_ref & ~FLOATING_FLAG)))
      {
        ReferenceCountable *self = const_cast<ReferenceCountable*> (this);
        self->finalize();
        self->delete_this();                            // usually: delete this;
      }
  }
  void                            ref_diag (const char *msg = NULL) const;
  template<class Obj> static Obj& ref      (Obj &obj) { obj.ref();       return obj; }
  template<class Obj> static Obj* ref      (Obj *obj) { obj->ref();      return obj; }
  template<class Obj> static Obj& ref_sink (Obj &obj) { obj.ref_sink();  return obj; }
  template<class Obj> static Obj* ref_sink (Obj *obj) { obj->ref_sink(); return obj; }
  template<class Obj> static void unref    (Obj &obj) { obj.unref(); }
  template<class Obj> static void unref    (Obj *obj) { obj->unref(); }
protected:
  virtual void pre_finalize       ();
  virtual void finalize           ();
  virtual void delete_this        ();
  virtual     ~ReferenceCountable ();
};
template<class Obj> static Obj& ref      (Obj &obj) { obj.ref();       return obj; }
template<class Obj> static Obj* ref      (Obj *obj) { obj->ref();      return obj; }
template<class Obj> static Obj& ref_sink (Obj &obj) { obj.ref_sink();  return obj; }
template<class Obj> static Obj* ref_sink (Obj *obj) { obj->ref_sink(); return obj; }
template<class Obj> static void unref    (Obj &obj) { obj.unref(); }
template<class Obj> static void unref    (Obj *obj) { obj->unref(); }

/* --- Locatable --- */
class Locatable : public virtual ReferenceCountable {
  mutable uint m_locatable_index;
protected:
  explicit          Locatable         ();
  virtual          ~Locatable         ();
public:
  uint64            locatable_id      () const;
  static Locatable* from_locatable_id (uint64 locatable_id);
};

/* --- Binary Lookups --- */
template<typename RandIter, class Cmp, typename Arg, int case_lookup_or_sibling_or_insertion>
static inline std::pair<RandIter,bool>
binary_lookup_fuzzy (RandIter  begin,
                     RandIter  end,
                     Cmp       cmp_elements,
                     const Arg &arg)
{
  RandIter current = end;
  size_t n_elements = end - begin, offs = 0;
  const bool want_lookup = case_lookup_or_sibling_or_insertion == 0;
  // const bool want_sibling = case_lookup_or_sibling_or_insertion == 1;
  const bool want_insertion_pos = case_lookup_or_sibling_or_insertion > 1;
  ssize_t cmp = 0;
  while (offs < n_elements)
    {
      size_t i = (offs + n_elements) >> 1;
      current = begin + i;
      cmp = cmp_elements (arg, *current);
      if (cmp == 0)
        return want_insertion_pos ? std::make_pair (current, true) : std::make_pair (current, /*ignored*/ false);
      else if (cmp < 0)
        n_elements = i;
      else /* (cmp > 0) */
        offs = i + 1;
    }
  /* check is last mismatch, cmp > 0 indicates greater key */
  return (want_lookup
          ? std::make_pair (end, /*ignored*/ false)
          : (want_insertion_pos && cmp > 0)
          ? std::make_pair (current + 1, false)
          : std::make_pair (current, false));
}
template<typename RandIter, class Cmp, typename Arg>
static inline std::pair<RandIter,bool>
binary_lookup_insertion_pos (RandIter  begin,
                             RandIter  end,
                             Cmp       cmp_elements,
                             const Arg &arg)
{
  /* return (end,false) for end-begin==0, or return (position,true) for exact match,
   * otherwise return (position,false) where position indicates the location for
   * the key to be inserted (and may equal end).
   */
  return binary_lookup_fuzzy<RandIter,Cmp,Arg,2> (begin, end, cmp_elements, arg);
}
template<typename RandIter, class Cmp, typename Arg>
static inline RandIter
binary_lookup_sibling (RandIter  begin,
                       RandIter  end,
                       Cmp       cmp_elements,
                       const Arg &arg)
{
  /* return end for end-begin==0, otherwise return the exact match element, or,
   * if there's no such element, return the element last visited, which is pretty
   * close to an exact match (will be one off into either direction).
   */
  return binary_lookup_fuzzy<RandIter,Cmp,Arg,1> (begin, end, cmp_elements, arg).first;
}
template<typename RandIter, class Cmp, typename Arg>
static inline RandIter
binary_lookup (RandIter  begin,
               RandIter  end,
               Cmp       cmp_elements,
               const Arg &arg)
{
  /* return end or exact match */
  return binary_lookup_fuzzy<RandIter,Cmp,Arg,0> (begin, end, cmp_elements, arg).first;
}

/* --- generic named data --- */
template<typename Type>
class DataKey {
private:
  /*Copy*/        DataKey    (const DataKey&);
  DataKey&        operator=  (const DataKey&);
public:
  /* explicit */  DataKey    ()                 { }
  virtual Type    fallback   ()                 { Type d = Type(); return d; }
  virtual void    destroy    (Type data)        { /* destruction hook */ }
  virtual        ~DataKey    ()                 {}
};

class DataList {
  class NodeBase {
  protected:
    NodeBase      *next;
    DataKey<void> *key;
    explicit       NodeBase (DataKey<void> *k) : next (NULL), key (k) {}
    virtual       ~NodeBase ();
    friend         class DataList;
  };
  template<typename T>
  class Node : public NodeBase {
    T data;
  public:
    T        get_data ()     { return data; }
    T        swap     (T d)  { T result = data; data = d; return result; }
    virtual ~Node()
    {
      if (key)
        {
          DataKey<T> *dkey = reinterpret_cast<DataKey<T>*> (key);
          dkey->destroy (data);
        }
    }
    explicit Node (DataKey<T> *k,
                   T           d) :
      NodeBase (reinterpret_cast<DataKey<void>*> (k)),
      data (d)
    {}
  };
  NodeBase *nodes;
public:
  DataList() :
    nodes (NULL)
  {}
  template<typename T> void
  set (DataKey<T> *key,
       T           data)
  {
    Node<T> *node = new Node<T> (key, data);
    set_data (node);
  }
  template<typename T> T
  get (DataKey<T> *key) const
  {
    NodeBase *nb = get_data (reinterpret_cast<DataKey<void>*> (key));
    if (nb)
      {
        Node<T> *node = reinterpret_cast<Node<T>*> (nb);
        return node->get_data();
      }
    else
      return key->fallback();
  }
  template<typename T> T
  swap (DataKey<T> *key,
        T           data)
  {
    NodeBase *nb = get_data (reinterpret_cast<DataKey<void>*> (key));
    if (nb)
      {
        Node<T> *node = reinterpret_cast<Node<T>*> (nb);
        return node->swap (data);
      }
    else
      {
        set (key, data);
        return key->fallback();
      }
  }
  template<typename T> T
  swap (DataKey<T> *key)
  {
    NodeBase *nb = rip_data (reinterpret_cast<DataKey<void>*> (key));
    if (nb)
      {
        Node<T> *node = reinterpret_cast<Node<T>*> (nb);
        T d = node->get_data();
        nb->key = NULL; // rip key to prevent data destruction
        delete nb;
        return d;
      }
    else
      return key->fallback();
  }
  template<typename T> void
  del (DataKey<T> *key)
  {
    NodeBase *nb = rip_data (reinterpret_cast<DataKey<void>*> (key));
    if (nb)
      delete nb;
  }
  void clear_like_destructor();
  ~DataList();
private:
  void      set_data (NodeBase      *node);
  NodeBase* get_data (DataKey<void> *key) const;
  NodeBase* rip_data (DataKey<void> *key);
};

/* --- DataListContainer --- */
class DataListContainer {
  DataList data_list;
public: /* generic data API */
  template<typename Type> inline void set_data    (DataKey<Type> *key, Type data) { data_list.set (key, data); }
  template<typename Type> inline Type get_data    (DataKey<Type> *key) const      { return data_list.get (key); }
  template<typename Type> inline Type swap_data   (DataKey<Type> *key, Type data) { return data_list.swap (key, data); }
  template<typename Type> inline Type swap_data   (DataKey<Type> *key)            { return data_list.swap (key); }
  template<typename Type> inline void delete_data (DataKey<Type> *key)            { data_list.del (key); }
};

/* --- BaseObject --- */
class BaseObject : public virtual Locatable, public virtual DataListContainer, protected NonCopyable {
protected:
  class                    InterfaceMatcher;
  template<class C>  class InterfaceMatch;
  static BaseObject* plor_get  (const String &plor_url);
  void               plor_name (const String &plor_name);
public:
  String             plor_name () const;
  virtual void       dispose   ();
};
class NullInterface : std::exception {};

struct BaseObject::InterfaceMatcher : protected NonCopyable {
  explicit      InterfaceMatcher (const String &ident) : m_ident (ident), m_match_found (false) {}
  bool          done             () const { return m_match_found; }
  virtual  bool match            (BaseObject *object, const String &ident = String()) = 0;
protected:
  const String &m_ident;
  bool          m_match_found;
};

template<class C>
struct BaseObject::InterfaceMatch : BaseObject::InterfaceMatcher {
  typedef C&    Result;
  explicit      InterfaceMatch  (const String &ident) : InterfaceMatcher (ident), m_instance (NULL) {}
  C&            result          (bool may_throw) const;
  virtual bool  match           (BaseObject *obj, const String &ident);
protected:
  C            *m_instance;
};
template<class C>
struct BaseObject::InterfaceMatch<C&> : InterfaceMatch<C> {
  explicit      InterfaceMatch  (const String &ident) : InterfaceMatch<C> (ident) {}
};
template<class C>
struct BaseObject::InterfaceMatch<C*> : InterfaceMatch<C> {
  typedef C*    Result;
  explicit      InterfaceMatch  (const String &ident) : InterfaceMatch<C> (ident) {}
  C*            result          (bool may_throw) const { return InterfaceMatch<C>::m_instance; }
};

template<class C> bool
BaseObject::InterfaceMatch<C>::match (BaseObject *obj, const String &ident)
{
  if (!m_instance)
    {
      const String &id = m_ident;
      if (id.empty() || id == ident)
        {
          m_instance = dynamic_cast<C*> (obj);
          m_match_found = m_instance != NULL;
        }
    }
  return m_match_found;
}

template<class C> C&
BaseObject::InterfaceMatch<C>::result (bool may_throw) const
{
  if (!this->m_instance && may_throw)
    throw NullInterface();
  return *this->m_instance;
}

} // Rapicorn

#endif /* __RAPICORN_CORE_UTILITIES_HH__ */
