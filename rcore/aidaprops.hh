// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_AIDA_PROPS_HH__
#define __RAPICORN_AIDA_PROPS_HH__

#include <rcore/aida.hh>
#include <rcore/strings.hh>

namespace Rapicorn { namespace Aida {

// == Parameter ==
class Parameter {
  const String                           field_name_;
  const std::function<void (const Any&)> setter_;
  const std::function<Any ()>            getter_;
  const std::vector<String>              aux_data_;
  const Any                              live_instance_; // keep Handle/Iface instance alive
  String                                 get_aux_string (const String&, const String&);
public:
  virtual ~Parameter();
  template<class Klass, class Value> // Handle/Iface accessors for plain value (bool, int, float)
  Parameter (Klass &instance, const String &field_name, void (Klass::*setter) (Value), Value (Klass::*getter) () const) :
    field_name_ (field_name),
    setter_ ([&instance, setter] (const Any &any) -> void { return (instance .* setter) (any.get<Value>()); }),
    getter_ ([&instance, getter] ()               -> Any  { Any a; a.set ((instance .* getter) ()); return a; }),
    aux_data_ (instance.__aida_aux_data__()),
    live_instance_ (({ Any a; a.set (instance); a; }))
  {}
  template<class Klass, class Value> // Handle/Iface accessors for struct value (std::string, record)
  Parameter (Klass &instance, const String &field_name, void (Klass::*setter) (const Value&), Value (Klass::*getter) () const) :
    field_name_ (field_name),
    setter_ ([&instance, setter] (const Any &any) -> void { return (instance .* setter) (any.get<Value>()); }),
    getter_ ([&instance, getter] ()               -> Any  { Any a; a.set ((instance .* getter) ()); return a; }),
    aux_data_ (instance.__aida_aux_data__()),
    live_instance_ (({ Any a; a.set (instance); a; }))
  {}
  template<class Klass, class Value> // Iface accessors for instance value
  Parameter (Klass &instance, const String &field_name, void (Klass::*setter) (Value*), std::shared_ptr<Value> (Klass::*getter) () const) :
    field_name_ (field_name),
    setter_ ([&instance, setter] (const Any &any) -> void { return (instance .* setter) (any.get<std::shared_ptr<Value> >().get()); }),
    getter_ ([&instance, getter] ()               -> Any  { Any a; a.set ((instance .* getter) ()); return a; }),
    aux_data_ (instance.__aida_aux_data__()),
    live_instance_ (({ Any a; a.set (instance); a; }))
  {}
  void   set        (const Any &any);
  Any    get        () const;
  String field_name () const;
  template<typename Value = String>
  Value  get_aux    (const String &key, const String &fallback = "") { return string_to_type<Value> (get_aux_string (key, fallback)); }
};


// == PropertyHostInterface ==
typedef ImplicitBase PropertyHostInterface;

// == Property ==
class Property {
protected:
  virtual ~Property();
public:
  const char *ident;
  char       *label;
  char       *blurb;
  char       *hints;
  Property (const char *cident, const char *clabel, const char *cblurb, const char *chints);
  virtual void   set_value   (PropertyHostInterface &obj, const String &svalue) = 0;
  virtual String get_value   (PropertyHostInterface &obj) = 0;
  virtual bool   get_range   (PropertyHostInterface &obj, double &minimum, double &maximum, double &stepping) = 0;
  bool           readable    () const;
  bool           writable    () const;
};

// == PropertyList ==
struct PropertyList /// Container structure for property descriptions.
{
  typedef Aida::Property Property; // make Property available as class member
private:
  size_t     n_properties_;
  Property **properties_;
  void       append_properties (size_t n_props, Property **props, const PropertyList &c0, const PropertyList &c1,
                                const PropertyList &c2, const PropertyList &c3, const PropertyList &c4, const PropertyList &c5,
                                const PropertyList &c6, const PropertyList &c7, const PropertyList &c8, const PropertyList &c9);
public:
  Property** list_properties   (size_t *n_properties) const;
  /*dtor*/  ~PropertyList      ();
  explicit   PropertyList      () : n_properties_ (0), properties_ (NULL) {}
  template<typename Array>
  explicit   PropertyList      (Array &a, const PropertyList &c0 = PropertyList(), const PropertyList &c1 = PropertyList(),
                                const PropertyList &c2 = PropertyList(), const PropertyList &c3 = PropertyList(),
                                const PropertyList &c4 = PropertyList(), const PropertyList &c5 = PropertyList(),
                                const PropertyList &c6 = PropertyList(), const PropertyList &c7 = PropertyList(),
                                const PropertyList &c8 = PropertyList(), const PropertyList &c9 = PropertyList()) :
    n_properties_ (0), properties_ (NULL)
  {
    const size_t n_props = sizeof (a) / sizeof (a[0]);
    Property *props[n_props];
    for (size_t i = 0; i < sizeof (a) / sizeof (a[0]); i++)
      props[i] = a[i];
    append_properties (n_props, props, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9);
  }
};

// == Property Creation ==
#define RAPICORN_AIDA_PROPERTY(Type, accessor, label, blurb, ...)   \
  Rapicorn::Aida::create_property (&Type::accessor, &Type::accessor, #accessor, label, blurb, __VA_ARGS__)

#define RAPICORN_AIDA_PROPERTY_CHAIN(first,...)                    (*({ \
  static Property *__dummy_[] = {};                                     \
  static const PropertyList property_list (__dummy_, first, __VA_ARGS__); \
  &property_list; }))

