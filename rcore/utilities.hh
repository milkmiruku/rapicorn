// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_CORE_UTILITIES_HH__
#define __RAPICORN_CORE_UTILITIES_HH__

#include <rcore/cxxaux.hh>
#include <rcore/aida.hh>
#include <string>
#include <vector>
#include <map>

#if !defined __RAPICORN_CORE_HH__ && !defined __RAPICORN_BUILD__
#error Only <rapicorn-core.hh> can be included directly.
#endif

// === Convenience Macro Abbreviations ===
#ifdef RAPICORN_CONVENIENCE
#define DIR_SEPARATOR                   RAPICORN_DIR_SEPARATOR
#define DIR_SEPARATOR_S                 RAPICORN_DIR_SEPARATOR_S
#define SEARCHPATH_SEPARATOR            RAPICORN_SEARCHPATH_SEPARATOR
#define SEARCHPATH_SEPARATOR_S          RAPICORN_SEARCHPATH_SEPARATOR_S
#define __PRETTY_FILE__                 RAPICORN_PRETTY_FILE
//#define STRFUNC()                       RAPICORN_STRFUNC() // currently in cxxaux.hh
#define STRLOC()         RAPICORN_STRLOC()          ///< Produces a const char C string, describing the current code location.
#ifndef assert                                      // guard against previous inclusion of assert.h
#define assert           RAPICORN_ASSERT            ///< Assert @a condition to be true at runtime.
#endif // assert
#define assert_unreached RAPICORN_ASSERT_UNREACHED  ///< Assertion used to label unreachable code.
#define assert_return    RAPICORN_ASSERT_RETURN     ///< Issue an assertion warning and return @a ... if @a condition is false.
#define return_if        RAPICORN_RETURN_IF         ///< Return @a ... if @a condition evaluates to true.
#define return_unless    RAPICORN_RETURN_UNLESS     ///< Return @a ... if @a condition is false.
#define fatal            RAPICORN_FATAL             ///< Abort the program with a fatal error message.
#define critical_unless  RAPICORN_CRITICAL_UNLESS   ///< Issue a critical warning if @a condition is false.
#define critical         RAPICORN_CRITICAL          ///< Issue a critical warning.
#define FIXME            RAPICORN_FIXME             ///< Issue a warning about needed fixups on stderr, for development only.
#define DEBUG            RAPICORN_DEBUG             ///< Conditionally issue a message if debugging is enabled.
#define BREAKPOINT       Rapicorn::breakpoint       ///< Cause a debugging breakpoint, for development only.
#define STARTUP_ASSERT   RAPICORN_STARTUP_ASSERT    ///< Runtime check for @a condition to be true before main() starts.
#endif // RAPICORN_CONVENIENCE

