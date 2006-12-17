/* Birnet
 * Copyright (C) 2005-2006 Tim Janik
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
#ifndef __BIRNET_UTILS_XX_HH__
#define __BIRNET_UTILS_XX_HH__

#include <birnet/birnetcdefs.h>
#include <string>
#include <vector>
#include <map>

namespace Birnet {

/* --- short integer types --- */
typedef BirnetUInt8   uint8;
typedef BirnetUInt16  uint16;
typedef BirnetUInt32  uint32;
typedef BirnetUInt64  uint64;
typedef BirnetInt8    int8;
typedef BirnetInt16   int16;
typedef BirnetInt32   int32;
typedef BirnetInt64   int64;
typedef BirnetUnichar unichar;

/* --- convenient stdc++ types --- */
typedef std::string String;
using std::vector;
using std::map;       
class VirtualTypeid {
protected:
  virtual      ~VirtualTypeid      ();
public:
  String        typeid_name        ();
  String        typeid_pretty_name ();
  static String cxx_demangle       (const char *mangled_identifier);
};

/* --- implement assertion macros --- */
#ifndef BIRNET__RUNTIME_PROBLEM
#define BIRNET__RUNTIME_PROBLEM(ErrorWarningReturnAssertNotreach,domain,file,line,funcname,...) \
        ::Birnet::birnet_runtime_problem (ErrorWarningReturnAssertNotreach, domain, file, line, funcname, __VA_ARGS__)
#endif
void birnet_runtime_problem  (char        ewran_tag,
                              const char *domain,
                              const char *file,
                              int         line,
                              const char *funcname,
                              const char *msgformat,
                              ...) BIRNET_PRINTF (6, 7);
void birnet_runtime_problemv (char        ewran_tag,
                              const char *domain,
                              const char *file,
                              int         line,
                              const char *funcname,
                              const char *msgformat,
                              va_list     msgargs);

/* --- private copy constructor and assignment operator --- */
#define BIRNET_PRIVATE_CLASS_COPY(Class)        private: Class (const Class&); Class& operator= (const Class&);
#ifdef  _BIRNET_SOURCE_EXTENSIONS
#define PRIVATE_CLASS_COPY                      BIRNET_PRIVATE_CLASS_COPY
#endif  /* _BIRNET_SOURCE_EXTENSIONS */

/* --- initialization --- */
typedef BirnetInitValue    InitValue;
typedef BirnetInitSettings InitSettings;
InitSettings init_settings     ();
void         birnet_init       (int        *argcp,
                                char     ***argvp,
                                const char *app_name,
                                InitValue   ivalues[] = NULL);
bool         init_value_bool   (InitValue  *value);
double       init_value_double (InitValue  *value);
int64        init_value_int    (InitValue  *value);

/* --- initialization hooks --- */
class InitHook {
  typedef void (*InitHookFunc) (void);
  InitHook    *next;
  int          priority;
  InitHookFunc hook;
  BIRNET_PRIVATE_CLASS_COPY (InitHook);
  static void  invoke_hooks (void);
public:
  explicit InitHook (InitHookFunc _func,
                     int          _priority = 0);
};

/* --- assertions/warnings/errors --- */
void    raise_sigtrap           ();
#if (defined __i386__ || defined __x86_64__) && defined __GNUC__ && __GNUC__ >= 2
extern inline void BREAKPOINT() { __asm__ __volatile__ ("int $03"); }
#elif defined __alpha__ && !defined __osf__ && defined __GNUC__ && __GNUC__ >= 2
extern inline void BREAKPOINT() { __asm__ __volatile__ ("bpt"); }
#else   /* !__i386__ && !__alpha__ */
extern inline void BREAKPOINT() { raise_sigtrap(); }
#endif  /* __i386__ */

/* --- threading implementaiton bit --- */
extern BirnetThreadTable ThreadTable; /* private, provided by birnetthreadimpl.cc */