/* --- bool --- */
template<class Class>
struct PropertyBool : Property {
  void (Class::*setter) (bool);
  bool (Class::*getter) () const;
  PropertyBool (void (Class::*csetter) (bool), bool (Class::*cgetter) () const,
                const char *cident, const char *clabel, const char *cblurb,
                const char *chints);
  virtual void   set_value   (PropertyHostInterface &obj, const String &svalue);
  virtual String get_value   (PropertyHostInterface &obj);
  virtual bool   get_range   (PropertyHostInterface &obj, double &minimum, double &maximum, double &stepping) { return false; }
};
template<class Class> inline Property*
create_property (void (Class::*setter) (bool), bool (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb, const char *hints)
{ return new PropertyBool<Class> (setter, getter, ident, label, blurb, hints); }

/* --- range --- */
template<class Class, typename Type>
struct PropertyRange : Property {
  Type minimum_value;
  Type maximum_value;
  Type stepping;
  void (Class::*setter) (Type);
  Type (Class::*getter) () const;
  PropertyRange (void (Class::*csetter) (Type), Type (Class::*cgetter) () const,
                 const char *cident, const char *clabel, const char *cblurb,
                 Type cminimum_value, Type cmaximum_value,
                 Type cstepping, const char *chints);
  virtual void   set_value   (PropertyHostInterface &obj, const String &svalue);
  virtual String get_value   (PropertyHostInterface &obj);
  virtual bool   get_range   (PropertyHostInterface &obj, double &minimum, double &maximum, double &stepping);
};
/* int */
template<class Class> inline Property*
create_property (void (Class::*setter) (int), int (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb,
                 int min_value, int max_value, int stepping, const char *hints)
{ return new PropertyRange<Class,int> (setter, getter, ident, label, blurb, min_value, max_value, stepping, hints); }
/* int64 */
template<class Class> inline Property*
create_property (void (Class::*setter) (int64), int64 (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb,
                 int64 min_value, int64 max_value, int64 stepping, const char *hints)
{ return new PropertyRange<Class,int64> (setter, getter, ident, label, blurb, min_value, max_value, stepping, hints); }
/* double */
template<class Class> inline Property*
create_property (void (Class::*setter) (double), double (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb,
                 double min_value, double max_value, double stepping, const char *hints)
{ return new PropertyRange<Class,double> (setter, getter, ident, label, blurb, min_value, max_value, stepping, hints); }

/* --- string --- */
template<class Class>
struct PropertyString : Property {
  void   (Class::*setter) (const String&);
  String (Class::*getter) () const;
  PropertyString (void (Class::*csetter) (const String&), String (Class::*cgetter) () const,
                  const char *cident, const char *clabel, const char *cblurb,
                  const char *chints);
  virtual void   set_value   (PropertyHostInterface &obj, const String &svalue);
  virtual String get_value   (PropertyHostInterface &obj);
  virtual bool   get_range   (PropertyHostInterface &obj, double &minimum, double &maximum, double &stepping) { return false; }
};
template<class Class> inline Property*
create_property (void (Class::*setter) (const String&), String (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb, const char *hints)
{ return new PropertyString<Class> (setter, getter, ident, label, blurb, hints); }

// == Enum Properties ==
template<class Class, typename Type>
struct PropertyEnum : Property {
  const EnumInfo enum_;
  void (Class::*setter) (Type);
  Type (Class::*getter) () const;
  PropertyEnum (void (Class::*csetter) (Type), Type (Class::*cgetter) () const,
                const char *cident, const char *clabel, const char *cblurb,
                const EnumInfo enuminfo, const char *chints);
  virtual void   set_value   (PropertyHostInterface &obj, const String &svalue);
  virtual String get_value   (PropertyHostInterface &obj);
  virtual bool   get_range   (PropertyHostInterface &obj, double &minimum, double &maximum, double &stepping) { return false; }
};
template<class Class, typename Type,
         typename std::enable_if<std::is_enum<Type>::value>::type* = nullptr> inline Property*
create_property (void (Class::*setter) (Type), Type (Class::*getter) () const,
                 const char *ident, const char *label, const char *blurb, const char *hints)
{
  return new PropertyEnum<Class,Type> (setter, getter, ident, label, blurb, enum_info<Type>(), hints);
}

/* --- implementations --- */
/* bool property implementation */
template<class Class>
PropertyBool<Class>::PropertyBool (void (Class::*csetter) (bool), bool (Class::*cgetter) () const,
                                   const char *cident, const char *clabel, const char *cblurb,
                                   const char *chints) :
  Property (cident, clabel, cblurb, chints),
  setter (csetter),
  getter (cgetter)
{}

template<class Class> void
PropertyBool<Class>::set_value (PropertyHostInterface &obj, const String &svalue)
{
  bool b = string_to_bool (svalue);
  Class *instance = dynamic_cast<Class*> (&obj);
  (instance->*setter) (b);
}

template<class Class> String
PropertyBool<Class>::get_value (PropertyHostInterface &obj)
{
  Class *instance = dynamic_cast<Class*> (&obj);
  bool b = (instance->*getter) ();
  return string_from_bool (b);
}

/* range property implementation */
template<class Class, typename Type>
PropertyRange<Class,Type>::PropertyRange (void (Class::*csetter) (Type), Type (Class::*cgetter) () const,
                                          const char *cident, const char *clabel, const char *cblurb,
                                          Type cminimum_value, Type cmaximum_value,
                                          Type cstepping, const char *chints) :
  Property (cident, clabel, cblurb, chints),
  minimum_value (cminimum_value),
  maximum_value (cmaximum_value),
  stepping (cstepping),
  setter (csetter),
  getter (cgetter)
{
  AIDA_ASSERT (minimum_value <= maximum_value);
  AIDA_ASSERT (minimum_value + stepping <= maximum_value);
}

template<class Class, typename Type> void
PropertyRange<Class,Type>::set_value (PropertyHostInterface &obj, const String &svalue)
{
  Type v = string_to_type<Type> (svalue);
  Class *instance = dynamic_cast<Class*> (&obj);
  (instance->*setter) (v);
}

template<class Class, typename Type> String
PropertyRange<Class,Type>::get_value (PropertyHostInterface &obj)
{
  Class *instance = dynamic_cast<Class*> (&obj);
  Type v = (instance->*getter) ();
  return string_from_type<Type> (v);
}

template<class Class, typename Type> bool
PropertyRange<Class,Type>::get_range (PropertyHostInterface &obj, double &minimum, double &maximum, double &vstepping)
{
  minimum = minimum_value, maximum = maximum_value, vstepping = stepping;
  return true;
}

/* string property implementation */
template<class Class>
PropertyString<Class>::PropertyString (void (Class::*csetter) (const String&), String (Class::*cgetter) () const,
                                       const char *cident, const char *clabel, const char *cblurb,
                                       const char *chints) :
  Property (cident, clabel, cblurb, chints),
  setter (csetter),
  getter (cgetter)
{}

template<class Class> void
PropertyString<Class>::set_value (PropertyHostInterface &obj, const String &svalue)
{
  Class *instance = dynamic_cast<Class*> (&obj);
  (instance->*setter) (svalue);
}

template<class Class> String
PropertyString<Class>::get_value (PropertyHostInterface &obj)
{
  Class *instance = dynamic_cast<Class*> (&obj);
  return (instance->*getter) ();
}

/* enum property implementation */
template<class Class, typename Type>
PropertyEnum<Class,Type>::PropertyEnum (void (Class::*csetter) (Type), Type (Class::*cgetter) () const,
                                        const char *cident, const char *clabel, const char *cblurb,
                                        const EnumInfo enuminfo, const char *chints) :
  Property (cident, clabel, cblurb, chints),
  enum_ (enuminfo),
  setter (csetter),
  getter (cgetter)
{}

template<class Class, typename Type> void
PropertyEnum<Class,Type>::set_value (PropertyHostInterface &obj, const String &svalue)
{
  String error_string;
  const EnumValue *ev = enum_.find_value (svalue);
  if (!ev)
    print_warning (String (__PRETTY_FUNCTION__) + ": invalid enum value name: " + svalue);
  Type v = Type (ev ? ev->value : 0);
  Class *instance = dynamic_cast<Class*> (&obj);
  (instance->*setter) (v);
}

template<class Class, typename Type> String
PropertyEnum<Class,Type>::get_value (PropertyHostInterface &obj)
{
  Class *instance = dynamic_cast<Class*> (&obj);
  Type v = (instance->*getter) ();
  const EnumValue *ev = enum_.find_value (v);
  if (!ev)
    print_warning (String (__PRETTY_FUNCTION__) + ": unrecognized enum value");
  return ev ? ev->ident : "";
}

} } // Rapicorn::Aida

#endif  // __RAPICORN_AIDA_PROPS_HH__
