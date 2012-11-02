// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_CORE_UTILITIES_HH__
#define __RAPICORN_CORE_UTILITIES_HH__

#include <rcore/cxxaux.hh>
#include <string>
#include <vector>
#include <map>
#include <aidacc/runtime.hh>

#if !defined __RAPICORN_CORE_HH__ && !defined __RAPICORN_BUILD__
#error Only <rapicorn-core.hh> can be included directly.
#endif

// === Standard Macro Definitions ===
#ifndef SIZE_T_MAX
#define SIZE_T_MAX              (~size_t (0))
#define SSIZE_T_MAX             (ssize_t (SIZE_T_MAX / 2))
#endif

// === Convenience Macro Abbreviations ===
#ifdef RAPICORN_CONVENIENCE
#define DIR_SEPARATOR                   RAPICORN_DIR_SEPARATOR
#define DIR_SEPARATOR_S                 RAPICORN_DIR_SEPARATOR_S
#define SEARCHPATH_SEPARATOR            RAPICORN_SEARCHPATH_SEPARATOR
#define SEARCHPATH_SEPARATOR_S          RAPICORN_SEARCHPATH_SEPARATOR_S
//#define STRFUNC()                       RAPICORN_STRFUNC() // currently in cxxaux.hh
#define STRLOC()                        RAPICORN_STRLOC()
#define return_if                       RAPICORN_RETURN_IF
#define return_unless                   RAPICORN_RETURN_UNLESS
#define assert                          RAPICORN_ASSERT
#define assert_unreached                RAPICORN_ASSERT_UNREACHED
#define assert_return                   RAPICORN_ASSERT_RETURN
#define fatal                           RAPICORN_FATAL
#define critical_unless                 RAPICORN_CRITICAL_UNLESS
#define critical                        RAPICORN_CRITICAL
#define FIXME                           RAPICORN_FIXME
#define DEBUG                           RAPICORN_DEBUG
#define KEY_DEBUG                       RAPICORN_KEY_DEBUG
#define BREAKPOINT                      Rapicorn::breakpoint
#define STARTUP_ASSERT                  RAPICORN_STARTUP_ASSERT
#endif // RAPICORN_CONVENIENCE

