// CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/
#ifndef __RAPICORN_AIDA_HH__
#define __RAPICORN_AIDA_HH__

#include <rcore/cxxaux.hh>
#include <string>
#include <vector>
#include <memory>               // auto_ptr
#include <stdint.h>             // uint32_t
#include <stdarg.h>
#include <type_traits>
#include <memory>
#include <future>
#include <set>
#include <map>

namespace Rapicorn { namespace Aida {

// == Auxillary macros ==
#define AIDA_CPP_STRINGIFYi(s)  #s // indirection required to expand __LINE__ etc
#define AIDA_CPP_STRINGIFY(s)   AIDA_CPP_STRINGIFYi (s)
#if     __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)
#define AIDA_UNUSED             __attribute__ ((__unused__))
#define AIDA_DEPRECATED         __attribute__ ((__deprecated__))
#define AIDA_NORETURN           __attribute__ ((__noreturn__))
#define AIDA_PRINTF(fix, arx)   __attribute__ ((__format__ (__printf__, fix, arx)))
#define AIDA_BOOLi(expr)        __extension__ ({ bool _plic__bool; if (expr) _plic__bool = 1; else _plic__bool = 0; _plic__bool; })
#define AIDA_ISLIKELY(expr)     __builtin_expect (AIDA_BOOLi (expr), 1)
#define AIDA_UNLIKELY(expr)     __builtin_expect (AIDA_BOOLi (expr), 0)
#define AIDA_ASSERT(expr)       do { if (__builtin_expect (!(expr), 0)) ::Rapicorn::Aida::assertion_error (__FILE__, __LINE__, #expr); } while (0)
#define AIDA_ASSERT_RETURN(expr,...) do { if (__builtin_expect (!!(expr), 1)) break; ::Rapicorn::Aida::assertion_error (__FILE__, __LINE__, #expr); return __VA_ARGS__; } while (0)
#else   // !__GNUC__
#define AIDA_UNUSED
#define AIDA_DEPRECATED
#define AIDA_NORETURN
#define AIDA_PRINTF(fix, arx)
#define AIDA_ISLIKELY(expr)     expr
#define AIDA_UNLIKELY(expr)     expr
#define AIDA_ASSERT(expr)       do { } while (0)
#endif
#define AIDA_LIKELY             AIDA_ISLIKELY

// == Standard Types ==
using std::vector;
typedef std::string String;
using Rapicorn::int32;
using Rapicorn::uint32;
using Rapicorn::int64;
using Rapicorn::uint64;

// == Prototypes ==
class RemoteHandle;

// == TypeKind ==
/// Classification enum for the underlying kind of a TypeCode.
enum TypeKind {
  UNTYPED        = 0,   ///< Type indicator for unused Any instances.
  VOID           = 'v', ///< 'void' type.
  BOOL           = 'b', ///< Boolean type.
  INT32          = 'i', ///< Signed numeric type for 32bit.
  INT64          = 'l', ///< Signed numeric type for 64bit.
  FLOAT64        = 'd', ///< Floating point type of IEEE-754 Double precision.
  STRING         = 's', ///< String type for character sequence in UTF-8 encoding.
  ENUM           = 'E', ///< Enumeration type to represent choices.
  SEQUENCE       = 'Q', ///< Type to form sequences of an other type.
  RECORD         = 'R', ///< Record type containing named fields.
  INSTANCE       = 'C', ///< Interface instance type.
  FUNC           = 'F', ///< Type of methods or signals.
  TYPE_REFERENCE = 'T', ///< Type reference for record fields.
  LOCAL          = 'L', ///< Local object type.
  REMOTE         = 'r', ///< Remote object type.
  ANY            = 'Y', ///< Generic type to hold any other type.
};
const char* type_kind_name (TypeKind type_kind); ///< Obtain TypeKind names as a string.

/// Aida wrapper for enumeration values.
struct EnumValue {
  int64 value;
  const char *ident, *label, *blurb;
  constexpr EnumValue (int64 dflt = 0) : value (dflt), ident (0), label (0), blurb (0) {}
};

// == TypeCode ==
struct TypeCode /// Representation of type information to describe structured type compositions and for the Any class.
{
  /*copy*/              TypeCode        (const TypeCode&);
  /*dtor*/             ~TypeCode        ();
  bool                  operator!=      (const TypeCode&) const;
  bool                  operator==      (const TypeCode&) const;
  TypeCode&             operator=       (const TypeCode&);
  void                  swap            (TypeCode &other);      ///< Swap the contents of @a this and @a other in constant time.
  TypeKind              kind            () const;               ///< Obtain the underlying primitive type kind.
  std::string           kind_name       () const;               ///< Obtain the name of kind().
  std::string           name            () const;               ///< Obtain the type name.
  size_t                aux_count       () const;               ///< Number of items of auxillary data.
  std::string           aux_data        (size_t index) const;   ///< Accessor for auxillary data as key=utf8data string.
  std::string           aux_value       (std::string key) const; ///< Accessor for auxillary data by key as utf8 string.
  std::string           hints           () const;               ///< Obtain "hints" aux_value(), enclosed in two ':'.
  size_t                prerequisite_count () const;            ///< Number of interface prerequisites
  std::string           prerequisite    (size_t index) const;   ///< Obtain prerequisite type names for an interface type.
  size_t                field_count     () const;               ///< Number of fields in a record type.
  TypeCode              field           (size_t index) const;   ///< Obtain field type for a record or sequence type.
  std::string           origin          () const;               ///< Obtain the type origin for a TYPE_REFERENCE (fields).
  TypeCode              resolve         () const;               ///< Returns type code after resolving kind TYPE_REFERENCE.
  bool                  untyped         () const;               ///< Checks whether the TypeCode is undefined.
  std::string           pretty          (const std::string &indent = "") const; ///< Pretty print into a string.
  bool                  enum_combinable () const;               ///< Indicate if multiple enum values are combinable into a mask.
  size_t                enum_count      () const;               ///< Number of enum values for an enum type.
  EnumValue             enum_value      (size_t index) const;   ///< Obtain an enum value as: (value, ident, label, blurb)
  EnumValue             enum_find       (int64 value) const;    ///< Find first enum value equal to @a value.
  EnumValue             enum_find       (const String &name) const; ///< Find first enum value matching @a name.
  String                enum_string     (int64 value) const;    ///< Convert enum value to string, possibly combining identifiers.
  int64                 enum_parse      (const String &value_string, String *error = NULL) const; ///< Parse an enum_string() result.
  template<class E> static
  inline TypeCode       from_enum       ();                     ///< Retrieve a TypeCode by giving the enum C++ type.
  class InternalType;
  class MapHandle;
private: // implementation bits
  explicit              TypeCode        (MapHandle*, InternalType*);
  static TypeCode       notype          (MapHandle*);
  friend class TypeMap;
  MapHandle    *handle_;
  InternalType *type_;
};

class TypeMap /// A TypeMap serves as a repository and loader for IDL type information.
{
  TypeCode::MapHandle  *handle_;
  friend class TypeCode::MapHandle;
  explicit              TypeMap      (TypeCode::MapHandle*);
  static TypeMap        builtins     ();
  static TypeMap        enlist_map   (size_t length, const char *static_type_map, bool global);
public:
  /*copy*/              TypeMap      (const TypeMap&);
  TypeMap&              operator=    (const TypeMap&);
  /*dtor*/             ~TypeMap      ();
  size_t                type_count   () const;                  ///< Number of TypeCode classes in this TypeMap.
  const TypeCode        type         (size_t      index) const; ///< Obtain a TypeCode by index.
  int                   error_status ();                        ///< Obtain errno status from load().
  static TypeMap        load         (std::string file_name);   ///< Load a new TypeMap and register for global lookups.
  static TypeCode       lookup       (std::string name);        ///< Globally lookup a TypeCode by name.
  static TypeMap        load_local   (std::string file_name);   ///< Load a new TypeMap for local lookups only.
  TypeCode              lookup_local (std::string name) const;  ///< Lookup TypeCode within this TypeMap.
  static TypeCode       notype       ();
  template<ssize_t S>
  static void           enlist_map   (const char (&static_type_map)[S]) { enlist_map (S, static_type_map, true); }
};

// == Type Declarations ==
class ObjectBroker;
class BaseConnection;
class ClientConnection;
class ServerConnection;
union FieldUnion;
class FieldBuffer;
class FieldReader;
typedef FieldBuffer* (*DispatchFunc) (FieldReader&);
class PropertyList;
class Property;

// == ImplicitBase ==
class ImplicitBase /// Abstract base interface that all IDL interfaces are implicitely derived from.
{
protected:
  virtual                     ~ImplicitBase       () = 0; // abstract class
  virtual const PropertyList& __aida_properties__ () = 0; ///< Retrieve the list properties of an instance.
  Property*                   __aida_lookup__     (const std::string &property_name);
  bool                        __aida_setter__     (const std::string &property_name, const std::string &value);
  std::string                 __aida_getter__     (const std::string &property_name);
public:
  virtual std::string        __aida_type_name__   () const = 0; ///< Retrieve the IDL type name of an instance.
};

// == Any Type ==
class Any /// Generic value type that can hold values of all other types.
{
  ///@cond
  template<class ANY> struct AnyField : ANY { // We must wrap Any::Field into a template, because "Any" is not yet fully defined.
    std::string name;
    AnyField () = default;
    AnyField (const std::string &_name, const ANY &any) : name (_name) { this->ANY::operator= (any); }
    template<class V>
    AnyField (const std::string &_name, const V &value) : name (_name) { this->operator<<= (value); }
  };
  struct PlaceHolder {
    virtual                      ~PlaceHolder() {}
    virtual PlaceHolder*          clone      () const = 0;
    virtual const std::type_info& type_info  () const = 0;
    virtual bool                  operator== (const PlaceHolder*) const = 0;
  };
  template<class T> struct Holder : PlaceHolder {
    explicit                      Holder    (const T &value) : value_ (value) {}
    virtual PlaceHolder*          clone     () const { return new Holder (value_); }
    virtual const std::type_info& type_info () const { return typeid (T); }
    virtual bool                  operator== (const PlaceHolder *rhs) const
    { const Holder *other = dynamic_cast<const Holder*> (rhs); return other ? eq (this, other) : false; }
    template<class Q = T> static typename std::enable_if<IsComparable<Q>::value, bool>::
    type eq (const Holder *a, const Holder *b) { return a->value_ == b->value_; }
    template<class Q = T> static typename std::enable_if<!IsComparable<Q>::value, bool>::
    type eq (const Holder *a, const Holder *b) { return false; }
    T value_;
  };
  template<class T> T*
  cast () const
  {
    if (kind() != LOCAL)
      return NULL;
    const std::type_info &tt = typeid (T);
    const std::type_info &ht = u_.pholder->type_info();
    if (tt != ht)
      return NULL;
    return &static_cast<Holder<T>*> (u_.pholder)->value_;
  }
  ///@endcond
public:
#ifndef DOXYGEN
  typedef AnyField<Any> Field;  // See DOXYGEN section for the "unwrapped" definition.
#else // DOXYGEN
  struct Field : Any    /// Any::Field is an Any with a std::string @a name attached.
  {
    String name;        ///< The @a name of this Any::Field, as used in e.g. #RECORD types.
    AnyField();         ///< Default initialize Any::Field.
    AnyField (const String &name, const Any &any);                   ///< Initialize Any::Field with @a name and an @a any value.
    template<class V> AnyField (const String &name, const V &value); ///< Initialize Any::Field with a @a value convertible to an Any.
  };
#endif // DOXYGEN
  typedef std::vector<Field> FieldVector; ///< Vector of fields (named Any structures) for use in #RECORD types.
  typedef std::vector<Any> AnyVector;     ///< Vector of Any structures for use in #SEQUENCE types.
protected:
  bool  plain_zero_type (TypeKind kind);
  template<class Rec> static void any_from_record (Any &any, const Rec &record);
private:
  TypeCode type_code_;
  union {
    uint64 vuint64; int64 vint64; double vdouble; Any *vany; AnyVector *vanys; FieldVector *vfields; RemoteHandle *shandle; PlaceHolder *pholder;
    String&       vstring() { return *(String*) this; static_assert (sizeof (String) <= sizeof (*this), "union size"); }
    const String& vstring() const { return *(const String*) this; }
  } u_;
  void    hold    (PlaceHolder*);
  void    ensure  (TypeKind _kind) { if (AIDA_LIKELY (kind() == _kind)) return; rekind (_kind); }
  void    rekind  (TypeKind _kind);
  void    reset   ();
  bool    to_int  (int64 &v, char b) const;
public:
  /*dtor*/ ~Any    ();                                   ///< Any destructor.
  explicit  Any    ();                                   ///< Default initialize Any with no type.
  explicit  Any    (const TypeCode &tc);                 ///< Default initialize Any for a specific type.
  /*copy*/  Any    (const Any &clone);                   ///< Carry out a deep copy of @a clone into a new Any.
  template<class V>
  explicit  Any    (const V &value);                     ///< Initialize Any with a @a value convertible to an Any.
  Any& operator=   (const Any &clone);                   ///< Carry out a deep copy of @a clone into this Any.
  bool operator==  (const Any &clone) const;             ///< Check if Any is exactly equal to @a clone.
  bool operator!=  (const Any &clone) const;             ///< Check if Any is not equal to @a clone, see operator==().
  TypeCode  type   () const { return type_code_; }       ///< Obtain the full TypeCode for the contents of this Any.
  TypeKind  kind   () const { return type_code_.kind(); } ///< Obtain the underlying primitive type kind.
  void      retype (const TypeCode &tc);                 ///< Force Any to assume type @a tc.
  void      swap   (Any           &other);               ///< Swap the contents of @a this and @a other in constant time.
  bool operator>>= (bool          &v) const { int64 d; const bool r = to_int (d, 1); v = d; return r; }
  bool operator>>= (char          &v) const { int64 d; const bool r = to_int (d, 7); v = d; return r; }
  bool operator>>= (unsigned char &v) const { int64 d; const bool r = to_int (d, 8); v = d; return r; }
  bool operator>>= (int32         &v) const { int64 d; const bool r = to_int (d, 31); v = d; return r; }
  bool operator>>= (uint32        &v) const { int64 d; const bool r = to_int (d, 32); v = d; return r; }
  bool operator>>= (LongIffy      &v) const { int64 d; const bool r = to_int (d, 47); v = d; return r; }
  bool operator>>= (ULongIffy     &v) const { int64 d; const bool r = to_int (d, 48); v = d; return r; }
  bool operator>>= (int64         &v) const { int64 d; const bool r = to_int (d, 63); v = d; return r; }
  bool operator>>= (uint64        &v) const { int64 d; const bool r = to_int (d, 64); v = d; return r; }
  bool operator>>= (float         &v) const { double d; const bool r = operator>>= (d); v = d; return r; }
  bool operator>>= (double        &v) const; ///< Extract a floating point number as double if possible.
  bool operator>>= (EnumValue          &v) const; ///< Extract the numeric representation of an EnumValue if possible.
  bool operator>>= (const char        *&v) const { String s; const bool r = operator>>= (s); v = s.c_str(); return r; }
  bool operator>>= (std::string        &v) const; ///< Extract a std::string if possible.
  bool operator>>= (const Any         *&v) const; ///< Extract an Any if possible.
  bool operator>>= (const AnyVector   *&v) const; ///< Extract an AnyVector if possible (sequence type).
  bool operator>>= (const FieldVector *&v) const; ///< Extract a FieldVector if possible (record type).
  bool operator>>= (RemoteHandle       &v);
  String     to_string (const String &field_name = "") const; ///< Retrieve string representation of Any for printouts.
  const Any& as_any   () const { return kind() == ANY ? *u_.vany : *this; } ///< Obtain contents as Any.
  double     as_float () const; ///< Obtain BOOL, INT*, or FLOAT* contents as double float.
  int64      as_int   () const; ///< Obtain BOOL, INT* or FLOAT* contents as integer (yields 1 for non-empty strings).
  String     as_string() const; ///< Obtain BOOL, INT*, FLOAT* or STRING contents as string.
  void operator<<= (bool           v);
  void operator<<= (char           v) { operator<<= (int32 (v)); }
  void operator<<= (unsigned char  v) { operator<<= (int32 (v)); }
  void operator<<= (int32          v);
  void operator<<= (uint32         v) { operator<<= (int64 (v)); }
  void operator<<= (LongIffy       v) { operator<<= (CastIffy (v)); }
  void operator<<= (ULongIffy      v) { operator<<= (UCastIffy (v)); }
  void operator<<= (int64          v); ///< Store a 64bit signed integer.
  void operator<<= (uint64         v); ///< Store a 64bit unsigned integer.
  void operator<<= (float          v) { operator<<= (double (v)); }
  void operator<<= (double         v); ///< Store a double floating point number.
  void operator<<= (const EnumValue   &v); ///< Store the numeric representation of an EnumValue.
  void operator<<= (const char        *v) { operator<<= (std::string (v)); }
  void operator<<= (char              *v) { operator<<= (std::string (v)); }
  void operator<<= (const String      &v); ///< Store a std::string.
  void operator<<= (const Any         &v); ///< Store an Any.
  void operator<<= (const AnyVector   &v); ///< Store a sequence of Any structures (sequence type).
  void operator<<= (const FieldVector &v); ///< Store a sequence of Any::Field structures (record type).
  void operator<<= (const RemoteHandle &v);
  template<class T, typename // SFINAE idiom for operator<<= to match unknown classes
           std::enable_if<(std::is_class<T>::value && !std::is_convertible<T, RemoteHandle>::value)>::type* = nullptr>
  void operator<<= (const T &v)         { hold (new Holder<T> (v)); }   ///< Store arbitrary type class in this Any, see any_cast<>().
  template<class T> friend T*
  any_cast (Any *const any)     ///< Cast Any* into @a T* if possible or return NULL.
  {
    return !any ? NULL : any->cast<T>();
  }
  template<class T> friend const T*
  any_cast (const Any *any)     ///< Cast const Any* into @a const @a T* if possible or return NULL.
  {
    return !any ? NULL : any->cast<T>();
  }
  template<class T> friend T
  any_cast (Any &any)           ///< Cast Any& into @a T if possible or throw a std::bad_cast exception.
  {
    const T *result = any.cast<typename std::remove_reference<T>::type>();
    return result ? *result : throw std::bad_cast();
  }
  template<class T> friend T
  any_cast (const Any &any)     ///< Cast const Any& into @a const @a T if possible or throw a std::bad_cast exception.
  {
    const T *result = any.cast<typename std::add_const<typename std::remove_reference<T>::type>::type>();
    return result ? *result : throw std::bad_cast();
  }
};

// === EventFd ===
class EventFd            /// Wakeup facility for IPC.
{
  int      fds[2];
  void     operator= (const EventFd&) = delete; // no assignments
  explicit EventFd   (const EventFd&) = delete; // no copying
public:
  explicit EventFd   ();
  int      open      (); ///< Opens the eventfd and returns -errno.
  bool     opened    (); ///< Indicates whether eventfd has been opened.
  void     wakeup    (); ///< Wakeup polling end.
  int      inputfd   (); ///< Returns the file descriptor for POLLIN.
  bool     pollin    (); ///< Checks whether events are pending.
  void     flush     (); ///< Clear pending wakeups.
  /*Des*/ ~EventFd   ();
};

// == Type Hash ==
struct TypeHash {
  uint64 typehi, typelo;
  explicit    TypeHash   (uint64 hi, uint64 lo) : typehi (hi), typelo (lo) {}
  explicit    TypeHash   () : typehi (0), typelo (0) {}
  inline bool operator== (const TypeHash &z) const { return typehi == z.typehi && typelo == z.typelo; }
};
typedef std::vector<TypeHash> TypeHashList;

// == Utilities ==
void assertion_error (const char *file, uint line, const char *expr) AIDA_NORETURN;
void fatal_error     (const String &msg) AIDA_NORETURN;
void print_warning   (const String &msg);

// == Type Utilities ==
template<class Y> struct ValueType           { typedef Y T; };
template<class Y> struct ValueType<Y&>       { typedef Y T; };
template<class Y> struct ValueType<const Y&> { typedef Y T; };

// == Message IDs ==
enum MessageId {
  MSGID_NONE           = 0x0000000000000000ULL, ///< No message ID.
  MSGID_ONEWAY_CALL    = 0x1000000000000000ULL, ///< One-way method call (void return).
  MSGID_DISCONNECT     = 0x2000000000000000ULL, ///< Signal destroyed, disconnect all handlers.
  MSGID_EMIT_ONEWAY    = 0x3000000000000000ULL, ///< One-way signal emissions (void return).
  MSGID_DROP_REFS      = 0x4000000000000000ULL, ///< FIXME: Unimplemented.
  // unused            = 0x5
  // unused            = 0x6
  // unused            = 0x7
  MSGID_HELLO_REQUEST  = 0x8000000000000000ULL, ///< Two-way hello message and connection request.
  MSGID_TWOWAY_CALL    = 0x9000000000000000ULL, ///< Two-way method call, returns result message.
  MSGID_CONNECT        = 0xa000000000000000ULL, ///< Signal handler connection/disconnection request.
  MSGID_EMIT_TWOWAY    = 0xb000000000000000ULL, ///< Two-way signal emissions, returns result message.
  MSGID_HELLO_REPLY    = 0xc000000000000000ULL, ///< Reply message for two-way hello request.
  MSGID_CALL_RESULT    = 0xd000000000000000ULL, ///< Result message for two-way call.
  MSGID_CONNECT_RESULT = 0xe000000000000000ULL, ///< Result message for signal handler connection/disconnection.
  MSGID_EMIT_RESULT    = 0xf000000000000000ULL, ///< Result message for two-way signal emissions.
};
inline bool      msgid_has_result (MessageId mid) { return (mid & 0xc000000000000000ULL) == 0x8000000000000000ULL; }
inline bool      msgid_is_result  (MessageId mid) { return (mid & 0xc000000000000000ULL) == 0xc000000000000000ULL; }
inline MessageId msgid_as_result  (MessageId mid) { return MessageId (mid | 0x4000000000000000ULL); }
inline uint64    msgid_mask       (uint64    mid) { return  mid & 0xf000000000000000ULL; }

union IdentifierParts {
  uint64 vuint64;
  struct { // MessageId bits
    uint   sender_connection : 16;
    uint   msg_unused : 16;
    uint   receiver_connection : 16;
    uint   msg_unused2 : 8;
    uint   message_id : 8;
  };
  struct { // OrbID bits
    uint   orbid32 : 32;
    uint   orbid_connection : 16;
    uint   orbid_type_index : 16;
  };
  constexpr IdentifierParts (uint64 vu64) : vuint64 (vu64) {}
  constexpr IdentifierParts (MessageId id, uint sender_con, uint receiver_con) :
    sender_connection (sender_con), msg_unused (0), receiver_connection (receiver_con), message_id (IdentifierParts (id).message_id)
  {}
  struct ORBID {}; // constructor tag
  constexpr IdentifierParts (const ORBID&, uint orbid_con, uint orbid_v32, uint type_index) :
    orbid32 (orbid_v32), orbid_connection (orbid_con), orbid_type_index (type_index) {}
};
constexpr uint64 CONNECTION_MASK = 0x0000ffff;

// == OrbObject ==
/// Internal management structure for objects known to the ORB.
class OrbObject {
  uint64      orbid_;
protected:
  explicit      OrbObject       (uint64 orbid);
public:
  uint64        orbid           ()            { return orbid_; }
};

// == RemoteHandle ==
class RemoteHandle {
  OrbObject     *orbo_;
  template<class Parent> struct NullRemoteHandle : public Parent { TypeHashList __aida_typelist__ () { return TypeHashList(); } };
  typedef NullRemoteHandle<RemoteHandle> NullHandle;
  friend  class ObjectBroker;
  void    assign (const RemoteHandle&);
  void    reset ();
protected:
  explicit          RemoteHandle  (OrbObject&);
  explicit          RemoteHandle  ();
public:
  uint64            _orbid        () const { return orbo_->orbid(); }
  virtual          ~RemoteHandle  ();
  static NullHandle _null_handle  ()       { return NullHandle(); }
  // Determine if this RemoteHandle contains an object or null handle.
  explicit          operator bool () const noexcept               { return 0 != orbo_->orbid(); }
  bool              operator==    (std::nullptr_t) const noexcept { return !static_cast<bool> (*this); }
  bool              operator!=    (std::nullptr_t) const noexcept { return static_cast<bool> (*this); }
  bool              operator==    (const RemoteHandle&) const noexcept;
  bool              operator!=    (const RemoteHandle&) const noexcept;
  friend bool       operator==    (std::nullptr_t, const RemoteHandle &shd) noexcept { return !static_cast<bool> (shd); }
  friend bool       operator!=    (std::nullptr_t, const RemoteHandle &shd) noexcept { return static_cast<bool> (shd); }
};

// == RemoteMember ==
template<class RemoteHandle>
class RemoteMember : public RemoteHandle {
public:
  inline   RemoteMember (const RemoteHandle &src) : RemoteHandle() { *this = src; }
  explicit RemoteMember () : RemoteHandle() {}
  void     operator=   (const RemoteHandle &src) { RemoteHandle::operator= (src); }
};

// == Conversion Type Tags ==
constexpr struct _ServantType {} _servant; ///< Tag to retrieve servant from remote handle.
constexpr struct _HandleType  {} _handle;  ///< Tag to retrieve remote handle from servant.

// == ObjectBroker ==
class ObjectBroker {
protected:
  static void              tie_handle (RemoteHandle&, uint64);
public:
  static void              pop_handle (FieldReader&, RemoteHandle&);
  static void              post_msg   (FieldBuffer*); ///< Route message to the appropriate party.
  static ServerConnection* new_server_connection (const std::string &feature_keys);
  static ClientConnection* new_client_connection (const std::string &feature_keys);
  static uint         connection_id_from_signal_handler_id (size_t signal_handler_id);
  static inline uint  connection_id_from_orbid  (uint64 orbid)        { return IdentifierParts (orbid).orbid_connection; }
  static inline uint  connection_id_from_handle (const RemoteHandle &sh) { return connection_id_from_orbid (sh._orbid()); }
  static inline uint  connection_id_from_keys   (const vector<std::string> &feature_key_list);
  static inline uint  sender_connection_id      (uint64 msgid)        { return IdentifierParts (msgid).sender_connection; }
  static inline uint  receiver_connection_id    (uint64 msgid)        { return IdentifierParts (msgid).receiver_connection; }
  static FieldBuffer* renew_into_result         (FieldBuffer *fb,  MessageId m, uint rconnection, uint64 h, uint64 l, uint32 n = 1);
  static FieldBuffer* renew_into_result         (FieldReader &fbr, MessageId m, uint rconnection, uint64 h, uint64 l, uint32 n = 1);
  template<class TargetHandle> static TargetHandle remote_handle_down_cast (RemoteHandle smh);
};

// == FieldBuffer ==
union FieldUnion {
  int64        vint64;
  double       vdouble;
  Any         *vany;
  uint64       smem[(sizeof (std::string) + 7) / 8];    // String memory
  void        *pmem[2];                 // equate sizeof (FieldBuffer)
  uint8        bytes[8];                // FieldBuffer types
  struct { uint32 index, capacity; }; // FieldBuffer.buffermem[0]
};

class FieldBuffer { // buffer for marshalling procedure calls
  friend class FieldReader;
  void               check_internal ();
  inline FieldUnion& upeek (uint32 n) const { return buffermem[offset() + n]; }
protected:
  FieldUnion        *buffermem;
  inline void        check ()      { if (AIDA_UNLIKELY (size() > capacity())) check_internal(); }
  inline uint32      offset () const { const uint32 offs = 1 + (capacity() + 7) / 8; return offs; }
  inline TypeKind    type_at  (uint32 n) const { return TypeKind (buffermem[1 + n/8].bytes[n%8]); }
  inline void        set_type (TypeKind ft)  { buffermem[1 + size()/8].bytes[size()%8] = ft; }
  inline FieldUnion& getu () const           { return buffermem[offset() + size()]; }
  inline FieldUnion& addu (TypeKind ft) { set_type (ft); FieldUnion &u = getu(); buffermem[0].index++; check(); return u; }
  inline FieldUnion& uat (uint32 n) const { return AIDA_LIKELY (n < size()) ? upeek (n) : *(FieldUnion*) NULL; }
  explicit           FieldBuffer (uint32 _ntypes);
  explicit           FieldBuffer (uint32, FieldUnion*, uint32);
public:
  virtual     ~FieldBuffer();
  inline uint32 size     () const          { return buffermem[0].index; }
  inline uint32 capacity () const          { return buffermem[0].capacity; }
  inline uint64 first_id () const          { return AIDA_LIKELY (buffermem && size() && type_at (0) == INT64) ? upeek (0).vint64 : 0; }
  inline void add_bool   (bool    vbool)   { FieldUnion &u = addu (BOOL); u.vint64 = vbool; }
  inline void add_int64  (int64 vint64)    { FieldUnion &u = addu (INT64); u.vint64 = vint64; }
  inline void add_evalue (int64 vint64)    { FieldUnion &u = addu (ENUM); u.vint64 = vint64; }
  inline void add_double (double vdouble)  { FieldUnion &u = addu (FLOAT64); u.vdouble = vdouble; }
  inline void add_string (const String &s) { FieldUnion &u = addu (STRING); new (&u) String (s); }
  inline void add_object (uint64 objid)    { FieldUnion &u = addu (INSTANCE); u.vint64 = objid; }
  inline void add_any    (const Any &vany, BaseConnection &bcon);
  inline void add_header1 (MessageId m, uint c, uint64 h, uint64 l) { add_int64 (IdentifierParts (m, c, 0).vuint64); add_int64 (h); add_int64 (l); }
  inline void add_header2 (MessageId m, uint c, uint r, uint64 h, uint64 l) { add_int64 (IdentifierParts (m, c, r).vuint64); add_int64 (h); add_int64 (l); }
  inline FieldBuffer& add_rec (uint32 nt) { FieldUnion &u = addu (RECORD); return *new (&u) FieldBuffer (nt); }
  inline FieldBuffer& add_seq (uint32 nt) { FieldUnion &u = addu (SEQUENCE); return *new (&u) FieldBuffer (nt); }
  inline void         reset();
  String              first_id_str() const;
  String              to_string() const;
  static String       type_name (int field_type);
  static FieldBuffer* _new (uint32 _ntypes); // Heap allocated FieldBuffer
  // static FieldBuffer* new_error (const String &msg, const String &domain = "");
  static FieldBuffer* new_result (MessageId m, uint rconnection, uint64 h, uint64 l, uint32 n = 1);
  inline void operator<<= (uint32 v)          { FieldUnion &u = addu (INT64); u.vint64 = v; }
  inline void operator<<= (ULongIffy v)       { FieldUnion &u = addu (INT64); u.vint64 = v; }
  inline void operator<<= (uint64 v)          { FieldUnion &u = addu (INT64); u.vint64 = v; }
  inline void operator<<= (int32 v)           { FieldUnion &u = addu (INT64); u.vint64 = v; }
  inline void operator<<= (LongIffy v)        { FieldUnion &u = addu (INT64); u.vint64 = v; }
  inline void operator<<= (int64 v)           { FieldUnion &u = addu (INT64); u.vint64 = v; }
  inline void operator<<= (bool   v)          { FieldUnion &u = addu (BOOL); u.vint64 = v; }
  inline void operator<<= (double v)          { FieldUnion &u = addu (FLOAT64); u.vdouble = v; }
  inline void operator<<= (EnumValue e)       { FieldUnion &u = addu (ENUM); u.vint64 = e.value; }
  inline void operator<<= (const String &s)   { FieldUnion &u = addu (STRING); new (&u) String (s); }
  inline void operator<<= (const TypeHash &h) { *this <<= h.typehi; *this <<= h.typelo; }
};

class FieldBuffer8 : public FieldBuffer { // Stack contained buffer for up to 8 fields
  FieldUnion bmem[1 + 1 + 8];
public:
  virtual ~FieldBuffer8 () { reset(); buffermem = NULL; }
  inline   FieldBuffer8 (uint32 ntypes = 8) : FieldBuffer (ntypes, bmem, sizeof (bmem)) { AIDA_ASSERT (ntypes <= 8); }
};

class FieldReader { // read field buffer contents
  const FieldBuffer *fb_;
  uint32             nth_;
  void               check_request (int type);
  inline void        request (int t) { if (AIDA_UNLIKELY (nth_ >= n_types() || get_type() != t)) check_request (t); }
  inline FieldUnion& fb_getu (int t) { request (t); return fb_->upeek (nth_); }
  inline FieldUnion& fb_popu (int t) { request (t); FieldUnion &u = fb_->upeek (nth_++); return u; }
public:
  explicit                 FieldReader (const FieldBuffer &fb) : fb_ (&fb), nth_ (0) {}
  inline const FieldBuffer* field_buffer() const { return fb_; }
  inline void               reset      (const FieldBuffer &fb) { fb_ = &fb; nth_ = 0; }
  inline void               reset      () { fb_ = NULL; nth_ = 0; }
  inline uint32             remaining  () { return n_types() - nth_; }
  inline void               skip       () { if (AIDA_UNLIKELY (nth_ >= n_types())) check_request (0); nth_++; }
  inline void               skip_header () { skip(); skip(); skip(); }
  inline uint32             n_types    () { return fb_->size(); }
  inline TypeKind           get_type   () { return fb_->type_at (nth_); }
  inline int64              get_bool   () { FieldUnion &u = fb_getu (BOOL); return u.vint64; }
  inline int64              get_int64  () { FieldUnion &u = fb_getu (INT64); return u.vint64; }
  inline int64              get_evalue () { FieldUnion &u = fb_getu (ENUM); return u.vint64; }
  inline double             get_double () { FieldUnion &u = fb_getu (FLOAT64); return u.vdouble; }
  inline const String&      get_string () { FieldUnion &u = fb_getu (STRING); return *(String*) &u; }
  inline uint64             get_object () { FieldUnion &u = fb_getu (INSTANCE); return u.vint64; }
  inline const FieldBuffer& get_rec    () { FieldUnion &u = fb_getu (RECORD); return *(FieldBuffer*) &u; }
  inline const FieldBuffer& get_seq    () { FieldUnion &u = fb_getu (SEQUENCE); return *(FieldBuffer*) &u; }
  inline int64              pop_bool   () { FieldUnion &u = fb_popu (BOOL); return u.vint64; }
  inline int64              pop_int64  () { FieldUnion &u = fb_popu (INT64); return u.vint64; }
  inline int64              pop_evalue () { FieldUnion &u = fb_popu (ENUM); return u.vint64; }
  inline double             pop_double () { FieldUnion &u = fb_popu (FLOAT64); return u.vdouble; }
  inline const String&      pop_string () { FieldUnion &u = fb_popu (STRING); return *(String*) &u; }
  inline uint64             pop_object () { FieldUnion &u = fb_popu (INSTANCE); return u.vint64; }
  inline const Any&         pop_any    (BaseConnection &bcon);
  inline const FieldBuffer& pop_rec    () { FieldUnion &u = fb_popu (RECORD); return *(FieldBuffer*) &u; }
  inline const FieldBuffer& pop_seq    () { FieldUnion &u = fb_popu (SEQUENCE); return *(FieldBuffer*) &u; }
  inline void operator>>= (uint32 &v)          { FieldUnion &u = fb_popu (INT64); v = u.vint64; }
  inline void operator>>= (ULongIffy &v)       { FieldUnion &u = fb_popu (INT64); v = u.vint64; }
  inline void operator>>= (uint64 &v)          { FieldUnion &u = fb_popu (INT64); v = u.vint64; }
  inline void operator>>= (int32 &v)           { FieldUnion &u = fb_popu (INT64); v = u.vint64; }
  inline void operator>>= (LongIffy &v)        { FieldUnion &u = fb_popu (INT64); v = u.vint64; }
  inline void operator>>= (int64 &v)           { FieldUnion &u = fb_popu (INT64); v = u.vint64; }
  inline void operator>>= (bool &v)            { FieldUnion &u = fb_popu (BOOL); v = u.vint64; }
  inline void operator>>= (double &v)          { FieldUnion &u = fb_popu (FLOAT64); v = u.vdouble; }
  inline void operator>>= (EnumValue &e)       { FieldUnion &u = fb_popu (ENUM); e.value = u.vint64; }
  inline void operator>>= (String &s)          { FieldUnion &u = fb_popu (STRING); s = *(String*) &u; }
  inline void operator>>= (TypeHash &h)        { *this >>= h.typehi; *this >>= h.typelo; }
  inline void operator>>= (std::vector<bool>::reference v) { bool b; *this >>= b; v = b; }
};

// == Connections ==
/// Base connection context for ORB message exchange.
class BaseConnection {
  uint              index_;
  const std::string feature_keys_;
  friend  class ObjectBroker;
  RAPICORN_CLASS_NON_COPYABLE (BaseConnection);
protected:
  void                   register_connection  ();
  void                   unregister_connection();
  explicit               BaseConnection  (const std::string &feature_keys);
  virtual               ~BaseConnection  ();
  virtual void           send_msg        (FieldBuffer*) = 0; ///< Carry out a remote call syncronously, transfers memory.
  static BaseConnection* connection_from_id (uint id);  ///< Lookup for connection, used by ORB for message delivery.
public:
  uint                   connection_id  () const;   ///< Get unique conneciton ID (returns 0 if unregistered).
  virtual int            notify_fd      () = 0;     ///< Returns fd for POLLIN, to wake up on incomming events.
  virtual bool           pending        () = 0;     ///< Indicate whether any incoming events are pending that need to be dispatched.
  virtual void           dispatch       () = 0;     ///< Dispatch a single event if any is pending.
  virtual void           remote_origin  (ImplicitBase *rorigin);
  virtual RemoteHandle   remote_origin  (const vector<std::string> &feature_key_list);
  virtual Any*           any2remote     (const Any&);
  virtual void           any2local      (Any&);
};

/// Function typoe for internal signal handling.
typedef FieldBuffer* SignalEmitHandler (const FieldBuffer*, void*);

/// Connection context for IPC servers. @nosubgrouping
class ServerConnection : public BaseConnection {
  RAPICORN_CLASS_NON_COPYABLE (ServerConnection);
protected:
  /*ctor*/           ServerConnection (const std::string &feature_keys);
  virtual           ~ServerConnection ();
public: /// @name API for remote calls
  virtual uint64        instance2orbid (ImplicitBase*) = 0;
  virtual ImplicitBase* orbid2instance (uint64) = 0;
  virtual ImplicitBase* remote_origin  () const = 0;
protected: /// @name Registry for IPC method lookups
  static DispatchFunc find_method (uint64 hi, uint64 lo); ///< Lookup method in registry.
public:
  struct MethodEntry       { uint64 hashhi, hashlo; DispatchFunc dispatcher; };
  struct MethodRegistry    /// Registry structure for IPC method stubs.
  {
    template<class T, size_t S> MethodRegistry  (T (&static_const_entries)[S])
    { for (size_t i = 0; i < S; i++) register_method (static_const_entries[i]); }
  private: static void register_method  (const MethodEntry &mentry);
  };
  typedef std::function<void (Rapicorn::Aida::FieldReader&)> EmitResultHandler;
  virtual void emit_result_handler_add (size_t id, const EmitResultHandler &handler) = 0;
};

/// Connection context for IPC clients. @nosubgrouping
class ClientConnection : public BaseConnection {
  RAPICORN_CLASS_NON_COPYABLE (ClientConnection);
protected:
  explicit              ClientConnection (const std::string &feature_keys);
  virtual              ~ClientConnection ();
public: /// @name API for remote calls.
  virtual FieldBuffer*  call_remote (FieldBuffer*) = 0; ///< Carry out a remote call syncronously, transfers memory.
public: /// @name API for signal event handlers.
  virtual size_t        signal_connect    (uint64 hhi, uint64 hlo, uint64 orbid, SignalEmitHandler seh, void *data) = 0;
  virtual bool          signal_disconnect (size_t signal_handler_id) = 0;
public: /// @name API for remote types.
  virtual std::string   type_name_from_orbid (uint64 orbid) = 0;
};

// == inline implementations ==
template<class E> inline TypeCode
TypeCode::from_enum () // fallback for unspecialized types
{
  static_assert (0 * sizeof (E), "no EnumInfo specialisation for this type");
  return *(TypeCode*) NULL; // silence compiler
}

template<class V> inline
Any::Any (const V &value) :
  type_code_ (TypeMap::notype()), u_ {0}
{
  this->operator<<= (value);
}

template<> inline
Any::Any<Any::Field> (const Any::Field &clone) :
  type_code_ (TypeMap::notype()), u_ {0}
{
  this->operator= (clone);
}

inline
Any::Any() :
  type_code_ (TypeMap::notype()), u_ {0}
{}

inline
Any::Any (const TypeCode &tc) :
  type_code_ (plain_zero_type (tc.kind()) ? tc : TypeMap::notype()), u_ {0}
{
  if (!plain_zero_type (tc.kind()))
    retype (tc);        // carry out special initializations
}

inline bool
Any::plain_zero_type (TypeKind kind)
{
  switch (kind)
    {
    case UNTYPED: case BOOL: case INT32: case INT64: case FLOAT64: case ENUM:
      return true;      // simple, properly initialized with u {0}
    case STRING: case ANY: case SEQUENCE: case RECORD: case INSTANCE:
    default:
      return false;     // complex types, needing special initializations
    }
}

inline
Any::Any (const Any &clone) :
  type_code_ (TypeMap::notype()), u_ {0}
{
  this->operator= (clone);
}

inline
Any::~Any ()
{
  reset();
}

template<class TargetHandle> TargetHandle
ObjectBroker::remote_handle_down_cast (RemoteHandle smh)
{
  TargetHandle target;
  target.assign (smh);                        // aka reinterpret_cast
  return TargetHandle::down_cast (target);    // aka dynamic_cast (remote)
}

inline void
FieldBuffer::reset()
{
  if (!buffermem)
    return;
  while (size() > 0)
    {
      buffermem[0].index--; // causes size()--
      switch (type_at (size()))
        {
        case STRING:    { FieldUnion &u = getu(); ((String*) &u)->~String(); }; break;
        case ANY:       { FieldUnion &u = getu(); delete u.vany; }; break;
        case SEQUENCE:
        case RECORD:    { FieldUnion &u = getu(); ((FieldBuffer*) &u)->~FieldBuffer(); }; break;
        default: ;
        }
    }
}

inline void
FieldBuffer::add_any (const Any &vany, BaseConnection &bcon)
{
  FieldUnion &u = addu (ANY);
  u.vany = bcon.any2remote (vany);
}

inline const Any&
FieldReader::pop_any (BaseConnection &bcon)
{
  FieldUnion &u = fb_popu (ANY);
  bcon.any2local (*u.vany);
  return *u.vany;
}

} } // Rapicorn::Aida

// == Signals ==
#include "aidasignal.hh"

#endif // __RAPICORN_AIDA_HH__