namespace Rapicorn {

// == Convenient stdc++ & Aida Types ==
using std::vector;
using std::map;
typedef std::string String;             ///< Convenience alias for std::string.
typedef vector<String> StringVector;    ///< Convenience alias for a std::vector of std::string.
using Aida::Any;

// == Common (stdc++) Utilities ==
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
String  pretty_file                             (const char *file_dir, const char *file);
#ifdef  __FILE_DIR__
#define RAPICORN_PRETTY_FILE                    (__FILE_DIR__ "/" __FILE__)
#else
#define __FILE_DIR__                            ""
#define RAPICORN_PRETTY_FILE                    (__FILE__)
#endif
#define RAPICORN_STRLOC()                       ((RAPICORN_PRETTY_FILE + ::Rapicorn::String (":") + RAPICORN_STRINGIFY (__LINE__)).c_str()) ///< Return "FILE:LINE"
#define RAPICORN_STRFUNC()                      (std::string (__FUNCTION__).c_str())            ///< Return "FUNCTION()"
#define RAPICORN_STRINGIFY(macro_or_string)     RAPICORN_STRINGIFY_ARG (macro_or_string)        ///< Return stringiified argument

// === Control Flow Helpers ===
#define RAPICORN_RETURN_IF(cond, ...)           do { if (RAPICORN_UNLIKELY (cond)) return __VA_ARGS__; } while (0)
#define RAPICORN_RETURN_UNLESS(cond, ...)       do { if (RAPICORN_LIKELY (cond)) break; return __VA_ARGS__; } while (0)

// === Development Guards ===
#define RAPICORN_FATAL(...)                     do { Rapicorn::debug_fatal (RAPICORN_PRETTY_FILE, __LINE__, __VA_ARGS__); } while (0)
#define RAPICORN_ASSERT(cond)                   do { if (RAPICORN_LIKELY (cond)) break; Rapicorn::debug_fassert (RAPICORN_PRETTY_FILE, __LINE__, #cond); } while (0)
#define RAPICORN_ASSERT_RETURN(cond, ...)       do { if (RAPICORN_LIKELY (cond)) break; Rapicorn::debug_assert (RAPICORN_PRETTY_FILE, __LINE__, #cond); return __VA_ARGS__; } while (0)
#define RAPICORN_ASSERT_UNREACHED()             do { Rapicorn::debug_fassert (RAPICORN_PRETTY_FILE, __LINE__, "code must not be reached"); } while (0)

// === Development Messages ===
#define RAPICORN_CRITICAL_UNLESS(cond)  do { if (RAPICORN_LIKELY (cond)) break; Rapicorn::debug_assert (RAPICORN_PRETTY_FILE, __LINE__, #cond); } while (0)
#define RAPICORN_CRITICAL(...)          do { Rapicorn::debug_critical (RAPICORN_PRETTY_FILE, __LINE__, __VA_ARGS__); } while (0)
#define RAPICORN_FIXME(...)             do { Rapicorn::debug_fixit (RAPICORN_PRETTY_FILE, __LINE__, __VA_ARGS__); } while (0)

// === Conditional Debugging ===
#define RAPICORN_DEBUG(...)             do { if (RAPICORN_UNLIKELY (Rapicorn::_cached_rapicorn_debug)) Rapicorn::rapicorn_debug (NULL, RAPICORN_PRETTY_FILE, __LINE__, __VA_ARGS__); } while (0)
#define RAPICORN_KEY_DEBUG(key,...)     do { if (RAPICORN_UNLIKELY (Rapicorn::_cached_rapicorn_debug)) Rapicorn::rapicorn_debug (key, RAPICORN_PRETTY_FILE, __LINE__, __VA_ARGS__); } while (0)

// === Debugging Functions ===
bool debug_envkey_check   (const char*, const char*, volatile bool* = NULL);
void debug_envkey_message (const char*, const char*, const char*, int, const char*, va_list, volatile bool* = NULL);

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
void        debug_backtrace_snapshot (size_t key);
String      debug_backtrace_showshot (size_t key);

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
void debug_assert         (const char*, int, const char*);
void debug_fassert        (const char*, int, const char*) RAPICORN_NORETURN;
void debug_fatal          (const char*, int, const char*, ...) RAPICORN_PRINTF (3, 4) RAPICORN_NORETURN;
void debug_critical       (const char*, int, const char*, ...) RAPICORN_PRINTF (3, 4);
void debug_fixit          (const char*, int, const char*, ...) RAPICORN_PRINTF (3, 4);
void rapicorn_debug       (const char*, const char*, int, const char*, ...) RAPICORN_PRINTF (4, 5);
extern bool volatile _cached_rapicorn_debug; ///< Caching flag to inhibit useless rapicorn_debug() calls.

// === Macro Implementations ===
#define RAPICORN_STRINGIFY_ARG(arg)     #arg
#define RAPICORN_STARTUP_ASSERTi(e, _N) namespace { static struct _N { inline _N() { RAPICORN_ASSERT (e); } } _N; }
#define RAPICORN_STARTUP_ASSERT(expr)   RAPICORN_STARTUP_ASSERTi (expr, RAPICORN_CPP_PASTE2 (StartupAssertion, __LINE__))
#if (defined __i386__ || defined __x86_64__)
inline void breakpoint() { __asm__ __volatile__ ("int $03"); }
#elif defined __alpha__ && !defined __osf__
inline void breakpoint() { __asm__ __volatile__ ("bpt"); }
#else   // !__i386__ && !__alpha__
inline void breakpoint() { __builtin_trap(); }
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
  const String msg_;
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
  RAPICORN_CLASS_NON_COPYABLE (DataList);
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

} // Rapicorn

#endif /* __RAPICORN_CORE_UTILITIES_HH__ */