/* --- string functionality --- */
String  			string_tolower           (const String &str);
String  			string_toupper           (const String &str);
String  			string_totitle           (const String &str);
String  			string_printf            (const char *format, ...) BIRNET_PRINTF (1, 2);
String  			string_vprintf           (const char *format, va_list vargs);
String  			string_strip             (const String &str);
bool    			string_to_bool           (const String &string);
String  			string_from_bool         (bool value);
uint64  			string_to_uint           (const String &string, uint base = 10);
String  			string_from_uint         (uint64 value);
bool    			string_has_int           (const String &string);
int64   			string_to_int            (const String &string, uint base = 10);
String  			string_from_int          (int64 value);
String  			string_from_float        (float value);
double  			string_to_double         (const String &string);
String                          string_from_double       (double value);
inline String                   string_from_float        (double value)         { return string_from_double (value); }
inline double                   string_to_float          (const String &string) { return string_to_double (string); }
template<typename Type> Type    string_to_type           (const String &string);
template<typename Type> String  string_from_type         (Type          value);
template<> inline double        string_to_type<double>   (const String &string) { return string_to_double (string); }
template<> inline String        string_from_type<double> (double         value) { return string_from_double (value); }
template<> inline float         string_to_type<float>    (const String &string) { return string_to_float (string); }
template<> inline String        string_from_type<float>  (float         value)  { return string_from_float (value); }
template<> inline bool          string_to_type<bool>     (const String &string) { return string_to_bool (string); }
template<> inline String        string_from_type<bool>   (bool         value)   { return string_from_bool (value); }
template<> inline int16         string_to_type<int16>    (const String &string) { return string_to_int (string); }
template<> inline String        string_from_type<int16>  (int16         value)  { return string_from_int (value); }
template<> inline uint16        string_to_type<uint16>   (const String &string) { return string_to_uint (string); }
template<> inline String        string_from_type<uint16> (uint16        value)  { return string_from_uint (value); }
template<> inline int           string_to_type<int>      (const String &string) { return string_to_int (string); }
template<> inline String        string_from_type<int>    (int         value)    { return string_from_int (value); }
template<> inline uint          string_to_type<uint>     (const String &string) { return string_to_uint (string); }
template<> inline String        string_from_type<uint>   (uint           value) { return string_from_uint (value); }
template<> inline int64         string_to_type<int64>    (const String &string) { return string_to_int (string); }
template<> inline String        string_from_type<int64>  (int64         value)  { return string_from_int (value); }
template<> inline uint64        string_to_type<uint64>   (const String &string) { return string_to_uint (string); }
template<> inline String        string_from_type<uint64> (uint64         value) { return string_from_uint (value); }
vector<double>                  string_to_vector         (const String         &string);
String                          string_from_vector       (const vector<double> &dvec,
                                                          const String         &delim = " ");
String  			string_from_errno        (int         errno_val);
bool                            string_is_uuid           (const String &uuid_string); /* check uuid formatting */
int                             string_cmp_uuid          (const String &uuid_string1,
                                                          const String &uuid_string2); /* -1=smaller, 0=equal, +1=greater (assuming valid uuid strings) */