namespace Rapicorn {
using namespace Aida;

/* --- short integer types --- */
typedef RapicornUInt8   uint8;          ///< An 8-bit unsigned integer.
typedef RapicornUInt16  uint16;         ///< A 16-bit unsigned integer.
typedef RapicornUInt32  uint32;         ///< A 32-bit unsigned integer.
typedef RapicornUInt64  uint64;         ///< A 64-bit unsigned integer, use "%llu" in format strings.
typedef RapicornInt8    int8;           ///< An 8-bit signed integer.
typedef RapicornInt16   int16;          ///< A 16-bit signed integer.
typedef RapicornInt32   int32;          ///< A 32-bit signed integer.
typedef RapicornInt64   int64;          ///< A 64-bit unsigned integer, use "%lld" in format strings.
typedef RapicornUnichar unichar;        ///< A 32-bit unsigned integer used in Unicode character contexts.

/* --- convenient stdc++ types --- */
using std::vector;
using std::map;
typedef std::string String;             ///< Convenience alias for std::string.
typedef vector<String> StringVector;    ///< Convenience alias for a std::vector of std::string.

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

// === User Messages ==
class UserSource;
void    user_notice     (const UserSource &source, const char *format, ...) RAPICORN_PRINTF (2, 3);
void    user_warning    (const UserSource &source, const char *format, ...) RAPICORN_PRINTF (2, 3);

struct UserSource {
  String line, filename; int lineno;
  UserSource (String _filename, int _lineno = 0);
};

// === source location strings ===
#define RAPICORN_STRLOC()                       (__FILE__ ":" RAPICORN_STRINGIFY (__LINE__))    ///< Return "FILE:LINE"
#define RAPICORN_STRFUNC()                      (std::string (__FUNCTION__).c_str())            ///< Return "FUNCTION()"
#define RAPICORN_STRINGIFY(macro_or_string)     RAPICORN_STRINGIFY_ARG (macro_or_string)        ///< Return stringiified argument

// === Control Flow Helpers ===
#define RAPICORN_RETURN_IF(cond, ...)           do { if (RAPICORN_UNLIKELY (cond)) return __VA_ARGS__; } while (0)
#define RAPICORN_RETURN_UNLESS(cond, ...)       do { if (RAPICORN_LIKELY (cond)) break; return __VA_ARGS__; } while (0)

// === Development Guards ===
#define RAPICORN_FATAL(...)                     do { Rapicorn::debug_fatal (__FILE__, __LINE__, __VA_ARGS__); } while (0)
#define RAPICORN_ASSERT(cond)                   do { if (RAPICORN_LIKELY (cond)) break; Rapicorn::debug_fassert (__FILE__, __LINE__, #cond); } while (0)
#define RAPICORN_ASSERT_RETURN(cond, ...)       do { if (RAPICORN_LIKELY (cond)) break; Rapicorn::debug_assert (__FILE__, __LINE__, #cond); return __VA_ARGS__; } while (0)
#define RAPICORN_ASSERT_UNREACHED()             do { Rapicorn::debug_fassert (__FILE__, __LINE__, "code must not be reached"); } while (0)

// === Development Messages ===
#define RAPICORN_CRITICAL_UNLESS(cond)  do { if (RAPICORN_LIKELY (cond)) break; Rapicorn::debug_assert (__FILE__, __LINE__, #cond); } while (0)
#define RAPICORN_CRITICAL(...)          do { Rapicorn::debug_critical (__FILE__, __LINE__, __VA_ARGS__); } while (0)
#define RAPICORN_FIXME(...)             do { Rapicorn::debug_fixit (__FILE__, __LINE__, __VA_ARGS__); } while (0)

// === Conditional Debugging ===
#define RAPICORN_DEBUG(...)             do { if (Rapicorn::debug_enabled()) Rapicorn::debug_general (__FILE__, __LINE__, __VA_ARGS__); } while (0)
#define RAPICORN_KEY_DEBUG(key,...)     do { const char *__k_ = key; if (Rapicorn::debug_enabled (__k_)) Rapicorn::debug_keymsg (__FILE__, __LINE__, __k_, __VA_ARGS__); } while (0)

// === Debugging Functions (internal) ===
vector<String> pretty_backtrace (uint level = 0, size_t *parent_addr = NULL) __attribute__ ((noinline));
extern bool _debug_flag, _devel_flag;
inline bool devel_enabled     () { return RAPICORN_UNLIKELY (_devel_flag); }
bool        debug_key_enabled (const char *key);
inline bool debug_enabled     () { return RAPICORN_UNLIKELY (_debug_flag); }
inline bool debug_enabled     (const char *key) { return RAPICORN_UNLIKELY (_debug_flag) && debug_key_enabled (key); }
void        debug_configure   (const String &options);
String      debug_confstring  (const String &option, const String &vdefault = "");
bool        debug_confbool    (const String &option, bool vdefault = false);
int64       debug_confnum     (const String &option, int64 vdefault = 0);
String      debug_help        ();
const char* strerror    (void);         // simple wrapper for strerror (errno)
const char* strerror    (int errnum);   // wrapper for ::strerror
class DebugEntry {
  static void dbe_list  (DebugEntry*, int);
public:
  const char *const key, *const blurb;
  String      confstring  (const String &vdefault = "") { return debug_confstring (key, vdefault); }
  bool        confbool    (bool vdefault = false)       { return debug_confbool (key, vdefault); }
  int64       confnum     (int64 vdefault = 0)          { return debug_confnum (key, vdefault); }
  /*dtor*/   ~DebugEntry  ()                            { dbe_list (this, -1); }
  explicit    DebugEntry  (const char *const dkey, const char *const dblurb = NULL) :
    key (dkey), blurb (dblurb)
  { dbe_list (this, +1); }
};

// implementation prototypes
void        debug_assert     (const char*, int, const char*);
void        debug_fassert    (const char*, int, const char*) RAPICORN_NORETURN;
void        debug_fatal      (const char*, int, const char*, ...) RAPICORN_PRINTF (3, 4) RAPICORN_NORETURN;
void        debug_critical   (const char*, int, const char*, ...) RAPICORN_PRINTF (3, 4);
void        debug_fixit      (const char*, int, const char*, ...) RAPICORN_PRINTF (3, 4);
void        debug_general    (const char*, int, const char*, ...) RAPICORN_PRINTF (3, 4);
void        debug_keymsg     (const char*, int, const char*, const char*, ...) RAPICORN_PRINTF (4, 5);

// === Macro Implementations ===
#define RAPICORN_STRINGIFY_ARG(arg)     #arg
#define RAPICORN_STARTUP_ASSERTi(e, _N) namespace { static struct _N { inline _N() { RAPICORN_ASSERT (e); } } _N; }
#define RAPICORN_STARTUP_ASSERT(expr)   RAPICORN_STARTUP_ASSERTi (expr, RAPICORN_CPP_PASTE2 (StartupAssertion, __LINE__))
#ifdef  __SOURCE_COMPONENT__
#define RAPICORN__SOURCE_COMPONENT__            __SOURCE_COMPONENT__
#elif   defined __BASE_FILE__
#define RAPICORN__SOURCE_COMPONENT__            __BASE_FILE__
#else
#define RAPICORN__SOURCE_COMPONENT__            __FILE__
#endif
#if (defined __i386__ || defined __x86_64__)
inline void breakpoint() { __asm__ __volatile__ ("int $03"); }
#elif defined __alpha__ && !defined __osf__
inline void breakpoint() { __asm__ __volatile__ ("bpt"); }
#else   // !__i386__ && !__alpha__
inline void breakpoint() { __builtin_trap(); }
#endif

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
String  timestamp_format     (uint64 stamp);

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

// == ResourceBlob ==
class ResourceBlob {
  String                       m_name;
  size_t                       m_size;
  std::shared_ptr<const uint8> m_data;
  explicit            ResourceBlob (const String &name, size_t dsize, std::shared_ptr<const uint8> shdata);
public:
  String              name   () const { return m_name; }        ///< Provide the name of this resource Blob.
  size_t              size   () const { return m_size; }        ///< Retrieve the size of a Blob in bytes, this may be 0.
  const uint8*        data   () const { return m_data.get(); }  ///< Access the data of a Blob.
  String              string () const;                          ///< Access data as string, automatically strips trailing 0.
  static ResourceBlob load   (const String &res_path);          ///< Provide Blob access to the resource at @a res_path.
  ///@cond
  class Entry {
    Entry            *next;
    const char *const name;
    const char       *const pdata;
    const size_t      psize, dsize;
    friend class ResourceBlob;
    static const Entry* find_entry (const String &res_name);
    static void         reg_add    (Entry*);
  public:
    template <size_t N> Entry (const char *res_name, const char (&idata) [N], size_t data_size = 0) :
      next (NULL), name (res_name), pdata (idata), psize (N), dsize (data_size)
    { reg_add (this); }
    /*dtor*/           ~Entry();
  };
  ///@endcond
};

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
int   fmsb                      (uint64                value) RAPICORN_CONST;

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

template<typename Value> static inline int ///< sort lesser items first
compare_lesser (const Value &v1, const Value &v2)
{
  return -(v1 < v2) | (v2 < v1);
}

template<typename Value> static inline int ///< sort greater items first
compare_greater (const Value &v1, const Value &v2)
{
  return (v1 < v2) | -(v2 < v1);
}

// == Custom Keyed Data ==
/// DataKey objects are used to identify and manage custom data members of DataListContainer objects.
template<typename Type> class DataKey {
private:
  /*Copy*/        DataKey    (const DataKey&);
  DataKey&        operator=  (const DataKey&);
public:
  /* explicit */  DataKey    ()                 { }
  virtual Type    fallback   ()                 { Type d = Type(); return d; }  ///< Return the default value for Type.
  virtual void    destroy    (Type data)        { /* destruction hook */ }      ///< Hook invoked when @a data is deleted.
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

/**
 * By using a DataKey, DataListContainer objects allow storage and retrieval of custom data members in a typesafe fashion.
 * The custom data members will initially default to DataKey::fallback and are deleted by the DataListContainer destructor.
 * Example: @SNIPPET{rcore/tests/datalist.cc, DataListContainer-EXAMPLE}
 */
class DataListContainer {
  DataList data_list;
public: /// @name Accessing custom data members
  /// Assign @a data to the custom keyed data member, deletes any previously set data.
  template<typename Type> inline void set_data    (DataKey<Type> *key, Type data) { data_list.set (key, data); }
  /// Retrieve contents of the custom keyed data member, returns DataKey::fallback if nothing was set.
  template<typename Type> inline Type get_data    (DataKey<Type> *key) const      { return data_list.get (key); }
  /// Swap @a data with the current contents of the custom keyed data member, returns the current contents.
  template<typename Type> inline Type swap_data   (DataKey<Type> *key, Type data) { return data_list.swap (key, data); }
  /// Removes and returns the current contents of the custom keyed data member without deleting it.
  template<typename Type> inline Type swap_data   (DataKey<Type> *key)            { return data_list.swap (key); }
  /// Delete the current contents of the custom keyed data member, invokes DataKey::destroy.
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

// == Helper Macros ==
/// Statically declare a ResourceBlob data variable.
#define RAPICORN_STATIC_RESOURCE_DATA(IDENT)            \
  static const char __Rapicorn_static_resourceD__##IDENT[] __attribute__ ((__aligned__ (2 * sizeof (size_t))))
/// Statically register a ResourceBlob entry, referring a previously declared RAPICORN_STATIC_RESOURCE_DATA(IDENT) variable.
#define RAPICORN_STATIC_RESOURCE_ENTRY(IDENT,PATH,...) \
  static const Rapicorn::ResourceBlob::Entry __Rapicorn_static_resourceE__##IDENT = { PATH, __Rapicorn_static_resourceD__##IDENT, __VA_ARGS__ };

#endif /* __RAPICORN_CORE_UTILITIES_HH__ */