/* --- file/path functionality --- */
namespace Path {
const String    dirname   (const String &path);
const String    basename  (const String &path);
bool            isabs     (const String &path);
const String    skip_root (const String &path);
const String    join      (const String &frag0, const String &frag1,
                           const String &frag2 = "", const String &frag3 = "",
                           const String &frag4 = "", const String &frag5 = "",
                           const String &frag6 = "", const String &frag7 = "",
                           const String &frag8 = "", const String &frag9 = "",
                           const String &frag10 = "", const String &frag11 = "",
                           const String &frag12 = "", const String &frag13 = "",
                           const String &frag14 = "", const String &frag15 = "");
bool            check     (const String &file,
                           const String &mode);
bool            equals    (const String &file1,
                           const String &file2);
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

/* --- string utils --- */
void memset4		        (uint32              *mem,
                                 uint32               filler,
                                 uint                 length);
/* --- memory utils --- */
void* malloc_aligned            (size_t                total_size,
                                 size_t                alignment,
                                 uint8               **free_pointer);

/* --- C++ demangling --- */
char*   cxx_demangle	        (const char  *mangled_identifier); /* in birnetutilsxx.cc */

/* --- zintern support --- */
uint8*  zintern_decompress      (unsigned int          decompressed_size,
                                 const unsigned char  *cdata,
                                 unsigned int          cdata_size);

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
 * by deriving from Birnet::Deletable::DeletionHook. No extra per-object space is
 * consumed to allow deletion hooks, which makes Deletable a suitable base
 * type for classes that may or may not need this feature (e.g. objects that
 * can but often aren't used for signal handler connections).
 */
struct Deletable : public virtual VirtualTypeid {
  /**
   * DeletionHook is the base implementation class for hooks which are hooked
   * up into the deletion phase of a Birnet::Deletable.
   */
  class DeletionHook {
    DeletionHook    *prev;
    DeletionHook    *next;
    friend class Deletable;
  protected:
    virtual     ~DeletionHook          ();
  public:
    explicit     DeletionHook          () : prev (NULL), next (NULL) {}
    virtual void deletable_dispose     (Deletable &deletable) = 0;
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

/* --- ReferenceCountImpl --- */
class ReferenceCountImpl : public virtual Deletable
{
  volatile mutable uint32 ref_field;
  static const uint32     FLOATING_FLAG = 1 << 31;
  inline bool
  ref_cas (uint32       oldv,
           uint32       newv) const
  {
    return ThreadTable.atomic_uint_cas (&ref_field, oldv, newv);
  }
  inline uint32
  ref_get() const
  {
    return ThreadTable.atomic_uint_get (&ref_field);
  }
protected:
  inline uint32
  ref_count() const
  {
    return ref_get() & ~FLOATING_FLAG;
  }
public:
  ReferenceCountImpl() :
    ref_field (FLOATING_FLAG + 1)
  {}
  bool
  floating() const
  {
    return 0 != (ref_get() & FLOATING_FLAG);
  }
  void
  ref() const
  {
    BIRNET_ASSERT (ref_count() > 0);
    uint32 old_ref, new_ref;
    do {
      old_ref = ref_get();
      new_ref = old_ref + 1;
      BIRNET_ASSERT (new_ref & ~FLOATING_FLAG); /* catch overflow */
    } while (!ref_cas (old_ref, new_ref));
  }
  void
  ref_sink() const
  {
    BIRNET_ASSERT (ref_count() > 0);
    ref();
    uint32 old_ref, new_ref;
    do {
      old_ref = ref_get();
      new_ref = old_ref & ~FLOATING_FLAG;
    } while (!ref_cas (old_ref, new_ref));
    if (old_ref & FLOATING_FLAG)
      unref();
  }
  bool
  finalizing() const
  {
    return ref_count() < 1;
  }
  void
  unref() const
  {
    BIRNET_ASSERT (ref_count() > 0);
    uint32 old_ref, new_ref;
    do {
      old_ref = ref_get();
      BIRNET_ASSERT (old_ref & ~FLOATING_FLAG); /* catch underflow */
      new_ref = old_ref - 1;
    } while (!ref_cas (old_ref, new_ref));
    if (0 == (new_ref & ~FLOATING_FLAG))
      {
        ReferenceCountImpl *self = const_cast<ReferenceCountImpl*> (this);
        self->finalize();
        self->delete_this(); // effectively: delete this;
      }
  }
  void                            ref_diag (const char *msg = NULL) const;
  template<class Obj> static Obj& ref      (Obj &obj) { obj.ref();       return obj; }
  template<class Obj> static Obj* ref      (Obj *obj) { obj->ref();      return obj; }
  template<class Obj> static Obj& ref_sink (Obj &obj) { obj.ref_sink();  return obj; }
  template<class Obj> static Obj* ref_sink (Obj *obj) { obj->ref_sink(); return obj; }
  template<class Obj> static void unref    (Obj &obj) { obj.unref(); }
  template<class Obj> static void unref    (Obj *obj) { obj->unref(); }
  template<class Obj> static void sink     (Obj &obj) { obj.ref_sink(); obj.unref(); }
  template<class Obj> static void sink     (Obj *obj) { obj->ref_sink(); obj->unref(); }
protected:
  virtual void finalize           ();
  virtual void delete_this        ();
  virtual     ~ReferenceCountImpl ();
};
template<class Obj> static Obj& ref      (Obj &obj) { obj.ref();       return obj; }
template<class Obj> static Obj* ref      (Obj *obj) { obj->ref();      return obj; }
template<class Obj> static Obj& ref_sink (Obj &obj) { obj.ref_sink();  return obj; }
template<class Obj> static Obj* ref_sink (Obj *obj) { obj->ref_sink(); return obj; }
template<class Obj> static void unref    (Obj &obj) { obj.unref(); }
template<class Obj> static void unref    (Obj *obj) { obj->unref(); }
template<class Obj> static void sink     (Obj &obj) { obj.ref_sink(); obj.unref(); }
template<class Obj> static void sink     (Obj *obj) { obj->ref_sink(); obj->unref(); }

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

/* --- implementation --- */
void _birnet_init_threads (void);

} // Birnet

#endif /* __BIRNET_UTILS_XX_HH__ */
/* vim:set ts=8 sts=2 sw=2: */
